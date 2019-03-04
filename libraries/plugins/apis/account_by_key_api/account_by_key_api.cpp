#include <sophiatx/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <sophiatx/plugins/account_by_key_api/account_by_key_api.hpp>

#include <sophiatx/plugins/account_by_key/account_by_key_objects.hpp>

namespace sophiatx { namespace plugins { namespace account_by_key {

namespace detail {

class account_by_key_api_impl
{
   public:
      account_by_key_api_impl( account_by_key_api_plugin& plugin ) : _app(plugin.app()), _db( plugin.app()->get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ) {}

      DECLARE_API_IMPL( (get_key_references)  )

   appbase::application* _app;
   std::shared_ptr<database_interface> _db;
};

DEFINE_API_IMPL(account_by_key_api_impl, get_key_references)
{
   get_key_references_return final_result;
   final_result.accounts.reserve( args.keys.size() );

   const auto& key_idx = _db->get_index< account_by_key::key_lookup_index >().indices().get< account_by_key::by_key >();

   for( auto& key : args.keys )
   {
      std::vector< sophiatx::protocol::account_name_type > result;
      auto lookup_itr = key_idx.lower_bound( key );

      while( lookup_itr != key_idx.end() && lookup_itr->key == key )
      {
         result.push_back( lookup_itr->account );
         ++lookup_itr;
      }

      final_result.accounts.emplace_back( std::move( result ) );
   }

   return final_result;
}

} // detail

account_by_key_api::account_by_key_api(account_by_key_api_plugin& plugin): my( new detail::account_by_key_api_impl(plugin) )
{
   JSON_RPC_REGISTER_API( SOPHIATX_ACCOUNT_BY_KEY_API_PLUGIN_NAME, plugin.app() );
}

account_by_key_api::~account_by_key_api()
{
   JSON_RPC_DEREGISTER_API( SOPHIATX_ACCOUNT_BY_KEY_API_PLUGIN_NAME, my->_app );
}

DEFINE_READ_APIS( account_by_key_api, (get_key_references) )

} } } // sophiatx::plugins::account_by_key
