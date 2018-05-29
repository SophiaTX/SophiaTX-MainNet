#pragma once
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define STEEM_CUSTOM_API_PLUGIN_NAME "custom_api"


namespace steem { namespace plugins { namespace custom {

using namespace appbase;

class custom_api_plugin : public plugin< custom_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
         //(steem::plugins::custom::custom_plugin)
         (steem::plugins::json_rpc::json_rpc_plugin)
   )

   custom_api_plugin();
   virtual ~custom_api_plugin();

   static const std::string& name() { static std::string name = STEEM_CUSTOM_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class custom_api > api;
};

} } } // steem::plugins::custom
