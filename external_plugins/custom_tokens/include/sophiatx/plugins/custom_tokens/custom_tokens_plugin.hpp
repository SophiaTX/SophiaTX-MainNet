#pragma once

#include <appbase/application.hpp>

#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/json_rpc/json_rpc_plugin.hpp>

#define SOPHIATX_CT_PLUGIN_NAME "custom_tokens"

namespace sophiatx {
namespace plugins {
namespace custom_tokens {

using namespace appbase;

namespace detail {
class custom_tokens_api_impl;
class custom_tokens_plugin_impl;
}

/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class custom_tokens_plugin : public plugin<custom_tokens_plugin> {
   friend class detail::custom_tokens_api_impl;
   friend class detail::custom_tokens_plugin_impl;

public:
   custom_tokens_plugin();

   ~custom_tokens_plugin() {}

   APPBASE_PLUGIN_REQUIRES(
         (sophiatx::plugins::chain::chain_plugin)
               (sophiatx::plugins::json_rpc::json_rpc_plugin)
   )

   static const std::string &name() {
      static std::string name = SOPHIATX_CT_PLUGIN_NAME;
      return name;
   }

    virtual void set_program_options(
         options_description &cli,
         options_description &cfg);

   virtual void plugin_initialize(const variables_map &options) override;

   virtual void plugin_startup() override;

   virtual void plugin_shutdown() override;

   uint64_t app_id_;
   std::shared_ptr<class custom_tokens_api> api_;
private:
   std::shared_ptr<detail::custom_tokens_plugin_impl> my_;
};

}
}
} //sophiatx::plugins::custom_tokens

