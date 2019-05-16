#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api.hpp>
#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_plugin.hpp>
#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_impl.hpp>

namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

account_bandwidth_api::account_bandwidth_api() :
   my(std::make_unique<account_bandwidth_api_impl>()) {
   JSON_RPC_REGISTER_API(SOPHIATX_ACCOUNT_BANDWIDTH_API_PLUGIN_NAME);
}

account_bandwidth_api::~account_bandwidth_api() {}

DEFINE_READ_APIS(account_bandwidth_api,
      (get_account_bandwidth)
)


} } }
