#include <appbase/application.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <steem/manifest/plugins.hpp>


#include <steem/plugins/smt_test/smt_test_plugin.hpp>

#include <steem/plugins/witness/witness_plugin.hpp>

#include <steem/plugins/account_history_api/account_history_api_plugin.hpp>

#include <steem/plugins/custom_api/custom_api_plugin.hpp>

#include <steem/plugins/witness_api/witness_api_plugin.hpp>

#include <steem/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>

#include <steem/plugins/database_api/database_api_plugin.hpp>

#include <steem/plugins/chain_api/chain_api_plugin.hpp>

#include <steem/plugins/account_by_key_api/account_by_key_api_plugin.hpp>

#include <steem/plugins/block_api/block_api_plugin.hpp>

#include <steem/plugins/debug_node_api/debug_node_api_plugin.hpp>

#include <steem/plugins/condenser_api/condenser_api_plugin.hpp>

#include <steem/plugins/account_history/account_history_plugin.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

#include <steem/plugins/debug_node/debug_node_plugin.hpp>

#include <steem/plugins/webserver/webserver_plugin.hpp>

#include <steem/plugins/p2p/p2p_plugin.hpp>

#include <steem/plugins/block_log_info/block_log_info_plugin.hpp>

#include <steem/plugins/account_by_key/account_by_key_plugin.hpp>


namespace steem { namespace plugins {

void register_plugins()
{
   
   appbase::app().register_plugin< steem::plugins::smt_test::smt_test_plugin >();
   
   appbase::app().register_plugin< steem::plugins::witness::witness_plugin >();
   
   appbase::app().register_plugin< steem::plugins::account_history::account_history_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::custom::custom_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::witness::witness_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::network_broadcast_api::network_broadcast_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::database_api::database_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::chain::chain_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::account_by_key::account_by_key_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::block_api::block_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::debug_node::debug_node_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::condenser_api::condenser_api_plugin >();
   
   appbase::app().register_plugin< steem::plugins::account_history::account_history_plugin >();
   
   appbase::app().register_plugin< steem::plugins::chain::chain_plugin >();
   
   appbase::app().register_plugin< steem::plugins::debug_node::debug_node_plugin >();
   
   appbase::app().register_plugin< steem::plugins::webserver::webserver_plugin >();
   
   appbase::app().register_plugin< steem::plugins::p2p::p2p_plugin >();
   
   appbase::app().register_plugin< steem::plugins::block_log_info::block_log_info_plugin >();
   
   appbase::app().register_plugin< steem::plugins::account_by_key::account_by_key_plugin >();
   
}

} }