#ifndef SOPHIATX_CHAIN_PLUGIN_LITE_HPP
#define SOPHIATX_CHAIN_PLUGIN_LITE_HPP

#include <sophiatx/plugins/chain/chain_plugin.hpp>

namespace sophiatx {
namespace plugins {
namespace chain {

using namespace appbase;


class chain_plugin_lite : public chain_plugin {
public:

   chain_plugin_lite();

   virtual ~chain_plugin_lite();

   void set_program_options(options_description &cli, options_description &cfg) override;

   void plugin_initialize(const variables_map &options) override;

   void plugin_startup() override;

   void plugin_shutdown() override;

private:
   string ws_endpoint;
   uint64_t app_id;
};

}
}
} // sophiatx::plugins::chain

#endif //SOPHIATX_CHAIN_PLUGIN_LITE_HPP
