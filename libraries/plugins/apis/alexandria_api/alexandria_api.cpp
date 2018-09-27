#include <appbase/application.hpp>

#include <sophiatx/plugins/alexandria_api/alexandria_api.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_plugin.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_impl.hpp>
#include <sophiatx/plugins/block_api/block_api.hpp>

#include <sophiatx/protocol/get_config.hpp>

#include <sophiatx/plugins/database_api/database_api_plugin.hpp>
#include <sophiatx/plugins/block_api/block_api_plugin.hpp>
#include <sophiatx/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <sophiatx/plugins/account_history_api/account_history_api_plugin.hpp>
#include <sophiatx/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <sophiatx/plugins/custom_api/custom_api_plugin.hpp>
#include <sophiatx/plugins/subscribe_api/subscribe_api_plugin.hpp>
#include <sophiatx/plugins/witness_api/witness_api_plugin.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

alexandria_api::alexandria_api()
      : my( new alexandria_api_impl() )
{
   JSON_RPC_REGISTER_API( SOPHIATX_ALEXANDRIA_API_PLUGIN_NAME );
}

alexandria_api::~alexandria_api() {}


void alexandria_api::init() {
   auto database = appbase::app().find_plugin< database_api::database_api_plugin >();
   if( database != nullptr ) {
      my->set_database_api(database->api);
   }

   auto block = appbase::app().find_plugin< block_api::block_api_plugin >();
   if( block != nullptr ) {
      my->set_block_api(block->api);
   }

   auto account_by_key = appbase::app().find_plugin< account_by_key::account_by_key_api_plugin >();
   if( account_by_key != nullptr ) {
      my->set_account_by_key_api(account_by_key->api);
   }

   auto account_history = appbase::app().find_plugin< account_history::account_history_api_plugin >();
   if( account_history != nullptr ) {
      my->set_account_history_api(account_history->api);
   }

   auto network_broadcast = appbase::app().find_plugin< network_broadcast_api::network_broadcast_api_plugin >();
   if( network_broadcast != nullptr ) {
      my->set_network_broadcast_api(network_broadcast->api);
   }

   auto witness = appbase::app().find_plugin< witness::witness_api_plugin >();
   if( witness != nullptr ) {
      my->set_witness_api(witness->api);
   }

   auto custom = appbase::app().find_plugin< custom::custom_api_plugin>();
   if( custom != nullptr ) {
      my->set_custom_api(custom->api);
   }

   auto subscribe = appbase::app().find_plugin< subscribe::subscribe_api_plugin>();
   if ( subscribe != nullptr) {
      my->set_subscribe_api(subscribe->api);
   }
}

DEFINE_READ_APIS(alexandria_api,

//         /// alexandria api
//         (help)(gethelp)
//         (about)
//         (hello)
//
//         /// query api
//         (info)
         (list_witnesses)
         (list_witnesses_by_vote)
         (get_witness)
         (get_block)
         (get_ops_in_block)
         (get_feed_history)
         (get_application_buyings)
//         (get_applications)
//         (get_applications_by_ids)
//         (get_received_documents)
//         (get_active_witnesses)
//         (get_transaction)
//         (get_required_signatures)
//
//         ///account api
//         (get_account_name_from_seed)
//         (account_exist)
//         (get_account)
//         (get_account_history)
//         (get_active_authority)
//         (get_owner_authority)
//         (get_memo_key)
//         (get_account_balance)
//         (get_vesting_balance)
//         (create_simple_authority)
//         (create_simple_multisig_authority)
//         (create_simple_managed_authority)
//         (create_simple_multisig_managed_authority)
//
//         /// transaction api
//         (create_account)
//         (update_account)
//         (delete_account)
//         (update_witness)
//         (set_voting_proxy)
//         (vote_for_witness)
//         (transfer)
//         (sponsor_account_fees)
//
//         (transfer_to_vesting)
//         (withdraw_vesting)
//         (create_application)
//         (update_application)
//         (delete_application)
//         (buy_application)
//         (cancel_application_buying)
//
//         (make_custom_json_operation)
//         (make_custom_binary_operation)
//
//         /// helper api
//         (broadcast_transaction)
//         (create_transaction)
//         (create_simple_transaction)
//         (calculate_fee)
//         (fiat_to_sphtx)
//
//         ///local api
//         (get_transaction_digest)
//         (add_signature)
//         (add_fee)
//
//         (sign_digest)
//         (send_and_sign_operation)
//         (send_and_sign_transaction)
//         (verify_signature)
//         (generate_key_pair)
//         (generate_key_pair_from_brain_key)
//         (get_public_key)
//         (from_base64)
//         (to_base64)
//         (encrypt_data)
//         (decrypt_data)
//         (custom_object_subscription)
)

} } } // sophiatx::plugins::alexandria_api
