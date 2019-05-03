#pragma once
#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

using namespace appbase;

#define SOPHIATX_ALEXANDRIA_API_PLUGIN_NAME "alexandria_api"

class alexandria_api_plugin : public plugin< alexandria_api_plugin >
{
   public:
      alexandria_api_plugin();
      virtual ~alexandria_api_plugin();

      APPBASE_PLUGIN_REQUIRES(
         (sophiatx::plugins::json_rpc::json_rpc_plugin)
         (sophiatx::plugins::chain::chain_plugin)
      )

      static const std::string& name() { static std::string name = SOPHIATX_ALEXANDRIA_API_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      void plugin_initialize( const variables_map& options ) override;
      void plugin_startup() override;
      void plugin_shutdown() override;

      std::shared_ptr< class alexandria_api > api;
};

} } } // sophiatx::plugins::alexandria_api
