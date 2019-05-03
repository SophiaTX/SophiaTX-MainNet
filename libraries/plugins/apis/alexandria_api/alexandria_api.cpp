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

DEFINE_LOCKLESS_APIS(alexandria_api,
      (get_version)
      (sponsor_account_fees)
      (get_account_name_from_seed)
      (create_simple_authority)
      (create_simple_multisig_authority)
      (create_simple_managed_authority)
      (create_simple_multisig_managed_authority)
      (from_base64)
      (to_base64)
      (encrypt_data)
      (decrypt_data)
      (generate_key_pair_from_brain_key)
      (generate_key_pair)
      (verify_signature)
      (sign_digest)
      (add_signature)
      (add_fee)
      (delete_account)
		(get_transaction_id)
		(make_custom_json_operation)
		(make_custom_binary_operation)
		(create_application)
		(update_application)
		(delete_application)
		(buy_application)
		(cancel_application_buying)
		(vote_for_witness)
		(set_voting_proxy)
		(stop_witness)
		(transfer)
		(transfer_to_vesting)
		(withdraw_vesting)
		)

DEFINE_READ_APIS(alexandria_api,
      (info)
      (about)
      (get_block)
		(get_ops_in_block)
		(get_feed_history)
		(get_active_witnesses)
		(get_account)
		(get_accounts)
		(get_transaction)
		(create_account)
		(update_account)
		(update_account_auth)
		(list_witnesses)
		(list_witnesses_by_vote)
		(get_witness)
		(update_witness)
		(get_owner_history)
		(get_application_buyings)
		(broadcast_transaction)
		(create_transaction)
		(create_simple_transaction)
		(get_applications)
		(get_applications_by_ids)
		(get_transaction_digest)
		(send_and_sign_operation)
		(send_and_sign_transaction)
		(get_public_key)
		(account_exist)
		(get_account_history)
		(get_received_documents)
		(get_active_authority)
		(get_owner_authority)
		(get_memo_key)
		(get_account_balance)
		(get_vesting_balance)
		(get_required_signatures)
		(calculate_fee)
		(fiat_to_sphtx)
		(custom_object_subscription)
		(get_key_references)
		(get_dynamic_global_properties)
		(get_witness_schedule_object)
		(get_hardfork_property_object)
)

} } } // sophiatx::plugins::alexandria_api
