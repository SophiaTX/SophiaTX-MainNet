#include <sophiatx/plugins/chain_api/chain_api_plugin.hpp>
#include <sophiatx/plugins/chain_api/chain_api.hpp>

namespace sophiatx { namespace plugins { namespace chain {

namespace detail {

class chain_api_impl
{
public:
   chain_api_impl(chain_api_plugin& plugin) : _app(plugin.app()), _chain( plugin.app()->get_plugin<chain_plugin>() ) {}

      DECLARE_API_IMPL(
         (push_block)
         (push_transaction) )

   appbase::application* _app;
private:
   chain_plugin& _chain;
};

DEFINE_API_IMPL( chain_api_impl, push_block )
{
   push_block_return result;

   result.success = false;

   try
   {
      _chain.accept_block(args.block, args.currently_syncing, chain::database_interface::skip_nothing);
      result.success = true;
   }
   catch (const fc::exception& e)
   {
      result.error = e.to_detail_string();
   }
   catch (const std::exception& e)
   {
      result.error = e.what();
   }
   catch (...)
   {
      result.error = "uknown error";
   }

   return result;
}

DEFINE_API_IMPL( chain_api_impl, push_transaction )
{
   push_transaction_return result;

   result.success = false;

   try
   {
      _chain.accept_transaction(args);
      result.success = true;
   }
   catch (const fc::exception& e)
   {
      result.error = e.to_detail_string();
   }
   catch (const std::exception& e)
   {
      result.error = e.what();
   }
   catch (...)
   {
      result.error = "uknown error";
   }

   return result;
}

} // detail

chain_api::chain_api(chain_api_plugin& plugin): my( new detail::chain_api_impl(plugin) )
{
   JSON_RPC_REGISTER_API( SOPHIATX_CHAIN_API_PLUGIN_NAME, plugin.app() );
}

chain_api::~chain_api()
{
   JSON_RPC_DEREGISTER_API( SOPHIATX_CHAIN_API_PLUGIN_NAME, my->_app );
}

DEFINE_LOCKLESS_APIS( chain_api,
   (push_block)
   (push_transaction)
)

} } } //sophiatx::plugins::chain
