#include <sophiatx/plugins/alexandria_api/alexandria_api.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_plugin.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

alexandria_api_plugin::alexandria_api_plugin() {}
alexandria_api_plugin::~alexandria_api_plugin() {}

void alexandria_api_plugin::set_program_options(
   options_description& cli,
   options_description& cfg ) {}

void alexandria_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< alexandria_api >();
}

void alexandria_api_plugin::plugin_startup() {
   api->init();
}

void alexandria_api_plugin::plugin_shutdown() {}

} } } // sophiatx::plugins::alexandria_api
