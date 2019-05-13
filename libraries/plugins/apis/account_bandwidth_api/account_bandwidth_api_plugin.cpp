#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_plugin.hpp>
#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api.hpp>

namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

account_bandwidth_api_plugin::account_bandwidth_api_plugin() {}
account_bandwidth_api_plugin::~account_bandwidth_api_plugin() {}

void account_bandwidth_api_plugin::set_program_options(
      options_description& cli,
      options_description& cfg )
{}

void account_bandwidth_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< account_bandwidth_api >();
}

void account_bandwidth_api_plugin::plugin_startup() {}

void account_bandwidth_api_plugin::plugin_shutdown() {}

} } } // sophiatx::plugins::account_bandwidth_api
