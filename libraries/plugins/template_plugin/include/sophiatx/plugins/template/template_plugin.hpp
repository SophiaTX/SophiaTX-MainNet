#pragma once
#include <appbase/application.hpp>

#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/json_rpc/json_rpc_plugin.hpp>

#define SOPHIATX_TEMPLATE_PLUGIN_NAME "template"

namespace sophiatx { namespace plugins { namespace template_plugin {

namespace detail { class template_plugin_impl; }

using namespace appbase;



/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class template_plugin : public plugin< template_plugin >
{
   public:
      template_plugin();
      virtual ~template_plugin();

      APPBASE_PLUGIN_REQUIRES(
            (sophiatx::plugins::chain::chain_plugin)
            (sophiatx::plugins::json_rpc::json_rpc_plugin)
      )

      static const std::string& name() { static std::string name = SOPHIATX_TEMPLATE_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;


   private:
      std::unique_ptr< detail::template_plugin_impl > my;
      std::shared_ptr< class template_api > api;
};

} } } //sophiatx::plugins::template_plugin

