#pragma once
#include <appbase/application.hpp>

#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/json_rpc/json_rpc_plugin.hpp>

#define SOPHIATX_TAT_PLUGIN_NAME "track_and_trace"

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

namespace detail { class track_and_trace_plugin_impl; }

using namespace appbase;



/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class track_and_trace_plugin : public plugin< track_and_trace_plugin >
{
   public:
      track_and_trace_plugin();
      virtual ~track_and_trace_plugin();

      APPBASE_PLUGIN_REQUIRES(
            (sophiatx::plugins::chain::chain_plugin)
            (sophiatx::plugins::json_rpc::json_rpc_plugin)
      )

      static const std::string& name() { static std::string name = SOPHIATX_TAT_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      uint64_t app_id;
   private:
      std::unique_ptr< detail::track_and_trace_plugin_impl > my;
      std::shared_ptr< class track_and_trace_api > api;

};

} } } //sophiatx::plugins::track_and_trace_plugin

