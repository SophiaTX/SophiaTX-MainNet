#include <sophiatx/plugins/custom_api/custom_api_plugin.hpp>
#include <sophiatx/plugins/custom_api/custom_api.hpp>


namespace sophiatx { namespace plugins { namespace custom {

custom_api_plugin::custom_api_plugin() {}
custom_api_plugin::~custom_api_plugin() {}

void custom_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void custom_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< custom_api >();
}

void custom_api_plugin::plugin_startup() {}
void custom_api_plugin::plugin_shutdown() {}

} } } // namespace
