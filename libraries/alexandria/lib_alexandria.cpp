#include <steem/utilities/git_revision.hpp>
#include <steem/utilities/key_conversion.hpp>
#include <steem/utilities/words.hpp>

#include <steem/protocol/base.hpp>
#include <steem/alexandria/lib_alexandria.hpp>
#include <steem/alexandria/api_documentation.hpp>
#include <steem/alexandria/reflect_util.hpp>


#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <fc/container/deque.hpp>
#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/smart_ref_impl.hpp>

#define BRAIN_KEY_WORD_COUNT 16

namespace steem { namespace wallet {

using steem::plugins::condenser_api::legacy_asset;

namespace detail {

template<class T>
optional<T> maybe_id( const string& name_or_id )
{
   if( std::isdigit( name_or_id.front() ) )
   {
      try
      {
         return fc::variant(name_or_id).as<T>();
      }
      catch (const fc::exception&)
      {
      }
   }
   return optional<T>();
}

string pubkey_to_shorthash( const public_key_type& key )
{
   uint32_t x = fc::sha256::hash(key)._hash[0];
   static const char hd[] = "0123456789abcdef";
   string result;

   result += hd[(x >> 0x1c) & 0x0f];
   result += hd[(x >> 0x18) & 0x0f];
   result += hd[(x >> 0x14) & 0x0f];
   result += hd[(x >> 0x10) & 0x0f];
   result += hd[(x >> 0x0c) & 0x0f];
   result += hd[(x >> 0x08) & 0x0f];
   result += hd[(x >> 0x04) & 0x0f];
   result += hd[(x        ) & 0x0f];

   return result;
}


fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
}

struct op_prototype_visitor
{
   typedef void result_type;

   int t = 0;
   flat_map< std::string, operation >& name2op;

   op_prototype_visitor(
      int _t,
      flat_map< std::string, operation >& _prototype_ops
      ):t(_t), name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

class alexandria_api_impl
{
   public:
      api_documentation method_documentation;
   private:
      void enable_umask_protection() {
#ifdef __unix__
         _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
      }

      void disable_umask_protection() {
#ifdef __unix__
         umask( _old_umask );
#endif
      }

      void init_prototype_ops()
      {
         operation op;
         for( int t=0; t<op.count(); t++ )
         {
            op.set_which( t );
            op.visit( op_prototype_visitor(t, _prototype_ops) );
         }
         return;
      }

public:
   alexandria_api& self;
   alexandria_api_impl( alexandria_api& s, const string& _ws_server, const steem::protocol::chain_id_type& _steem_chain_id, fc::api< remote_node_api > rapi )
      : self( s ),
        _remote_api( rapi )
   {
      init_prototype_ops();

      ws_server = _ws_server;
      steem_chain_id = _steem_chain_id;
   }
   virtual ~alexandria_api_impl()
   {}

   variant info() const
   {
      auto dynamic_props = _remote_api->get_dynamic_global_properties();
      fc::mutable_variant_object result(fc::variant(dynamic_props).get_object());
      result["witness_majority_version"] = fc::string( _remote_api->get_witness_schedule().majority_version );
      result["hardfork_version"] = fc::string( _remote_api->get_hardfork_version() );
      //result["head_block_id"] = dynamic_props.head_block_id;
      result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                          time_point_sec(time_point::now()),
                                                                          " old");
      result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
      result["median_sbd1_price"] = _remote_api->get_current_median_history_price(SBD1_SYMBOL);
      result["median_sbd2_price"] = _remote_api->get_current_median_history_price(SBD2_SYMBOL);
      result["median_sbd3_price"] = _remote_api->get_current_median_history_price(SBD3_SYMBOL);
      result["median_sbd4_price"] = _remote_api->get_current_median_history_price(SBD4_SYMBOL);
      result["median_sbd5_price"] = _remote_api->get_current_median_history_price(SBD5_SYMBOL);

      result["account_creation_fee"] = _remote_api->get_chain_properties().account_creation_fee;
      //result["post_reward_fund"] = fc::variant(_remote_api->get_reward_fund( STEEM_POST_REWARD_FUND_NAME )).get_object();
      return result;
   }

