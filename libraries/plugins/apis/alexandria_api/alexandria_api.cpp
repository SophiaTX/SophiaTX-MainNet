#include <appbase/application.hpp>

#include <sophiatx/plugins/alexandria_api/alexandria_api.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_plugin.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_impl.hpp>
#include <sophiatx/plugins/block_api/block_api_plugin.hpp>
#include <sophiatx/plugins/block_api/block_api.hpp>

#include <sophiatx/protocol/get_config.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

alexandria_api::alexandria_api()
      : my( new alexandria_api_impl() )
{
   JSON_RPC_REGISTER_API( SOPHIATX_ALEXANDRIA_API_PLUGIN_NAME );
}

alexandria_api::~alexandria_api() {}

void alexandria_api::init() {
   auto block = appbase::app().find_plugin< block_api::block_api_plugin >();
   if( block != nullptr )
   {
      my->set_block_api(block->api);
   }
}

DEFINE_READ_APIS(alexandria_api,
                 (get_block)
)

} } } // sophiatx::plugins::alexandria_api
