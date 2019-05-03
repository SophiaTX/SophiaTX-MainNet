
#include <sophiatx/plugins/alexandria_api/alexandria_api_impl.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_objects.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/utilities/key_conversion.hpp>
#include <sophiatx/utilities/git_revision.hpp>
#include <fc/git_revision.hpp>
#include <fc/crypto/aes.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

alexandria_api_impl::alexandria_api_impl()
      : _db(appbase::app().get_plugin<sophiatx::plugins::chain::chain_plugin>().db()) {}

alexandria_api_impl::~alexandria_api_impl() {}

const std::shared_ptr<chain::database_interface> &alexandria_api_impl::get_db() const {
   return _db;
}

const shared_ptr<block_api::block_api> &alexandria_api_impl::get_block_api() const {
   return _block_api;
}

void alexandria_api_impl::set_block_api(const shared_ptr<block_api::block_api> &block_api) {
   _block_api = block_api;
}

const shared_ptr<database_api::database_api> &alexandria_api_impl::get_database_api() const {
   return _database_api;
}

void alexandria_api_impl::set_database_api(const shared_ptr<database_api::database_api> &database_api) {
   _database_api = database_api;
}

const shared_ptr<account_history::account_history_api> &alexandria_api_impl::get_account_history_api() const {
   return _account_history_api;
}

void alexandria_api_impl::set_account_history_api(const shared_ptr<account_history::account_history_api> &account_history_api) {
   _account_history_api = account_history_api;
}

const shared_ptr<account_by_key::account_by_key_api> &alexandria_api_impl::get_account_by_key_api() const {
   return _account_by_key_api;
}

void alexandria_api_impl::set_account_by_key_api(const shared_ptr<account_by_key::account_by_key_api> &account_by_key_api) {
   _account_by_key_api = account_by_key_api;
}

const shared_ptr<network_broadcast_api::network_broadcast_api> &alexandria_api_impl::get_network_broadcast_api() const {
   return _network_broadcast_api;
}

void alexandria_api_impl::set_network_broadcast_api(const shared_ptr<network_broadcast_api::network_broadcast_api> &network_broadcast_api) {
   _network_broadcast_api = network_broadcast_api;
}

const shared_ptr<witness::witness_api> &alexandria_api_impl::get_witness_api() const {
   return _witness_api;
}

void alexandria_api_impl::set_witness_api(const shared_ptr<witness::witness_api> &witness_api) {
   _witness_api = witness_api;
}

const shared_ptr<custom::custom_api> &alexandria_api_impl::get_custom_api() const {
   return _custom_api;
}

void alexandria_api_impl::set_custom_api(const shared_ptr<custom::custom_api> &custom_api) {
   _custom_api = custom_api;
}

const shared_ptr<subscribe::subscribe_api> &alexandria_api_impl::get_subscribe_api() const {
   return _subscribe_api;
}

void alexandria_api_impl::set_subscribe_api(const shared_ptr<subscribe::subscribe_api> &subscribe_api) {
   _subscribe_api = subscribe_api;
}

const chain_id_type &alexandria_api_impl::get_chain_id() {
   if(_chain_id == fc::sha256())
   {
      checkApiEnabled(_database_api);
      set_chain_id(_database_api->get_dynamic_global_properties({}).chain_id);
   }

   return _chain_id;
}

void alexandria_api_impl::set_chain_id(const chain_id_type & chain_id) {
   _chain_id = chain_id;
}

/************************************************/
/*********  Api methods implementations *********/
/************************************************/

