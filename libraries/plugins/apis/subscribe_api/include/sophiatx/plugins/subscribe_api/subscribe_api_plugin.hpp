#pragma once
#include <sophiatx/plugins/json_rpc/json_rpc_plugin.hpp>
#include <sophiatx/plugins/custom_api/custom_api_plugin.hpp>

#include <appbase/application.hpp>

#define SOPHIATX_SUBSCRIBE_API_PLUGIN_NAME "subscribe_api"


namespace sophiatx { namespace plugins { namespace subscribe {

using namespace appbase;

class subscribe_api_plugin : public plugin< subscribe_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
         (sophiatx::plugins::json_rpc::json_rpc_plugin)
         (sophiatx::plugins::custom::custom_api_plugin)
   )

   subscribe_api_plugin();
   virtual ~subscribe_api_plugin();

   static const std::string& name() { static std::string name = SOPHIATX_SUBSCRIBE_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class subscribe_api > api;
};

} } } // sophiatx::plugins::subscribe
