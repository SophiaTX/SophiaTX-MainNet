#pragma once
#include <appbase/application.hpp>

#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/json_rpc/json_rpc_plugin.hpp>

#define SOPHIATX_MM_PLUGIN_NAME "multiparty_messaging"

namespace sophiatx { namespace plugins { namespace multiparty_messaging_plugin {

namespace detail { class multiparty_messaging_plugin_impl; }

using namespace appbase;



/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class multiparty_messaging_plugin : public plugin< multiparty_messaging_plugin >
{
   public:
      multiparty_messaging_plugin();
      ~multiparty_messaging_plugin(){};

      APPBASE_PLUGIN_REQUIRES(
            (sophiatx::plugins::chain::chain_plugin)
            (sophiatx::plugins::json_rpc::json_rpc_plugin)
      )

      static const std::string& name() { static std::string name = SOPHIATX_MM_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      uint64_t app_id;
   private:
      std::shared_ptr< detail::multiparty_messaging_plugin_impl > my;
      std::shared_ptr< class multiparty_messaging_api > api;

};

} } } //sophiatx::plugins::multiparty_messaging_plugin

