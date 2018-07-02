#include <sophiatx/utilities/git_revision.hpp>
#include <sophiatx/utilities/key_conversion.hpp>

#include <sophiatx/protocol/base.hpp>
#include <sophiatx/alexandria/lib_alexandria.hpp>
#include <sophiatx/alexandria/api_documentation.hpp>
#include <sophiatx/alexandria/reflect_util.hpp>


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

namespace sophiatx { namespace alexandria {

using sophiatx::plugins::condenser_api::legacy_asset;

namespace detail {

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

public:
   alexandria_api& self;
   alexandria_api_impl( alexandria_api& s, fc::api< remote_node_api > rapi )
      : self( s ), _remote_api( rapi )
   {}
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
      return result;
   }

   variant_object about()
   {
      string client_version( sophiatx::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::mutable_variant_object result;
      result["blockchain_version"]       = SOPHIATX_BLOCKCHAIN_VERSION;
      result["client_version"]           = client_version;
      result["sophiatx_revision"]        = sophiatx::utilities::git_revision_sha;
      result["sophiatx_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( sophiatx::utilities::git_revision_unix_timestamp ) );
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
         result["server_sophiatx_revision"] = v.sophiatx_revision;
         result["server_fc_revision"] = v.fc_revision;
         result["chain_id"] = v.chain_id;
         _chain_id = fc::sha256(v.chain_id);
      }
      catch( fc::exception& )
      {
         result["server"] = "could not retrieve server version information";
      }

      return result;
   }

   condenser_api::api_account_object get_account( string account_name ) const
   {
      string decoded_name = make_random_fixed_string(account_name);
      auto accounts = _remote_api->get_accounts( { account_name, decoded_name } );
      FC_ASSERT( !accounts.empty(), "Unknown account" );
      return accounts.front();
   }

   operation set_voting_proxy(string account_to_modify, string proxy)
   { try {
      account_witness_proxy_operation op;
      op.account = account_to_modify;
      op.proxy = proxy;

      return op;
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(proxy) ) }

   optional< condenser_api::api_witness_object > get_witness( string owner_account )
   {
      return _remote_api->get_witness_by_account( owner_account );
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
         asset total_sophiatx;
         asset total_vest(0, VESTS_SYMBOL );
         for( const auto& a : accounts ) {
            total_sophiatx += a.balance.to_asset();
            total_vest  += a.vesting_shares.to_asset();
            out << std::left << std::setw( 17 ) << std::string(a.name)
                << std::right << std::setw(18) << fc::variant(a.balance).as_string() <<" "
                << std::right << std::setw(26) << fc::variant(a.vesting_shares).as_string() <<" \n";
         }
         out << "-------------------------------------------------------------------------\n";
            out << std::left << std::setw( 17 ) << "TOTAL"
                << std::right << std::setw(18) << legacy_asset::from_asset(total_sophiatx).to_string() <<" "
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

   fc::api< remote_node_api >              _remote_api;
   uint32_t                                _tx_expiration_seconds = 30;
   chain_id_type                           _chain_id;

#ifdef __unix__
   mode_t                  _old_umask;
#endif
};

} } } // sophiatx::alexandria::detail



