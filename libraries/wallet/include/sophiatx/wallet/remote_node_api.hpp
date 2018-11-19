#pragma once

#include <fc/api.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_args.hpp>

namespace sophiatx { namespace wallet {

/**
 * This is a dummy API so that the wallet can create properly formatted API calls
 */
struct remote_node_api
{
   plugins::alexandria_api::info_return info(plugins::json_rpc::void_type);
   plugins::alexandria_api::about_return about(plugins::json_rpc::void_type);
   plugins::alexandria_api::get_version_return get_version(plugins::json_rpc::void_type);
   plugins::alexandria_api::get_active_witnesses_return get_active_witnesses(plugins::json_rpc::void_type);
   plugins::alexandria_api::get_block_return get_block( plugins::alexandria_api::get_block_args args );
   plugins::alexandria_api::get_ops_in_block_return get_ops_in_block( plugins::alexandria_api::get_ops_in_block_args );
   plugins::alexandria_api::get_dynamic_global_properties_return get_dynamic_global_properties(plugins::json_rpc::void_type);
   plugins::alexandria_api::get_feed_history_return get_feed_history( plugins::alexandria_api::get_feed_history_args );
   plugins::alexandria_api::get_account_return get_account(plugins::alexandria_api::get_account_args);
   plugins::alexandria_api::get_accounts_return get_accounts(plugins::alexandria_api::get_accounts_args);
   plugins::alexandria_api::create_account_return create_account(plugins::alexandria_api::create_account_args);
   plugins::alexandria_api::get_key_references_return get_key_references( plugins::alexandria_api::get_key_references_args );
   plugins::alexandria_api::get_owner_history_return get_owner_history( plugins::alexandria_api::get_owner_history_args );
   plugins::alexandria_api::get_witness_return get_witness(plugins::alexandria_api::get_witness_args);
   plugins::alexandria_api::list_witnesses_return list_witnesses( plugins::alexandria_api::list_witnesses_args );
   plugins::alexandria_api::get_transaction_return get_transaction( plugins::alexandria_api::get_transaction_args );
   plugins::alexandria_api::get_account_history_return get_account_history( plugins::alexandria_api::get_account_history_args );
   plugins::alexandria_api::broadcast_transaction_return broadcast_transaction( plugins::alexandria_api::broadcast_transaction_args );
   plugins::alexandria_api::get_received_documents_return get_received_documents( plugins::alexandria_api::get_received_documents_args );
   plugins::alexandria_api::get_applications_return get_applications(plugins::alexandria_api::get_applications_args);
   plugins::alexandria_api::get_application_buyings_return get_application_buyings(plugins::alexandria_api::get_application_buyings_args);
};

} }

FC_API( sophiatx::wallet::remote_node_api,
        (info)
        (about)
        (get_version)
        (get_active_witnesses)
        (get_block)
        (get_ops_in_block)
        (get_dynamic_global_properties)
        (get_feed_history)
        (get_key_references)
        (create_account)
        (get_account)
        (get_accounts)
        (get_owner_history)
        (get_witness)
        (list_witnesses)
        (get_transaction)
        (get_account_history)
        (broadcast_transaction)
        (get_applications)
        (get_application_buyings)
        (get_received_documents)
      )