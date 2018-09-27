
#include <sophiatx/plugins/alexandria_api/alexandria_api_impl.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_objects.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

alexandria_api_impl::alexandria_api_impl()
      : _db(appbase::app().get_plugin<sophiatx::plugins::chain::chain_plugin>().db()) {}

alexandria_api_impl::~alexandria_api_impl() {}

chain::database &alexandria_api_impl::get_db() const {
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




/************************************************/
/*********  Api methods implementations *********/
/************************************************/

DEFINE_API_IMPL( alexandria_api_impl, list_witnesses )
{
   checkApiEnabled(_database_api);

   alexandria_api::list_witnesses_return result;
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
         return list_witnesses_by_vote_return();

      start_key.push_back( fc::variant( start.witnesses[0].votes ) );
      start_key.push_back( fc::variant( start.witnesses[0].owner ) );
   }

   auto limit = args.limit;
   auto witnesses = _database_api->list_witnesses( { fc::variant( start_key ), limit, database_api::by_vote_name_reverse } ).witnesses;

   list_witnesses_by_vote_return result;

   for( auto& w : witnesses ) {
      result.witnesses.push_back( api_witness_object( w ) );
   }

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_witness )
{
   checkApiEnabled(_database_api);

   alexandria_api::get_witness_return result;
   auto witnesses = _database_api->find_witnesses( { { args.name } } ).witnesses;

   if( witnesses.size() ) {
      result.witness = api_witness_object( witnesses[0] );
   }

   return result;
}

DEFINE_API_IMPL(alexandria_api_impl, get_block) {
   checkApiEnabled(_block_api);

   alexandria_api::get_block_return result;
   auto block = _block_api->get_block( { args.block_num } ).block;

   if( block) {
      result.block = api_signed_block(*block);
   }

   return result;
}


DEFINE_API_IMPL( alexandria_api_impl, get_ops_in_block )
{
   checkApiEnabled(_account_history_api);

   auto ops = _account_history_api->get_ops_in_block( { args.block_num, args.only_virtual } ).ops;
   alexandria_api::get_ops_in_block_return result;

   api_operation l_op;
   api_operation_conversion_visitor visitor( l_op );

   for( auto& op_obj : ops )
   {
      if( op_obj.op.visit( visitor) )
      {
         result.ops.push_back( api_operation_object( op_obj, visitor.l_op ) );
      }
   }

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_feed_history )
{
   checkApiEnabled(_database_api);

   alexandria_api::get_feed_history_return result;
   result.history = _database_api->get_feed_history( { args.symbol } );

   return result;
}

DEFINE_API_IMPL( alexandria_api_impl, get_application_buyings )
{
   checkApiEnabled(_database_api);
   // TODO: check if FC_CAPTURE_AND_RETHROW should be used as in alexandria_lib
   return _database_api->get_application_buyings( { args.start, args.limit, args.search_type } );
}





} } } // sophiatx::plugins::alexandria_api

