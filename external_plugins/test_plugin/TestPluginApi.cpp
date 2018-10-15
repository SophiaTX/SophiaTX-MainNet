#include <sophiatx/plugins/test_plugin/TestPlugin.hpp>
#include <sophiatx/plugins/test_plugin/TestPluginApi.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>

namespace sophiatx { namespace plugins { namespace test_plugin {

namespace detail {

class TestPluginApiImpl
{
   public:
   //TestPluginApiImpl() : _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ) {}
   //TestPluginApiImpl(chain::database& db) : _db( db) {}
   TestPluginApiImpl() {}

   //chain::database& _db;
   int lentak;

   //std::shared_ptr<chain::database> _db;
};

} // detail

TestPluginApi::TestPluginApi()
{
   std::cout << "TestPluginApi()" << std::endl;
   my = std::make_unique<detail::TestPluginApiImpl>();

   //JSON_RPC_REGISTER_API( SOPHIATX_TAT_PLUGIN_NAME );
}

TestPluginApi::~TestPluginApi() {
   std::cout << "~TestPluginApi()" << std::endl;
}

//DEFINE_READ_APIS( track_and_trace_api, (get_current_holder)(get_holdings)(get_tracked_object_history)(get_transfer_requests)(get_item_details) )

} } }