DEFINE_API_IMPL(alexandria_api_impl, get_block) {
   checkApiEnabled(_block_api);

   get_block_return result;
   auto block = _block_api->get_block( { args.num } ).block;

   if( block) {
      result.block = api_signed_block(*block);
   }

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_ops_in_block )
{
   checkApiEnabled(_account_history_api);

   auto ops = _account_history_api->get_ops_in_block( { args.block_num, args.only_virtual } ).ops;
   get_ops_in_block_return result;

   api_operation l_op;
   api_operation_conversion_visitor visitor( l_op );

   for( auto& op_obj : ops )
   {
      if( op_obj.op.visit( visitor) )
      {
         result.ops_in_block.push_back( api_operation_object( op_obj, visitor.l_op ) );
      }
   }

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_active_witnesses )
{
   checkApiEnabled(_database_api);

   get_active_witnesses_return result;
   result.active_witnesses = _database_api->get_active_witnesses( {} ).witnesses;

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, info)
{
   checkApiEnabled(_database_api);

   auto dynamic_props = get_dynamic_global_properties( {} ).properties;
   fc::mutable_variant_object info_data(fc::variant(dynamic_props).get_object());

   info_data["witness_majority_version"] = fc::string( _database_api->get_witness_schedule( {} ).majority_version );
   info_data["hardfork_version"] = fc::string( _database_api->get_hardfork_properties( {} ).current_hardfork_version );
   //info_data["head_block_id"] = dynamic_props.head_block_id;
   info_data["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                       time_point_sec(time_point::now()),
                                                                       " old");
   info_data["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
   info_data["median_sbd1_price"] = _database_api->get_current_price_feed( { SBD1_SYMBOL } );
   info_data["median_sbd2_price"] = _database_api->get_current_price_feed( { SBD2_SYMBOL } );
   info_data["median_sbd3_price"] = _database_api->get_current_price_feed( { SBD3_SYMBOL } );
   info_data["median_sbd4_price"] = _database_api->get_current_price_feed( { SBD4_SYMBOL } );
   info_data["median_sbd5_price"] = _database_api->get_current_price_feed( { SBD5_SYMBOL } );
   info_data["account_creation_fee"] = _database_api->get_witness_schedule( {} ).median_props.account_creation_fee;

   info_return result;
   result.info = std::move(info_data);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, about)
{
   checkApiEnabled(_database_api);

   string client_version( sophiatx::utilities::git_revision_description );
   const size_t pos = client_version.find( '/' );
   if( pos != string::npos && client_version.size() > pos )
      client_version = client_version.substr( pos + 1 );

   fc::mutable_variant_object about_data;
   about_data["blockchain_version"]       = SOPHIATX_BLOCKCHAIN_VERSION;
   about_data["client_version"]           = client_version;
   about_data["sophiatx_revision"]        = sophiatx::utilities::git_revision_sha;
   about_data["sophiatx_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( sophiatx::utilities::git_revision_unix_timestamp ) );
   about_data["fc_revision"]              = fc::git_revision_sha;
   about_data["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
   about_data["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
   about_data["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
   about_data["openssl_version"]          = OPENSSL_VERSION_TEXT;

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
   about_data["build"] = os + " " + bitness;

   try
   {
      get_version_info info = get_version({}).version_info;

      about_data["server_blockchain_version"] = info.blockchain_version;
      about_data["server_sophiatx_revision"] = info.sophiatx_revision;
      about_data["server_fc_revision"] = info.fc_revision;
      about_data["chain_id"] = info.chain_id;
   }
   catch( fc::exception& )
   {
      about_data["server"] = "could not retrieve server version information";
   }

   about_return result;
   result.about = std::move(about_data);

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, list_witnesses )
{
   checkApiEnabled(_database_api);

   list_witnesses_return result;
   auto witnesses = _database_api->list_witnesses( { args.start, args.limit, database_api::by_name } ).witnesses;

   for( auto& w : witnesses )
   {
      result.witnesses.push_back( w.owner );
   }

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, list_witnesses_by_vote )
{
   checkApiEnabled(_database_api);

   list_witnesses_by_vote_return result;

   account_name_type start_name = args.name;
   vector< fc::variant > start_key;

   if( start_name == account_name_type() )
   {
      start_key.push_back( fc::variant( std::numeric_limits< int64_t >::max() ) );
      start_key.push_back( fc::variant( account_name_type() ) );
   }
   else
   {
      auto start = _database_api->list_witnesses( { args.name, 1, database_api::by_name_reverse } );

      if( start.witnesses.size() == 0 )
         return result;

      start_key.push_back( fc::variant( start.witnesses[0].votes ) );
      start_key.push_back( fc::variant( start.witnesses[0].owner ) );
   }

   auto limit = args.limit;
   auto witnesses = _database_api->list_witnesses( { fc::variant( start_key ), limit, database_api::by_vote_name } ).witnesses;

   for( auto& w : witnesses ) {
      result.witnesses_by_vote.push_back( api_witness_object( w ) );
   }

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_witness )
{
   checkApiEnabled(_database_api);

   get_witness_return result;
   auto witnesses = _database_api->find_witnesses( { { args.owner_account } } ).witnesses;

   if( witnesses.size() ) {
      result.witness = api_witness_object( witnesses[0] );
   }

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, set_voting_proxy)
{
   set_voting_proxy_return result;

   account_witness_proxy_operation op;
   op.account = args.account_to_modify;
   op.proxy = args.proxy;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_feed_history )
{
   checkApiEnabled(_database_api);

   get_feed_history_return result;
   result.feed_history = _database_api->get_feed_history( { asset_symbol_type::from_string(args.symbol) } );

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, create_account)
{
   checkApiEnabled(_database_api);

   account_create_operation op;
   op.creator = args.creator;
   op.name_seed = args.name_seed;
   op.owner = authority( 1, args.owner, 1 );
   op.active = authority( 1, args.active, 1 );
   op.memo_key = args.memo;
   op.json_metadata = args.json_meta;
   op.fee = _database_api->get_witness_schedule( {} ).median_props.account_creation_fee * asset( 1, SOPHIATX_SYMBOL );

   create_account_return result;
   result.op = std::move(op);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_owner_history)
{
   checkApiEnabled(_database_api);

   get_owner_history_return result;
   result.owner_history = _database_api->find_owner_histories( { args.account } ).owner_auths;

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, update_account)
{
   auto accounts = get_accounts( { { args.account_name } } ).accounts;
   FC_ASSERT( !accounts.empty(), "Account does not exist" );

   update_account_return result;

   account_update_operation op;
   op.account = args.account_name;
   op.owner  = authority( 1, args.owner, 1 );
   op.active = authority( 1, args.active, 1);
   op.memo_key = args.memo;
   op.json_metadata = args.json_meta;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, update_account_auth)
{
   auto accounts = get_accounts( { { args.account_name } } ).accounts;
   FC_ASSERT( !accounts.empty(), "Account does not exist" );

   update_account_auth_return result;

   account_update_operation op;
   op.account = args.account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = accounts[0].json_metadata;

   if( args.new_authority.is_impossible() )
   {
      if ( args.type == owner )
      {
         FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
      }

      wlog( "Authority is now impossible." );
   }

   switch( args.type )
   {
      case( owner ):
         op.owner = args.new_authority;
         break;
      case( active ):
         op.active = args.new_authority;
         break;

   }

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, update_witness)
{
   checkApiEnabled(_database_api);

   witness_update_operation op;

   auto witnesses = _database_api->find_witnesses( { { args.witness_account_name } }).witnesses;
   if (witnesses.size() > 0)
   {
      FC_ASSERT( witnesses[0].owner == args.witness_account_name );
      if( args.url != "" )
         op.url = args.url;
      else
         op.url = witnesses[0].url;
   }
   else
   {
      op.url = args.url;
   }
   op.owner = args.witness_account_name;
   op.block_signing_key = args.block_signing_key;
   op.props = args.props;


   update_witness_return result;
   result.op = std::move(op);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, stop_witness)
{
   stop_witness_return result;

   witness_stop_operation op;
   op.owner = args.witness_account_name;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, vote_for_witness)
{
   vote_for_witness_return result;

   account_witness_vote_operation op;
   op.account = args.voting_account;
   op.witness = args.witness_to_vote_for;
   op.approve = args.approve;

   result.op = std::move(op);
   return result;
}



DEFINE_API_IMPL(alexandria_api_impl, transfer)
{
   transfer_return result;

    transfer_operation op;
    op.from = args.from;
    op.to = args.to;
    op.amount = args.amount;
    op.memo =  args.memo;

    result.op = std::move(op);
    return result;
}

DEFINE_API_IMPL(alexandria_api_impl, transfer_to_vesting)
{
   transfer_to_vesting_return result;

    transfer_to_vesting_operation op;
    op.from = args.from;
    op.to = (args.to == args.from ? "" : args.to);
    op.amount = args.amount;

    result.op = std::move(op);
    return result;
}

DEFINE_API_IMPL(alexandria_api_impl, withdraw_vesting)
{
   withdraw_vesting_return result;

   withdraw_vesting_operation op;
   op.account = args.from;
   op.vesting_shares = args.vesting_shares;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_transaction) {
   checkApiEnabled(_account_history_api);
   FC_ASSERT( args.tx_id != sophiatx::protocol::transaction_id_type(), "Invalid tx_id parameter" );

   get_transaction_return result;
   result.tx = _account_history_api->get_transaction( { args.tx_id } );

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_account)
{
   string decoded_name = make_random_fixed_string(args.account_name);
   auto accounts = get_accounts( { { args.account_name, decoded_name } } ).accounts;
   FC_ASSERT( !accounts.empty(), "Unknown account" );

   std::vector<api_account_object>  accounts_ret(std::make_move_iterator(accounts.begin()),
                                                 std::make_move_iterator(accounts.end()));

   get_account_return result;
   result.account = std::move(accounts_ret);

   return result;

}

DEFINE_API_IMPL(alexandria_api_impl, get_accounts) {
   const auto& idx  = _db->get_index< chain::account_index >().indices().get< chain::by_name >();
   const auto& vidx = _db->get_index< chain::witness_vote_index >().indices().get< chain::by_account_witness >();
   vector< extended_account > accounts;
   accounts.reserve(args.account_names.size());

   for( const auto& name: args.account_names )
   {
      auto itr = idx.find( name );
      if ( itr != idx.end() )
      {
         accounts.emplace_back( extended_account( database_api::api_account_object( *itr, _db ) ) );

         auto vitr = vidx.lower_bound( boost::make_tuple( itr->name, account_name_type() ) );
         while( vitr != vidx.end() && vitr->account == itr->name ) {
            accounts.back().witness_votes.insert( _db->get< chain::witness_object, chain::by_name >( vitr->witness ).owner );
            ++vitr;
         }
      }
   }

   get_accounts_return result;
   result.accounts = accounts;

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, delete_application)
{
   delete_application_return result;

   application_delete_operation op;
   op.author = args.author;
   op.name = args.app_name;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, buy_application)
{
   buy_application_return result;

   buy_application_operation op;
   op.buyer = args.buyer;
   op.app_id = args.app_id;

   result.op = std::move(op);
   return result;

}

DEFINE_API_IMPL(alexandria_api_impl, cancel_application_buying)
{
   cancel_application_buying_return result;

   cancel_application_buying_operation op;
   op.app_owner = args.app_owner;
   op.buyer = args.buyer;
   op.app_id = args.app_id;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_application_buyings )
{
   checkApiEnabled(_database_api);

   get_application_buyings_return result;
   result.application_buyings = _database_api->get_application_buyings( { args.name, args.count, args.search_type } ).application_buyings;

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, update_application)
{
   update_application_return result;

   application_update_operation op;
   op.author = args.author;
   op.name = args.app_name;
   op.new_author = args.new_author;
   op.url = args.url;
   op.metadata = args.meta_data;
   op.price_param = args.price_param;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, create_application)
{
   create_application_return result;

   application_create_operation op;
   op.author = args.author;
   op.name = args.app_name;
   op.url = args.url;
   op.metadata = args.meta_data;
   op.price_param = args.price_param;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, make_custom_json_operation)
{
   make_custom_json_operation_return result;

   custom_json_operation op;
   op.app_id = args.app_id;
   op.sender = args.from;
   for(const auto& r: args.to)
      op.recipients.insert(r);
   op.json = args.json;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, make_custom_binary_operation)
{
   make_custom_binary_operation_return result;

   custom_binary_operation op;
   op.app_id = args.app_id;
   op.sender = args.from;
   for(const auto& r: args.to)
      op.recipients.insert(r);
   auto out = fc::base64_decode(args.data);
   std::copy(out.begin(), out.end(), std::back_inserter(op.data));

   result.op = std::move(op);
   return result;
}

#ifdef ABAP_INTERFACE
DEFINE_API_IMPL(alexandria_api_impl, get_received_documents){
   checkApiEnabled(_custom_api);

   get_received_documents_return result;

   typedef std::map<uint64_t, api_received_object> ObjectMap;
   ObjectMap from_api = _custom_api->list_received_documents( { args.app_id, args.account_name, args.search_type, args.start, args.count } );
   std::transform( from_api.begin(), from_api.end(),
                std::back_inserter(result.received_documents),
                boost::bind(&ObjectMap::value_type::second,_1) );


   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_account_history) {
   checkApiEnabled(_account_history_api);

   get_account_history_return result;

   typedef std::map<uint32_t, api_operation_object> ObjectMap;
   ObjectMap from_api;

   auto history = _account_history_api->get_account_history( { args.account, args.start, args.limit, true } ).history;

   api_operation l_op;
   api_operation_conversion_visitor visitor( l_op );

   for( auto& entry : history )
   {
      if( entry.second.op.visit( visitor ) )
      {
         from_api.emplace( entry.first, api_operation_object( entry.second, visitor.l_op ) );
      }
   }


   std::transform( from_api.begin(), from_api.end(),
                   std::back_inserter(result.account_history),
                   boost::bind(&ObjectMap::value_type::second,_1) );

   return result;
}
#else
DEFINE_API_IMPL(alexandria_api_impl, get_received_documents) {
   checkApiEnabled(_custom_api);

   get_received_documents_return result;
   result.received_documents = _custom_api->list_received_documents( { args.app_id, args.account_name, args.search_type, args.start, args.count } );

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_account_history) {
   checkApiEnabled(_account_history_api);

   auto history = _account_history_api->get_account_history( { args.account, args.start, args.limit, true } ).history;
   get_account_history_return result;

   api_operation l_op;
   api_operation_conversion_visitor visitor( l_op );

   for( auto& entry : history )
   {
      if( entry.second.op.visit( visitor ) )
      {
         result.account_history.emplace( entry.first, api_operation_object( entry.second, visitor.l_op ) );
      }
   }

   return result;
}
#endif


DEFINE_API_IMPL(alexandria_api_impl, broadcast_transaction)
{
   checkApiEnabled(_network_broadcast_api);

   broadcast_transaction_return result;
   auto broadcast_result = _network_broadcast_api->broadcast_transaction_synchronous( { args.tx } );

   annotated_signed_transaction rtrx(args.tx);
   rtrx.block_num = broadcast_result.block_num;
   rtrx.transaction_num = broadcast_result.trx_num;

   result.tx = std::move(rtrx);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, create_transaction)
{
   checkApiEnabled(_database_api);
   checkApiEnabled(_witness_api);

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

   vector<operation> op_vec = args.op_vec;
   for(auto& op : op_vec)
   {
      op.visit(op_v);
      tx.operations.push_back(op);
   }

   auto dyn_props = get_dynamic_global_properties( {} ).properties;

   tx.set_reference_block( dyn_props.head_block_id );
   tx.set_expiration( dyn_props.time + fc::seconds(_tx_expiration_seconds) );
   tx.validate();

   create_transaction_return result;
   result.tx = std::move(tx);

   return result;
}


DEFINE_API_IMPL(alexandria_api_impl, create_simple_transaction)
{
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

   operation op = args.op;
   op.visit(op_v);
   tx.operations.push_back(op);

   auto dyn_props = get_dynamic_global_properties( {} ).properties;

   tx.set_reference_block( dyn_props.head_block_id );
   tx.set_expiration( dyn_props.time + fc::seconds(_tx_expiration_seconds) );
   tx.validate();

   create_simple_transaction_return result;
   result.simple_tx = std::move(tx);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, delete_account) {
   delete_account_return result;

   account_delete_operation op;
   op.account = args.account_name;

   result.op = std::move(op);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_transaction_id)
{
   get_transaction_id_return result;
   result.tx_id = args.tx.id();

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_applications )
{
   vector< string > app_names = args.names;

   get_applications_return result;
   result.applications.reserve( app_names.size() );

   for( auto& name : app_names )
   {
      auto itr = _db->find< chain::application_object, chain::by_name >( name );

      if( itr )
      {
         result.applications.push_back( api_application_object( database_api::api_application_object( *itr ) ) );
      }
   }
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_applications_by_ids)
{

   vector< uint32_t > app_ids = args.ids;

   get_applications_by_ids_return result;
   result.applications.reserve( app_ids.size() );

   for( auto& id : app_ids )
   {
      auto itr = _db->find< chain::application_object, chain::by_id >( id );

      if( itr )
      {
         result.applications.push_back( api_application_object( database_api::api_application_object( *itr ) ) );
      }
   }
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_transaction_digest)
{
   get_transaction_digest_return result;
   result.tx_digest = args.tx.sig_digest(get_chain_id());

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, add_signature)
{
   add_signature_return result;

   signed_transaction trx = args.tx;
   trx.signatures.push_back(args.signature);

   result.signed_tx = std::move(trx);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, add_fee)
{
   class op_visitor{
   public:
      op_visitor(asset _fee):fee(_fee){};
      asset fee;
      typedef void result_type;
      result_type operator()( base_operation& bop){
         bop.fee = fee;
      };
   };


   op_visitor op_v(args.fee);
   operation ret = args.op;
   ret.visit(op_v);

   add_fee_return result;
   result.op = std::move(ret);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, sign_digest)
{
   auto priv_key = sophiatx::utilities::wif_to_key(args.pk);
   FC_ASSERT( priv_key.valid(), "Malformed private key" );

   sign_digest_return result;
   result.signed_digest = priv_key->sign_compact(args.digest);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, send_and_sign_operation)
{
   send_and_sign_operation_return result;

   signed_transaction tx                       = create_simple_transaction( { args.op } ).simple_tx;
   digest_type                       tx_digest = get_transaction_digest( { tx } ).tx_digest;
   fc::ecc::compact_signature	signed_tx_digest = sign_digest( { tx_digest, args.pk } ).signed_digest;
   signed_transaction                signed_tx = add_signature( { tx, signed_tx_digest } ).signed_tx;
   result.signed_tx                            = broadcast_transaction( { signed_tx } ).tx;

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, send_and_sign_transaction)
{
   send_and_sign_transaction_return result;

   digest_type                       tx_digest = get_transaction_digest( { args.tx } ).tx_digest;
   fc::ecc::compact_signature	signed_tx_digest = sign_digest( { tx_digest, args.pk } ).signed_digest;
   signed_transaction                signed_tx = add_signature( { args.tx, signed_tx_digest } ).signed_tx;
   result.signed_tx                            = broadcast_transaction( { signed_tx } ).tx;

   return result;
}


DEFINE_API_IMPL(alexandria_api_impl, verify_signature)
{
   verify_signature_return result;
   try {
      result.signature_valid = (args.pub_key == fc::ecc::public_key::recover_key(args.signature, args.digest,
                                                                                 _db->has_hardfork(
                                                                                       SOPHIATX_HARDFORK_1_1)
                                                                                 ? fc::ecc::bip_0062
                                                                                 : fc::ecc::fc_canonical)) ? true
                                                                                                           : false;
      return result;
   }catch(...){
      result.signature_valid = false;
      return result;
   }
}

DEFINE_API_IMPL(alexandria_api_impl, generate_key_pair)
{
   generate_key_pair_return result;

   private_key_type priv_key = fc::ecc::private_key::generate();
   key_pair_st kp;
   kp.pub_key = priv_key.get_public_key();
   kp.wif_priv_key = utilities::key_to_wif(priv_key);

   result.key_pair = std::move(kp);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, generate_key_pair_from_brain_key)
{
   generate_key_pair_from_brain_key_return result;

   fc::sha512 h = fc::sha512::hash(args.brain_key + " 0");
   auto priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   key_pair_st kp;
   kp.pub_key = priv_key.get_public_key();
   kp.wif_priv_key = utilities::key_to_wif(priv_key);

   result.key_pair = std::move(kp);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_public_key)
{
   auto priv_key = sophiatx::utilities::wif_to_key(args.private_key);
   FC_ASSERT( priv_key.valid(), "Malformed private key" );

   get_public_key_return result;
   result.public_key = priv_key->get_public_key();

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, from_base64)
{
   from_base64_return result;
   result.str = fc::base64_decode(args.data);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, to_base64)
{
   to_base64_return result;
   result.base64_str = fc::base64_encode(args.data);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, encrypt_data)
{
   memo_data m;

   auto priv_key = utilities::wif_to_key(args.private_key);
   FC_ASSERT( priv_key.valid(), "Malformed private key" );

   m.nonce = fc::time_point::now().time_since_epoch().count();

   auto shared_secret = priv_key->get_shared_secret( args.public_key );

   fc::sha512::encoder enc;
   fc::raw::pack( enc, m.nonce );
   fc::raw::pack( enc, shared_secret );
   auto encrypt_key = enc.result();

   m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(args.data) );
   m.check = fc::sha256::hash( encrypt_key )._hash[0];

   encrypt_data_return result;
   result.encrypted_data = string(m);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, decrypt_data)
{
   auto m = memo_data::from_string( args.data );
   FC_ASSERT(m , "Can not parse input!");

   fc::sha512 shared_secret;
   auto priv_key = sophiatx::utilities::wif_to_key(args.private_key);
   FC_ASSERT( priv_key.valid(), "Malformed private key" );

   shared_secret = priv_key->get_shared_secret(args.public_key);

   fc::sha512::encoder enc;
   fc::raw::pack(enc, m->nonce);
   fc::raw::pack(enc, shared_secret);
   auto encryption_key = enc.result();

   uint64_t check = fc::sha256::hash(encryption_key)._hash[ 0 ];
   FC_ASSERT(check == m->check, "Checksum does not match!");

   vector<char> decrypted = fc::aes_decrypt(encryption_key, m->encrypted);

   decrypt_data_return result;
   result.decrypted_data = fc::raw::unpack_from_vector<std::string>(decrypted, 0);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, account_exist) 
{
   std::string decoded_name = make_random_fixed_string(args.account_name);
   auto accounts = get_accounts( { { args.account_name, decoded_name } } ).accounts;

   account_exist_return result;
   result.account_exist = accounts.empty() ? false : true;
   
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_active_authority)
{
   get_active_authority_return result;
   
   auto accounts = get_account( { args.account_name } ).account;
   if(accounts.size() == 1) {
      result.active_authority = accounts.front().active;
      return result;
   }
   
   for(const auto& acc: accounts) {
      if(acc.name == account_name_type(args.account_name)) {
         result.active_authority =  acc.active;
         return result;
      }
   }
   
   FC_ASSERT(false, "Account name does not exist!");
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_owner_authority)
{
   get_owner_authority_return result;
   
   auto accounts = get_account( { args.account_name } ).account;
   if(accounts.size() == 1) {
      result.owner_authority = accounts.front().owner;
      return result;
   }
   
   for(const auto& acc: accounts) {
      if(acc.name == account_name_type(args.account_name)) {
         result.owner_authority = acc.owner;
         return result;
      }
   }
   
   FC_ASSERT(false, "Account name does not exist!");
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_memo_key)
{
   get_memo_key_return result;
   
   auto accounts = get_account( { args.account_name } ).account;
   if(accounts.size() == 1) {
      result.memo_key = accounts.front().memo_key;
      return result;
   }
   
   for(const auto& acc: accounts) {
      if(acc.name == account_name_type(args.account_name)) {
         result.memo_key = acc.memo_key;
         return result;
      }
   }
   
   FC_ASSERT(false, "Account name does not exist!");
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_account_balance)
{
   get_account_balance_return result;
   
   auto accounts =  get_account( { args.account_name } ).account;
   if(accounts.size() == 1) {
      result.account_balance = accounts.front().balance.amount.value;
      return result;
   }
   
   for(const auto& acc: accounts) {
      if(acc.name == account_name_type(args.account_name)) {
         result.account_balance = acc.balance.amount.value;
         return result;
      }
   }
   
   FC_ASSERT(false, "Account name does not exist!");
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_vesting_balance)
{
   get_vesting_balance_return result;

   auto accounts =  get_account( { args.account_name } ).account;
   if(accounts.size() == 1) {
      result.vesting_balance = accounts.front().vesting_shares.amount.value;
      return result;
   }
   
   for(const auto& acc: accounts) {
      if(acc.name == account_name_type(args.account_name)) {
         result.vesting_balance = acc.vesting_shares.amount.value;
         return result;
      }
   }

   FC_ASSERT(false, "Account name does not exist!");
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, create_simple_authority)
{
   create_simple_authority_return result;

   result.simple_authority = authority(1, args.pub_key, 1);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, create_simple_managed_authority)
{
   create_simple_managed_authority_return result;

   string decoded_name = make_random_fixed_string(args.managing_account);
   result.simple_managed_authority = authority(1, decoded_name, 1);
   
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, create_simple_multisig_authority)
{
   create_simple_multisig_authority_return result;

   result.simple_multisig_authority.weight_threshold = args.required_signatures;
   for(const auto& key : args.pub_keys)
   {
      result.simple_multisig_authority.add_authority(key, 1);
   }
   
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, create_simple_multisig_managed_authority)
{
   create_simple_multisig_managed_authority_return result;

   result.simple_multisig_managed_authority.weight_threshold = args.required_signatures;
   for(const auto& account : args.managing_accounts)
   {
      result.simple_multisig_managed_authority.add_authority(account, 1);
   }
   
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_account_name_from_seed)
{
   get_account_name_from_seed_return result;

   result.account_name = account_name_type(make_random_fixed_string(args.seed));
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, calculate_fee)
{
   checkApiEnabled(_database_api);

   calculate_fee_return result;
   api_chain_properties props = _database_api->get_witness_schedule( {} ).median_props;
   
   if(args.op.which() == operation::tag<account_create_operation>::value){
      result.fee = props.account_creation_fee;
      return result;
   }

   class op_visitor{
   public:
      op_visitor(asset_symbol_type _symbol):symbol(_symbol){};
      asset_symbol_type symbol;
      typedef asset result_type;
      result_type operator()(const base_operation& bop){
         if(bop.has_special_fee())
            return asset(0, SOPHIATX_SYMBOL);
         asset req_fee = bop.get_required_fee(symbol);
         FC_ASSERT(symbol == req_fee.symbol, "fee cannot be paid in with symbol ${s}", ("s", bop.fee.symbol));
         return req_fee;
      };
   };
   op_visitor op_v(args.symbol);

   result.fee = args.op.visit(op_v);
   //check if the symbol has current price feed
   FC_ASSERT(result.fee.symbol == SOPHIATX_SYMBOL || fiat_to_sphtx( { result.fee } ).sphtx.symbol == SOPHIATX_SYMBOL, "no current feed for this symbol");

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, fiat_to_sphtx)
{
   checkApiEnabled(_database_api);
   
   fiat_to_sphtx_return result;
   
   auto price = api_feed_history_object(_database_api->get_feed_history( { args.fiat.symbol } ) ).current_median_price;
   
   if(price.base.amount == 0 || price.quote.amount == 0) {
      result.sphtx = args.fiat;
      return result;
   }
   
   if(price.base.symbol != args.fiat.symbol && price.quote.symbol != args.fiat.symbol) {
      result.sphtx = args.fiat;
      return result;
   }


   result.sphtx = args.fiat * price;
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_required_signatures)
{
   flat_set<account_name_type>   req_active_approvals;
   flat_set<account_name_type>   req_owner_approvals;
   vector<authority>  other_auths;

   args.tx.get_required_authorities( req_active_approvals, req_owner_approvals, other_auths );

   vector<account_name_type> v_approving_account_names;
   std::merge(req_active_approvals.begin(), req_active_approvals.end(),
              req_owner_approvals.begin() , req_owner_approvals.end(),
              std::back_inserter( v_approving_account_names ) );

   auto approving_account_objects = get_accounts( { v_approving_account_names } ).accounts;

   FC_ASSERT( approving_account_objects.size() == v_approving_account_names.size(), "", ("aco.size:", approving_account_objects.size())("acn",v_approving_account_names.size()) );

   flat_map<string, alexandria_api::api_account_object> approving_account_lut;
   size_t i = 0;
   for( const optional<alexandria_api::api_account_object>& approving_acct : approving_account_objects )
   {
      if( !approving_acct.valid() )
      {
         wlog( "operation_get_required_auths said approval of non-existing account ${name} was needed",
               ("name", v_approving_account_names[i]) );
         i++;
         continue;
      }
      approving_account_lut[ approving_acct->name ] =  *approving_acct;
      i++;
   }

   set<public_key_type> approving_key_set;
   for( account_name_type& acct_name : req_active_approvals )
   {
      const auto it = approving_account_lut.find( acct_name );
      if( it == approving_account_lut.end() )
         continue;
      const alexandria_api::api_account_object& acct = it->second;
      vector<public_key_type> v_approving_keys = acct.active.get_keys();
      for( const public_key_type& approving_key : v_approving_keys )
      {
         approving_key_set.insert( approving_key );
      }
   }

   for( const account_name_type& acct_name : req_owner_approvals )
   {
      const auto it = approving_account_lut.find( acct_name );
      if( it == approving_account_lut.end() )
         continue;
      const alexandria_api::api_account_object& acct = it->second;
      vector<public_key_type> v_approving_keys = acct.owner.get_keys();
      for( const public_key_type& approving_key : v_approving_keys )
      {
         approving_key_set.insert( approving_key );
      }
   }

   for( const authority& a : other_auths )
   {
      for( const auto& k : a.key_auths )
      {
         approving_key_set.insert( k.first );
      }
   }

   get_required_signatures_return result;
   result.required_signatures = std::move(approving_key_set);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, custom_object_subscription)
{
   custom_object_subscription_return result;
   result.subscription =  _subscribe_api->custom_object_subscription( { args.return_id, args.app_id, args.account_name, args.search_type, args.start } );

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, sponsor_account_fees)
{
   sponsor_account_fees_return result;

   sponsor_fees_operation op;
   op.sponsor = args.sponsoring_account;
   op.sponsored = args.sponsored_account;
   op.is_sponsoring = args.is_sponsoring;

   result.op = std::move(op);
   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_version)
{
   get_version_return result;
   result.version_info = get_version_info (
      fc::string( SOPHIATX_BLOCKCHAIN_VERSION ),
      fc::string( sophiatx::utilities::git_revision_sha ),
      fc::string( fc::git_revision_sha ),
      fc::string( get_chain_id()  )
      );

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_dynamic_global_properties)
{
   checkApiEnabled(_database_api);
   checkApiEnabled(_block_api);

   extended_dynamic_global_properties props = _database_api->get_dynamic_global_properties( {} );
   props.average_block_size = _block_api->get_average_block_size( {} );


   get_dynamic_global_properties_return result;
   result.properties = std::move(props);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_key_references)
{
   checkApiEnabled(_account_by_key_api);

   get_key_references_return result;
   result.accounts = _account_by_key_api->get_key_references( { args.keys } ).accounts;

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_witness_schedule_object)
{
   checkApiEnabled(_database_api);

   api_witness_schedule_object props = _database_api->get_witness_schedule( {} );

   get_witness_schedule_object_return result;
   result.schedule_obj = std::move(props);

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_hardfork_property_object)
{
   checkApiEnabled(_database_api);

   alexandria_api::api_hardfork_property_object props = _database_api->get_hardfork_properties( {} );
   get_hardfork_property_object_return result;

   result.hf_obj = std::move(props);

   return result;
}




} } } // sophiatx::plugins::alexandria_api
