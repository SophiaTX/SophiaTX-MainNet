#ifndef SOPHIATX_ACCOUNT_BANDWIDTH_API_PLUGIN_HPP
#define SOPHIATX_ACCOUNT_BANDWIDTH_API_PLUGIN_HPP

#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/json_rpc/json_rpc_plugin.hpp>
#include <appbase/application.hpp>


namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

using namespace appbase;

#define SOPHIATX_ACCOUNT_BANDWIDTH_API_PLUGIN_NAME "account_bandwidth_api"

class account_bandwidth_api_plugin : public plugin< account_bandwidth_api_plugin >
{
public:
   account_bandwidth_api_plugin();
   virtual ~account_bandwidth_api_plugin();

   APPBASE_PLUGIN_REQUIRES(
         (sophiatx::plugins::json_rpc::json_rpc_plugin)
         (sophiatx::plugins::chain::chain_plugin)
   )

   static const std::string& name() { static std::string name = SOPHIATX_ACCOUNT_BANDWIDTH_API_PLUGIN_NAME; return name; }

   virtual void set_program_options(options_description& cli, options_description& cfg) override;
   virtual void plugin_initialize(const variables_map& options) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class account_bandwidth_api > api;
};

} } } // sophiatx::plugins::account_bandwidth_api

#endif //SOPHIATX_ACCOUNT_BANDWIDTH_API_PLUGIN_HPP