   variant_object about() const
   {
      string client_version( steem::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::mutable_variant_object result;
      result["blockchain_version"]       = STEEM_BLOCKCHAIN_VERSION;
      result["client_version"]           = client_version;
      result["steem_revision"]           = steem::utilities::git_revision_sha;
      result["steem_revision_age"]       = fc::get_approximate_relative_time_string( fc::time_point_sec( steem::utilities::git_revision_unix_timestamp ) );
      result["fc_revision"]              = fc::git_revision_sha;
      result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

      try
      {
         auto v = _remote_api->get_version();
         result["server_blockchain_version"] = v.blockchain_version;
         result["server_steem_revision"] = v.steem_revision;
         result["server_fc_revision"] = v.fc_revision;
      }
      catch( fc::exception& )
      {
         result["server"] = "could not retrieve server version information";
      }

      return result;
   }

   condenser_api::api_account_object get_account( string account_name ) const
   {
      auto accounts = _remote_api->get_accounts( { account_name } );
      FC_ASSERT( !accounts.empty(), "Unknown account" );
      return accounts.front();
   }

   signed_transaction set_voting_proxy(string account_to_modify, string proxy, bool broadcast /* = false */)
   { try {
      account_witness_proxy_operation op;
      op.account = account_to_modify;
      op.proxy = proxy;

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(proxy)(broadcast) ) }

   optional< condenser_api::api_witness_object > get_witness( string owner_account )
   {
      return _remote_api->get_witness_by_account( owner_account );
   }

   void set_transaction_expiration( uint32_t tx_expiration_seconds )
   {
      FC_ASSERT( tx_expiration_seconds < STEEM_MAX_TIME_UNTIL_EXPIRATION );
      _tx_expiration_seconds = tx_expiration_seconds;
   }

   annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
   {
      //set fees first
//      class op_visitor{
//      public:
//         op_visitor(){};
//         typedef void result_type;
//         result_type operator()( base_operation& bop){
//            if(bop.has_special_fee())
//               return;
//            asset req_fee = bop.get_required_fee(STEEM_SYMBOL);
//            bop.fee = req_fee;
//         };
//      };
//      op_visitor op_v;

//      for(operation& o: tx.operations){
//         o.visit(op_v);
//      }

//      flat_set< account_name_type >   req_active_approvals;
//      flat_set< account_name_type >   req_owner_approvals;
//      vector< authority >  other_auths;

//      tx.get_required_authorities( req_active_approvals, req_owner_approvals, other_auths );

//      for( const auto& auth : other_auths )
//         for( const auto& a : auth.account_auths )
//            req_active_approvals.insert(a.first);

//      // std::merge lets us de-duplica te account_id's that occur in both
//      //   sets, and dump them into a vector (as required by remote_db api)
//      //   at the same time
//      vector< account_name_type > v_approving_account_names;
//      std::merge(req_active_approvals.begin(), req_active_approvals.end(),
//                 req_owner_approvals.begin() , req_owner_approvals.end(),
//                 std::back_inserter( v_approving_account_names ) );

//      /// TODO: fetch the accounts specified via other_auths as well.

//      auto approving_account_objects = _remote_api->get_accounts( v_approving_account_names );

//      /// TODO: recursively check one layer deeper in the authority tree for keys

//      FC_ASSERT( approving_account_objects.size() == v_approving_account_names.size(), "", ("aco.size:", approving_account_objects.size())("acn",v_approving_account_names.size()) );

//      flat_map< string, condenser_api::api_account_object > approving_account_lut;
//      size_t i = 0;
//      for( const optional< condenser_api::api_account_object >& approving_acct : approving_account_objects )
//      {
//         if( !approving_acct.valid() )
//         {
//            wlog( "operation_get_required_auths said approval of non-existing account ${name} was needed",
//                  ("name", v_approving_account_names[i]) );
//            i++;
//            continue;
//         }
//         approving_account_lut[ approving_acct->name ] =  *approving_acct;
//         i++;
//      }
//      auto get_account_from_lut = [&]( const std::string& name ) -> const condenser_api::api_account_object&
//      {
//         auto it = approving_account_lut.find( name );
//         FC_ASSERT( it != approving_account_lut.end() );
//         return it->second;
//      };

//      flat_set<public_key_type> approving_key_set;
//      for( account_name_type& acct_name : req_active_approvals )
//      {
//         const auto it = approving_account_lut.find( acct_name );
//         if( it == approving_account_lut.end() )
//            continue;
//         const condenser_api::api_account_object& acct = it->second;
//         vector<public_key_type> v_approving_keys = acct.active.get_keys();
//         wdump((v_approving_keys));
//         for( const public_key_type& approving_key : v_approving_keys )
//         {
//            wdump((approving_key));
//            approving_key_set.insert( approving_key );
//         }
//      }


//      for( const account_name_type& acct_name : req_owner_approvals )
//      {
//         const auto it = approving_account_lut.find( acct_name );
//         if( it == approving_account_lut.end() )
//            continue;
//         const condenser_api::api_account_object& acct = it->second;
//         vector<public_key_type> v_approving_keys = acct.owner.get_keys();
//         for( const public_key_type& approving_key : v_approving_keys )
//         {
//            wdump((approving_key));
//            approving_key_set.insert( approving_key );
//         }
//      }
//      for( const authority& a : other_auths )
//      {
//         for( const auto& k : a.key_auths )
//         {
//            wdump((k.first));
//            approving_key_set.insert( k.first );
//         }
//      }

//      auto dyn_props = _remote_api->get_dynamic_global_properties();
//      tx.set_reference_block( dyn_props.head_block_id );
//      tx.set_expiration( dyn_props.time + fc::seconds(_tx_expiration_seconds) );
//      tx.signatures.clear();

//      //idump((_keys));
//      flat_set< public_key_type > available_keys;
//      flat_map< public_key_type, fc::ecc::private_key > available_private_keys;

//      auto minimal_signing_keys = tx.minimize_required_signatures(
//         steem_chain_id,
//         available_keys,
//         [&]( const string& account_name ) -> const authority&
//         { return (get_account_from_lut( account_name ).active); },
//         [&]( const string& account_name ) -> const authority&
//         { return (get_account_from_lut( account_name ).owner); },
//         STEEM_MAX_SIG_CHECK_DEPTH
//         );

//      for( const public_key_type& k : minimal_signing_keys )
//      {
//         auto it = available_private_keys.find(k);
//         FC_ASSERT( it != available_private_keys.end() );
//         tx.sign( it->second, steem_chain_id );
//      }

//      if( broadcast ) {
//         try {
//            auto result = _remote_api->broadcast_transaction_synchronous( tx );
//            annotated_signed_transaction rtrx(tx);
//            rtrx.block_num = result.block_num;
//            rtrx.transaction_num = result.trx_num;
//            return rtrx;
//         }
//         catch (const fc::exception& e)
//         {
//            elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
//            throw;
//         }
//      }
      return tx;
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["gethelp"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["list_my_accounts"] = [](variant result, const fc::variants& a ) {
         std::stringstream out;

         auto accounts = result.as<vector<condenser_api::api_account_object>>();
         asset total_steem;
         asset total_vest(0, VESTS_SYMBOL );
         for( const auto& a : accounts ) {
            total_steem += a.balance.to_asset();
            total_vest  += a.vesting_shares.to_asset();
            out << std::left << std::setw( 17 ) << std::string(a.name)
                << std::right << std::setw(18) << fc::variant(a.balance).as_string() <<" "
                << std::right << std::setw(26) << fc::variant(a.vesting_shares).as_string() <<" \n";
         }
         out << "-------------------------------------------------------------------------\n";
            out << std::left << std::setw( 17 ) << "TOTAL"
                << std::right << std::setw(18) << legacy_asset::from_asset(total_steem).to_string() <<" "
                << std::right << std::setw(26) << legacy_asset::from_asset(total_vest).to_string() <<" \n";
         return out.str();
      };
      m["get_account_history"] = []( variant result, const fc::variants& a ) {
         std::stringstream ss;
         ss << std::left << std::setw( 5 )  << "#" << " ";
         ss << std::left << std::setw( 10 ) << "BLOCK #" << " ";
         ss << std::left << std::setw( 15 ) << "TRX ID" << " ";
         ss << std::left << std::setw( 20 ) << "OPERATION" << " ";
         ss << std::left << std::setw( 50 ) << "DETAILS" << "\n";
         ss << "-------------------------------------------------------------------------------\n";
         const auto& results = result.get_array();
         for( const auto& item : results ) {
            ss << std::left << std::setw(5) << item.get_array()[0].as_string() << " ";
            const auto& op = item.get_array()[1].get_object();
            ss << std::left << std::setw(10) << op["block"].as_string() << " ";
            ss << std::left << std::setw(15) << op["trx_id"].as_string() << " ";
            const auto& opop = op["op"].get_array();
            ss << std::left << std::setw(20) << opop[0].as_string() << " ";
            ss << std::left << std::setw(50) << fc::json::to_string(opop[1]) << "\n ";
         }
         return ss.str();
      };


      return m;
   }

   operation get_prototype_operation( string operation_name )
   {
      auto it = _prototype_ops.find( operation_name );
      if( it == _prototype_ops.end() )
         FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
      return it->second;
   }

   string                                  ws_server;
   steem::protocol::chain_id_type          steem_chain_id;

   fc::sha512                              _checksum;
   fc::api< remote_node_api >              _remote_api;
   uint32_t                                _tx_expiration_seconds = 30;

   flat_map<string, operation>             _prototype_ops;

   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
};

} } } // steem::wallet::detail



namespace steem { namespace wallet {

alexandria_api::alexandria_api(const string& _ws_server, const steem::protocol::chain_id_type& _steem_chain_id, fc::api< remote_node_api > rapi)
   : my(new detail::alexandria_api_impl(*this, _ws_server, _steem_chain_id, rapi))
{}

alexandria_api::~alexandria_api(){}

optional< database_api::api_signed_block_object > alexandria_api::get_block(uint32_t num)
{
   return my->_remote_api->get_block( num );
}

vector< condenser_api::api_operation_object > alexandria_api::get_ops_in_block(uint32_t block_num, bool only_virtual)
{
   return my->_remote_api->get_ops_in_block( block_num, only_virtual );
}

vector< account_name_type > alexandria_api::get_active_witnesses()const {
   return my->_remote_api->get_active_witnesses();
}

brain_key_info alexandria_api::suggest_brain_key()const
{
   brain_key_info result;
   // create a private key for secure entropy
   fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
   fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
   fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
   fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
   fc::bigint entropy(entropy1);
   entropy <<= 8*sha_entropy1.data_size();
   entropy += entropy2;
   string brain_key = "";

   for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
   {
      fc::bigint choice = entropy % steem::words::word_list_size;
      entropy /= steem::words::word_list_size;
      if( i > 0 )
         brain_key += " ";
      brain_key += steem::words::word_list[ choice.to_int64() ];
   }

   brain_key = normalize_brain_key(brain_key);
   fc::ecc::private_key priv_key = detail::derive_private_key( brain_key, 0 );
   result.brain_priv_key = brain_key;
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

string alexandria_api::serialize_transaction( signed_transaction tx )const
{
   return fc::to_hex(fc::raw::pack_to_vector(tx));
}

string alexandria_api::normalize_brain_key(string s) const
{
   return detail::normalize_brain_key( s );
}

variant alexandria_api::info()
{
   return my->info();
}

variant_object alexandria_api::about() const
{
    return my->about();
}

/*
fc::ecc::private_key alexandria_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   return detail::derive_private_key( prefix_string, sequence_number );
}
*/

vector< account_name_type > alexandria_api::list_witnesses(const string& lowerbound, uint32_t limit)
{
   return my->_remote_api->lookup_witness_accounts( lowerbound, limit );
}

optional< condenser_api::api_witness_object > alexandria_api::get_witness(string owner_account)
{
   return my->get_witness(owner_account);
}

annotated_signed_transaction alexandria_api::set_voting_proxy(string account_to_modify, string voting_account, bool broadcast /* = false */)
{ return my->set_voting_proxy(account_to_modify, voting_account, broadcast); }

annotated_signed_transaction alexandria_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
{ try {
   return my->sign_transaction( tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

operation alexandria_api::get_prototype_operation(string operation_name) {
   return my->get_prototype_operation( operation_name );
}

string alexandria_api::help()const
{
   std::vector<std::string> method_names = my->method_documentation.get_method_names();
   std::stringstream ss;
   for (const std::string method_name : method_names)
   {
      try
      {
         ss << my->method_documentation.get_brief_description(method_name);
      }
      catch (const fc::key_not_found_exception&)
      {
         ss << method_name << " (no help available)\n";
      }
   }
   return ss.str();
}

string alexandria_api::gethelp(const string& method)const
{
   fc::api<alexandria_api> tmp;
   std::stringstream ss;
   ss << "\n";

   std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
   if (!doxygenHelpString.empty())
      ss << doxygenHelpString;
   else
      ss << "No help defined for method " << method << "\n";

   return ss.str();
}

std::map<string,std::function<string(fc::variant,const fc::variants&)> >
alexandria_api::get_result_formatters() const
{
   return my->get_result_formatters();
}

condenser_api::api_feed_history_object alexandria_api::get_feed_history(asset_symbol_type symbol)const {
   return my->_remote_api->get_feed_history(symbol);
}

/**
 * This method is used by faucets to create new accounts for other users which must
 * provide their desired keys. The resulting account may not be controllable by this
 * wallet.
 */
operation alexandria_api::create_account( string creator,
                                      string new_account_name,
                                      string json_meta,
                                      public_key_type owner,
                                      public_key_type active,
                                      public_key_type memo)const
{ try {

   account_create_operation op;
   op.creator = creator;
   op.new_account_name = new_account_name;
   op.owner = authority( 1, owner, 1 );
   op.active = authority( 1, active, 1 );
   op.memo_key = memo;
   op.json_metadata = json_meta;
   op.fee = my->_remote_api->get_chain_properties().account_creation_fee * asset( 1, STEEM_SYMBOL );

   return op;
} FC_CAPTURE_AND_RETHROW( (creator)(new_account_name)(json_meta)(owner)(active)(memo)) }

annotated_signed_transaction alexandria_api::request_account_recovery( string recovery_account, string account_to_recover, authority new_authority, bool broadcast )
{
   request_account_recovery_operation op;
   op.recovery_account = recovery_account;
   op.account_to_recover = account_to_recover;
   op.new_owner_authority = new_authority;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::recover_account( string account_to_recover, authority recent_authority, authority new_authority, bool broadcast ) {

   recover_account_operation op;
   op.account_to_recover = account_to_recover;
   op.new_owner_authority = new_authority;
   op.recent_owner_authority = recent_authority;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::change_recovery_account( string owner, string new_recovery_account, bool broadcast ) {

    change_recovery_account_operation op;
   op.account_to_recover = owner;
   op.new_recovery_account = new_recovery_account;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

vector< database_api::api_owner_authority_history_object > alexandria_api::get_owner_history( string account )const
{
   return my->_remote_api->get_owner_history( account );
}

operation alexandria_api::update_account(
                                      string account_name,
                                      string json_meta,
                                      public_key_type owner,
                                      public_key_type active,
                                      public_key_type memo )const
{
   try
   {
      account_update_operation op;
      op.account = account_name;
      op.owner  = authority( 1, owner, 1 );
      op.active = authority( 1, active, 1);
      op.memo_key = memo;
      op.json_metadata = json_meta;

      return op;
   }
   FC_CAPTURE_AND_RETHROW( (account_name)(json_meta)(owner)(active)(memo) )
}

operation alexandria_api::update_witness( string witness_account_name,
                                               string url,
                                               public_key_type block_signing_key,
                                               const chain_properties& props)
{


   witness_update_operation op;

   optional< condenser_api::api_witness_object > wit = my->_remote_api->get_witness_by_account( witness_account_name );
   if( !wit.valid() )
   {
      op.url = url;
   }
   else
   {
      FC_ASSERT( wit->owner == witness_account_name );
      if( url != "" )
         op.url = url;
      else
         op.url = wit->url;
   }
   op.owner = witness_account_name;
   op.block_signing_key = block_signing_key;
   op.props = props;
   return op;
}

annotated_signed_transaction alexandria_api::stop_witness( string witness_account_name,
                                                         bool broadcast  )
{


   witness_stop_operation op;

   op.owner = witness_account_name;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::vote_for_witness(string voting_account, string witness_to_vote_for, bool approve, bool broadcast )
{ try {

    account_witness_vote_operation op;
    op.account = voting_account;
    op.witness = witness_to_vote_for;
    op.approve = approve;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (voting_account)(witness_to_vote_for)(approve)(broadcast) ) }



annotated_signed_transaction alexandria_api::transfer(string from, string to, asset amount, string memo, bool broadcast )
{ try {

   // check_memo( memcheck_memocheck_memocheck_memocheck_memoo, get_account( from ) );
    transfer_operation op;
    op.from = from;
    op.to = to;
    op.amount = amount;

    op.memo = /*get_encrypted_memo( from, to,*/ memo /*)*/;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(memo)(broadcast) ) }

annotated_signed_transaction alexandria_api::escrow_transfer(
      string from,
      string to,
      string agent,
      uint32_t escrow_id,
      asset steem_amount,
      asset fee,
      time_point_sec ratification_deadline,
      time_point_sec escrow_expiration,
      string json_meta,
      bool broadcast
   )
{

   escrow_transfer_operation op;
   op.from = from;
   op.to = to;
   op.agent = agent;
   op.escrow_id = escrow_id;
   op.steem_amount = steem_amount;
   op.escrow_fee = fee;
   op.ratification_deadline = ratification_deadline;
   op.escrow_expiration = escrow_expiration;
   op.json_meta = json_meta;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::escrow_approve(
      string from,
      string to,
      string agent,
      string who,
      uint32_t escrow_id,
      bool approve,
      bool broadcast
   )
{

   escrow_approve_operation op;
   op.from = from;
   op.to = to;
   op.agent = agent;
   op.who = who;
   op.escrow_id = escrow_id;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::escrow_dispute(
      string from,
      string to,
      string agent,
      string who,
      uint32_t escrow_id,
      bool broadcast
   )
{

   escrow_dispute_operation op;
   op.from = from;
   op.to = to;
   op.agent = agent;
   op.who = who;
   op.escrow_id = escrow_id;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::escrow_release(
   string from,
   string to,
   string agent,
   string who,
   string receiver,
   uint32_t escrow_id,
   asset steem_amount,
   bool broadcast
)
{

   escrow_release_operation op;
   op.from = from;
   op.to = to;
   op.agent = agent;
   op.who = who;
   op.receiver = receiver;
   op.escrow_id = escrow_id;
   op.steem_amount = steem_amount;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();
   return my->sign_transaction( tx, broadcast );
}


annotated_signed_transaction alexandria_api::transfer_to_vesting(string from, string to, asset amount, bool broadcast )
{

    transfer_to_vesting_operation op;
    op.from = from;
    op.to = (to == from ? "" : to);
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::withdraw_vesting(string from, asset vesting_shares, bool broadcast )
{

    withdraw_vesting_operation op;
    op.account = from;
    op.vesting_shares = vesting_shares;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction alexandria_api::publish_feed(string witness, price exchange_rate, bool broadcast )
{

    feed_publish_operation op;
    op.publisher     = witness;
    op.exchange_rate = exchange_rate;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}


condenser_api::state alexandria_api::get_state( string url ) {
   return my->_remote_api->get_state( url );
}

void alexandria_api::set_transaction_expiration(uint32_t seconds)
{
   my->set_transaction_expiration(seconds);
}

condenser_api::api_account_object alexandria_api::get_account( string account_name ) const
{
   return my->get_account( account_name );
}

annotated_signed_transaction alexandria_api::get_transaction( transaction_id_type id )const {
   return my->_remote_api->get_transaction( id );
}

operation
alexandria_api::delete_application(string author, authority active_auth, string app_name)
{
   try
   {
      application_delete_operation op;
      op.author = author;
      op.active = active_auth;
      op.name = app_name;

      return op;
   }
    FC_CAPTURE_AND_RETHROW( (author)(active_auth)(app_name))
}

operation alexandria_api::buy_application(string buyer, authority active_auth, int64_t app_id)
{
    try
    {
       buy_application_operation op;
       op.buyer = buyer;
       op.active = active_auth;
       op.app_id = app_id;
       return op;
    }
    FC_CAPTURE_AND_RETHROW( (buyer)(active_auth)(app_id))
}

operation alexandria_api::cancel_application_buying(string app_owner, string buyer, authority active_auth, int64_t app_id)
{
    try
    {
       cancel_application_buying_operation op;
       op.app_owner = app_owner;
       op.buyer = buyer;
       op.active = active_auth;
       op.app_id = app_id;
       return op;
    }
    FC_CAPTURE_AND_RETHROW( (app_owner)(buyer)(active_auth)(app_id))
}

vector<condenser_api::api_application_buying_object> alexandria_api::get_application_buyings(string name, string search_type, uint32_t count)
{
    try{
       return my->_remote_api->get_application_buyings(name, count, search_type);
    }FC_CAPTURE_AND_RETHROW((name)(search_type)(count))
}

operation
alexandria_api::update_application(string author, authority active_auth, string app_name, string new_author, string url,
                               string meta_data, uint8_t price_param) {
   try
   {


      application_update_operation op;
      op.author = author;
      op.active = active_auth;
      op.name = app_name;
      op.new_author= new_author;
      op.url = url;
      op.metadata = meta_data;
      op.price_param = price_param;

      return op;
   }
   FC_CAPTURE_AND_RETHROW( (author)(active_auth)(app_name)(new_author)(url)(meta_data)(price_param))
}

operation
alexandria_api::create_application(string author, authority active_auth, string app_name, string url, string meta_data,
                               uint8_t price_param) {
   try
   {


      application_create_operation op;
      op.author = author;
      op.active = active_auth;
      op.name = app_name;
      op.url = url;
      op.metadata = meta_data;
      op.price_param = price_param;
      return op;
   }
   FC_CAPTURE_AND_RETHROW( (author)(active_auth)(app_name)(url)(meta_data)(price_param))
}

operation alexandria_api::send_custom_json_document(uint32_t app_id, string from, vector<string> to, string json){
   try{

      custom_json_operation op;
      op.app_id = app_id;
      op.sender = from;
      for(const auto& r: to)
         op.recipients.insert(r);
      op.json = json;

      return op;

   }FC_CAPTURE_AND_RETHROW( (app_id)(from)(to)(json))
}

operation alexandria_api::send_custom_binary_document(uint32_t app_id, string from, vector<string> to, string data){
   try{

      custom_binary_operation op;
      op.app_id = app_id;
      op.sender = from;
      for(const auto& r: to)
         op.recipients.insert(r);
      op.data = fc::from_base58(data);
      return op;
   }FC_CAPTURE_AND_RETHROW( (app_id)(from)(to)(data))
}

map< uint64_t, condenser_api::api_received_object >  alexandria_api::get_received_documents(uint32_t app_id, string account_name, string search_type, string start, uint32_t count){
   try{
      return my->_remote_api->get_received_documents(app_id, account_name, search_type, start, count);
    }FC_CAPTURE_AND_RETHROW((app_id)(account_name)(search_type)(start)(count))
}

annotated_signed_transaction alexandria_api::broadcast_transaction(signed_transaction tx) const
{
   try {
      auto result = my->_remote_api->broadcast_transaction_synchronous( tx );
      annotated_signed_transaction rtrx(tx);
      rtrx.block_num = result.block_num;
      rtrx.transaction_num = result.trx_num;
      return rtrx;
    }FC_CAPTURE_AND_RETHROW((tx))
}

signed_transaction alexandria_api::create_transaction(operation op) const
{
    try{
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        return tx;
    }FC_CAPTURE_AND_RETHROW( (op))
}

digest_type alexandria_api::get_digest(signed_transaction tx) const
{
    try{
        return tx.sig_digest(my->steem_chain_id);;
    }FC_CAPTURE_AND_RETHROW( (tx))
}

operation alexandria_api::delete_account(string account_name) {
   try{
      account_delete_operation op;
      op.account = account_name;
      return op;
   }FC_CAPTURE_AND_RETHROW( (account_name))
}

} } // steem::wallet