namespace sophiatx { namespace alexandria {

alexandria_api::alexandria_api(fc::api< remote_node_api > rapi)
   : my(new detail::alexandria_api_impl(*this, rapi))
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

variant alexandria_api::info()
{
   return my->info();
}

variant_object alexandria_api::about()
{
    return my->about();
}

vector< account_name_type > alexandria_api::list_witnesses(const string& lowerbound, uint32_t limit)
{
   return my->_remote_api->lookup_witness_accounts( lowerbound, limit );
}

optional< condenser_api::api_witness_object > alexandria_api::get_witness(string owner_account)
{
   return my->get_witness(owner_account);
}

operation alexandria_api::set_voting_proxy(string account_to_modify, string voting_account)
{ return my->set_voting_proxy(account_to_modify, voting_account); }

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

operation alexandria_api::create_account( string creator,
                                      string name_seed,
                                      string json_meta,
                                      public_key_type owner,
                                      public_key_type active,
                                      public_key_type memo)const
{ try {

   account_create_operation op;
   op.creator = creator;
   op.name_seed = name_seed;
   op.owner = authority( 1, owner, 1 );
   op.active = authority( 1, active, 1 );
   op.memo_key = memo;
   op.json_metadata = json_meta;
   op.fee = my->_remote_api->get_chain_properties().account_creation_fee * asset( 1, SOPHIATX_SYMBOL );

   return op;
} FC_CAPTURE_AND_RETHROW( (creator)(name_seed)(json_meta)(owner)(active)(memo)) }

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

operation alexandria_api::stop_witness( string witness_account_name )
{
   witness_stop_operation op;

   op.owner = witness_account_name;
   return op;
}

operation alexandria_api::vote_for_witness(string voting_account, string witness_to_vote_for, bool approve )
{ try {

    account_witness_vote_operation op;
    op.account = voting_account;
    op.witness = witness_to_vote_for;
    op.approve = approve;

   return op;
} FC_CAPTURE_AND_RETHROW( (voting_account)(witness_to_vote_for)(approve)) }



operation alexandria_api::transfer(string from, string to, asset amount, string memo)
{ try {

    transfer_operation op;
    op.from = from;
    op.to = to;
    op.amount = amount;

    op.memo =  memo;
    return op;
} FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(memo) ) }

operation alexandria_api::transfer_to_vesting(string from, string to, asset amount)
{
    transfer_to_vesting_operation op;
    op.from = from;
    op.to = (to == from ? "" : to);
    op.amount = amount;

    return op;
}

operation alexandria_api::withdraw_vesting(string from, asset vesting_shares)
{

    withdraw_vesting_operation op;
    op.account = from;
    op.vesting_shares = vesting_shares;

    return op;
}

annotated_signed_transaction alexandria_api::get_transaction( transaction_id_type id )const {
   return my->_remote_api->get_transaction( id );
}

condenser_api::api_account_object alexandria_api::get_account( string account_name ) const
{
   return my->get_account( account_name );
}

operation
alexandria_api::delete_application(string author, string app_name)
{
   try
   {
      application_delete_operation op;
      op.author = author;
      op.name = app_name;

      return op;
   }
    FC_CAPTURE_AND_RETHROW( (author)(app_name))
}

operation alexandria_api::buy_application(string buyer, int64_t app_id)
{
    try
    {
       buy_application_operation op;
       op.buyer = buyer;
       op.app_id = app_id;
       return op;
    }
    FC_CAPTURE_AND_RETHROW( (buyer)(app_id))
}

operation alexandria_api::cancel_application_buying(string app_owner, string buyer, int64_t app_id)
{
    try
    {
       cancel_application_buying_operation op;
       op.app_owner = app_owner;
       op.buyer = buyer;
       op.app_id = app_id;
       return op;
    }
    FC_CAPTURE_AND_RETHROW( (app_owner)(buyer)(app_id))
}

vector<condenser_api::api_application_buying_object> alexandria_api::get_application_buyings(string name, string search_type, uint32_t count)
{
    try{
       return my->_remote_api->get_application_buyings(name, count, search_type);
    }FC_CAPTURE_AND_RETHROW((name)(search_type)(count))
}

operation
alexandria_api::update_application(string author, string app_name, string new_author, string url,
                               string meta_data, uint8_t price_param) {
   try
   {


      application_update_operation op;
      op.author = author;
      op.name = app_name;
      op.new_author= new_author;
      op.url = url;
      op.metadata = meta_data;
      op.price_param = price_param;

      return op;
   }
   FC_CAPTURE_AND_RETHROW( (author)(app_name)(new_author)(url)(meta_data)(price_param))
}

operation
alexandria_api::create_application(string author, string app_name, string url, string meta_data,
                               uint8_t price_param) {
   try
   {


      application_create_operation op;
      op.author = author;
      op.name = app_name;
      op.url = url;
      op.metadata = meta_data;
      op.price_param = price_param;
      return op;
   }
   FC_CAPTURE_AND_RETHROW( (author)(app_name)(url)(meta_data)(price_param))
}

operation alexandria_api::make_custom_json_operation(uint32_t app_id, string from, vector<string> to, string json){
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

operation alexandria_api::make_custom_binary_operation(uint32_t app_id, string from, vector<string> to, string data){
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

signed_transaction alexandria_api::create_transaction(vector<operation> op_vec) const
{
    try{
         //set fees first
        signed_transaction tx;
        class op_visitor{
        public:
           op_visitor(){};
           typedef void result_type;
           result_type operator()( base_operation& bop){
              if(bop.has_special_fee())
                 return;
              asset req_fee = bop.get_required_fee(SOPHIATX_SYMBOL);
              bop.fee = req_fee;
           };
        };
        op_visitor op_v;

        for(auto& op : op_vec)
        {
            op.visit(op_v);
            tx.operations.push_back(op);
        }

        auto dyn_props = my->_remote_api->get_dynamic_global_properties();
        tx.set_reference_block( dyn_props.head_block_id );
        tx.set_expiration( dyn_props.time + fc::seconds(my->_tx_expiration_seconds) );

        tx.validate();
        return tx;
    }FC_CAPTURE_AND_RETHROW( (op_vec))
}

signed_transaction alexandria_api::create_simple_transaction(operation op) const
{
    try{
         //set fees first
        signed_transaction tx;
        class op_visitor{
        public:
           op_visitor(){};
           typedef void result_type;
           result_type operator()( base_operation& bop){
              if(bop.has_special_fee())
                 return;
              asset req_fee = bop.get_required_fee(SOPHIATX_SYMBOL);
              bop.fee = req_fee;
           };
        };
        op_visitor op_v;

        op.visit(op_v);
        tx.operations.push_back(op);

        auto dyn_props = my->_remote_api->get_dynamic_global_properties();
        tx.set_reference_block( dyn_props.head_block_id );
        tx.set_expiration( dyn_props.time + fc::seconds(my->_tx_expiration_seconds) );

        tx.validate();
        return tx;
    }FC_CAPTURE_AND_RETHROW( (op))
}

operation alexandria_api::delete_account(string account_name) {
   try{
      account_delete_operation op;
      op.account = account_name;
      return op;
   }FC_CAPTURE_AND_RETHROW( (account_name))
}

vector<condenser_api::api_application_object> alexandria_api::get_applications(vector<string> names) {
   try{
      return my->_remote_api->get_applications(names);
   }FC_CAPTURE_AND_RETHROW((names))
}

digest_type alexandria_api::get_transaction_digest(signed_transaction tx) {
   try{
      if(my->_chain_id == fc::sha256())
      {
         auto v = my->_remote_api->get_version();
         my->_chain_id = fc::sha256(v.chain_id);
      }
      return tx.sig_digest(my->_chain_id);
   }FC_CAPTURE_AND_RETHROW((tx))
}

signed_transaction alexandria_api::add_signature(signed_transaction tx, fc::ecc::compact_signature signature) const {
   try{
      tx.signatures.push_back(signature);
      return  tx;
   }FC_CAPTURE_AND_RETHROW((tx)(signature))
}

fc::ecc::compact_signature alexandria_api::sign_digest(digest_type digest, string pk) const {
   try{
      auto priv_key = *sophiatx::utilities::wif_to_key(pk);
      return priv_key.sign_compact(digest);
   }FC_CAPTURE_AND_RETHROW((digest)(pk))
}

annotated_signed_transaction alexandria_api::send_and_sign_operation(operation op, string pk) {
   try{
      auto tx = create_simple_transaction(op);
      broadcast_transaction(add_signature(tx, sign_digest(get_transaction_digest(tx), pk)));
   }FC_CAPTURE_AND_RETHROW((op)(pk))
}

annotated_signed_transaction alexandria_api::send_and_sign_transaction(signed_transaction tx, string pk){
   try{
      broadcast_transaction(add_signature(tx, sign_digest(get_transaction_digest(tx), pk)));
   }FC_CAPTURE_AND_RETHROW((tx)(pk))
}

bool alexandria_api::verify_signature(digest_type digest, public_key_type pub_key,
                                      fc::ecc::compact_signature signature) const {
   try{
      if(pub_key == fc::ecc::public_key(signature, digest)) {
         return true;
      }
      return  false;
   }FC_CAPTURE_AND_RETHROW((digest)(pub_key)(signature))
}

key_pair alexandria_api::generate_key_pair() const {
   private_key_type priv_key = fc::ecc::private_key::generate();
   key_pair kp;
   kp.pub_key = priv_key.get_public_key();
   kp.wif_priv_key = key_to_wif(priv_key);
   return kp;
}

key_pair alexandria_api::generate_key_pair_from_brain_key(string brain_key) const {
   fc::sha512 h = fc::sha512::hash(brain_key + " 0");
   auto priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   key_pair kp;
   kp.pub_key = priv_key.get_public_key();
   kp.wif_priv_key = key_to_wif(priv_key);
   return kp;
}

public_key_type alexandria_api::get_public_key(string private_key) const {
   try{
      auto priv_key = *sophiatx::utilities::wif_to_key(private_key);
      return priv_key.get_public_key();
   }FC_CAPTURE_AND_RETHROW((private_key))
}

std::vector<char> alexandria_api::from_base58(string data) const {
   return fc::from_base58(data);
}

string alexandria_api::to_base58(std::vector<char> data) const {
   return fc::to_base58(data);
}

string alexandria_api::encrypt_data(string data, public_key_type public_key, string private_key) const {
   try {
      memo_data m;

      auto priv_key = *sophiatx::utilities::wif_to_key(private_key);

      m.nonce = fc::time_point::now().time_since_epoch().count();

      auto shared_secret = priv_key.get_shared_secret( public_key );

      fc::sha512::encoder enc;
      fc::raw::pack( enc, m.nonce );
      fc::raw::pack( enc, shared_secret );
      auto encrypt_key = enc.result();

      m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(data) );
      m.check = fc::sha256::hash( encrypt_key )._hash[0];
      return string(m);
   } FC_CAPTURE_AND_RETHROW((data)(public_key)(private_key))
}

string alexandria_api::decrypt_data(string data, public_key_type public_key, string private_key) const {
   try {
      auto m = memo_data::from_string( data );

      FC_ASSERT(m , "Can not parse input!");

      fc::sha512 shared_secret;
      auto priv_key = *sophiatx::utilities::wif_to_key(private_key);

      shared_secret = priv_key.get_shared_secret(public_key);

      fc::sha512::encoder enc;
      fc::raw::pack(enc, m->nonce);
      fc::raw::pack(enc, shared_secret);
      auto encryption_key = enc.result();

      uint64_t check = fc::sha256::hash(encryption_key)._hash[ 0 ];

      FC_ASSERT(check == m->check, "Checksum does not match!");

      vector<char> decrypted = fc::aes_decrypt(encryption_key, m->encrypted);
      return fc::raw::unpack_from_vector<std::string>(decrypted);

   } FC_CAPTURE_AND_RETHROW((data)(public_key)(private_key))
}

bool alexandria_api::account_exist(string account_name) const {
   try {
      auto accounts = my->_remote_api->get_accounts( { account_name } );

      if( !accounts.empty())
      {
         return true;
      }
      else{
         return false;
      }
   } FC_CAPTURE_AND_RETHROW((account_name))
}


authority alexandria_api::get_active_authority(string account_name) const {
   try {
      auto account =  get_account(account_name);
      return account.active;
   } FC_CAPTURE_AND_RETHROW((account_name))
}

authority alexandria_api::get_owner_authority(string account_name) const {
   try {
      auto account =  get_account(account_name);
      return account.owner;
   } FC_CAPTURE_AND_RETHROW((account_name))
}

public_key_type alexandria_api::get_memo_key(string account_name) const {
   try {
      auto account =  get_account(account_name);
      return account.memo_key;
   } FC_CAPTURE_AND_RETHROW((account_name))
}

int64_t alexandria_api::get_account_balance(string account_name) const {
   try {
      auto account =  get_account(account_name);
      return account.balance.amount.value;
   } FC_CAPTURE_AND_RETHROW((account_name))
}

int64_t alexandria_api::get_vesting_balance(string account_name) const {
   try {
      auto account =  get_account(account_name);
      return account.vesting_shares.amount.value;
   } FC_CAPTURE_AND_RETHROW((account_name))
}

authority alexandria_api::create_simple_authority(public_key_type pub_key) const {
   return authority(1, pub_key, 1);
}

authority alexandria_api::create_simple_managed_authority(string managing_account) const {
   return authority(1, managing_account, 1);
}

map< uint32_t, condenser_api::api_operation_object > alexandria_api::get_account_history( string account, uint32_t from, uint32_t limit ) {
   auto result = my->_remote_api->get_account_history( account, from, limit );
   for( auto& item : result ) {
      if( item.second.op.which() == condenser_api::legacy_operation::tag<condenser_api::legacy_transfer_operation>::value )
         auto& top = item.second.op.get<condenser_api::legacy_transfer_operation>();
   }
   return result;
}

authority
alexandria_api::create_simple_multisig_authority(vector<public_key_type> pub_keys, uint32_t required_signatures) const {
   authority auth;
   auth.weight_threshold = required_signatures;
   for(const auto& key : pub_keys)
   {
      auth.add_authority(key, 1);
   }
   return auth;
}

authority alexandria_api::create_simple_multisig_managed_authority(vector<string> managing_accounts,
                                                                   uint32_t required_signatures) const {
   authority auth;
   auth.weight_threshold = required_signatures;
   for(const auto& account : managing_accounts)
   {
      auth.add_authority(account, 1);
   }
   return auth;
}

} } // sophiatx::alexandria

