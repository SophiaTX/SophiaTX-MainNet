#include <sophiatx/plugins/subscribe_api/subscribe_api_plugin.hpp>
#include <sophiatx/plugins/subscribe_api/subscribe_api.hpp>


namespace sophiatx { namespace plugins { namespace subscribe {

subscribe_api_plugin::subscribe_api_plugin() {}
subscribe_api_plugin::~subscribe_api_plugin() {}

void subscribe_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void subscribe_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< subscribe_api >();
}

void subscribe_api_plugin::plugin_startup()
{
   api->api_startup();
}
void subscribe_api_plugin::plugin_shutdown() {}

} } } // namespace