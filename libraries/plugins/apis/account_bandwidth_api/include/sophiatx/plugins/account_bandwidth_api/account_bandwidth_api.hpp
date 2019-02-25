#ifndef SOPHIATX_ACCOUNT_BANDWIDTH_API_HPP
#define SOPHIATX_ACCOUNT_BANDWIDTH_API_HPP

#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/account_bandwidth_api/account_bandwidth_api_args.hpp>

namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

class account_bandwidth_api_plugin;
class account_bandwidth_api_impl;

class account_bandwidth_api
{
public:
   account_bandwidth_api(account_bandwidth_api_plugin& plugin);
   ~account_bandwidth_api();

   DECLARE_API(
      /**
       * @brief Returns account bandwidth information
       */
      (get_account_bandwidth)
   )

private:
   std::unique_ptr< account_bandwidth_api_impl > my;
};

} } } // sophiatx::plugins::account_bandwidth_api

#endif //SOPHIATX_ACCOUNT_BANDWIDTH_API_HPP
