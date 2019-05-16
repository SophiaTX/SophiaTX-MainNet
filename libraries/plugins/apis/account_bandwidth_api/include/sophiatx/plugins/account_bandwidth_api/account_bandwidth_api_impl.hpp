#ifndef SOPHIATX_ACCOUNT_BANDWIDTH_API_IMPL_HPP
#define SOPHIATX_ACCOUNT_BANDWIDTH_API_IMPL_HPP

#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_args.hpp>

namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

class account_bandwidth_api_plugin;

class account_bandwidth_api_impl
{
public:
   account_bandwidth_api_impl();

   DECLARE_API_IMPL(
         (get_account_bandwidth)
   )

   std::shared_ptr<chain::database_interface> _db;
};


} } } // sophiatx::plugins::account_bandwidth_api

#endif //SOPHIATX_ACCOUNT_BANDWIDTH_API_IMPL_HPP
