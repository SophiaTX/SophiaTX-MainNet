#include <appbase/application.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <sophiatx/manifest/plugins.hpp>


#include <sophiatx/plugins/witness/witness_plugin.hpp>

#include <sophiatx/plugins/account_history_api/account_history_api_plugin.hpp>

#include <sophiatx/plugins/subscribe_api/subscribe_api_plugin.hpp>

#include <sophiatx/plugins/custom_api/custom_api_plugin.hpp>

#include <sophiatx/plugins/witness_api/witness_api_plugin.hpp>

#include <sophiatx/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>

#include <sophiatx/plugins/database_api/database_api_plugin.hpp>

#include <sophiatx/plugins/alexandria_api/alexandria_api_plugin.hpp>

#include <sophiatx/plugins/chain_api/chain_api_plugin.hpp>

#include <sophiatx/plugins/account_by_key_api/account_by_key_api_plugin.hpp>

#include <sophiatx/plugins/block_api/block_api_plugin.hpp>

#include <sophiatx/plugins/debug_node_api/debug_node_api_plugin.hpp>

#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_plugin.hpp>

#include <sophiatx/plugins/account_history/account_history_plugin.hpp>

#include <sophiatx/plugins/chain/chain_plugin.hpp>

#include <sophiatx/plugins/debug_node/debug_node_plugin.hpp>

#include <sophiatx/plugins/webserver/webserver_plugin.hpp>

#include <sophiatx/plugins/template/template_plugin.hpp>

#include <sophiatx/plugins/p2p/p2p_plugin.hpp>

#include <sophiatx/plugins/block_log_info/block_log_info_plugin.hpp>

#include <sophiatx/plugins/account_by_key/account_by_key_plugin.hpp>


namespace sophiatx { namespace plugins {

void register_plugins()
{
   appbase::app().register_plugin< sophiatx::plugins::chain::chain_plugin_full >();
   
   appbase::app().register_plugin< sophiatx::plugins::witness::witness_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::account_history::account_history_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::subscribe::subscribe_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::custom::custom_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::witness::witness_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::network_broadcast_api::network_broadcast_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::database_api::database_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::alexandria_api::alexandria_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::chain::chain_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::account_by_key::account_by_key_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::block_api::block_api_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::debug_node::debug_node_api_plugin >();

   appbase::app().register_plugin< sophiatx::plugins::multiparty_messaging::multiparty_messaging_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::account_history::account_history_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::debug_node::debug_node_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::webserver::webserver_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::template_plugin::template_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::p2p::p2p_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::block_log_info::block_log_info_plugin >();
   
   appbase::app().register_plugin< sophiatx::plugins::account_by_key::account_by_key_plugin >();
   
}

} }