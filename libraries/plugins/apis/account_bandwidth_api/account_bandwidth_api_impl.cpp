#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_impl.hpp>
#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_args.hpp>
#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_plugin.hpp>

namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

account_bandwidth_api_impl::account_bandwidth_api_impl() :
      _db(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db()) {}


DEFINE_API_IMPL(account_bandwidth_api_impl, get_account_bandwidth) {
   get_account_bandwidth_return result;

   auto band = _db->find<chain::account_bandwidth_object, chain::by_account>(args.account);
   if (band != nullptr) {
      result.bandwidth = *band;
   }

   return result;
}

} } }