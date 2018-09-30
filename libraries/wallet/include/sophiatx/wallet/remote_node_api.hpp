#pragma once
#include <sophiatx/plugins/condenser_api/condenser_api.hpp>

namespace sophiatx { namespace wallet {

using std::vector;
using fc::variant;
using fc::optional;

using namespace chain;
using namespace plugins;
/*using namespace plugins::condenser_api;
using namespace plugins::database_api;
using namespace plugins::account_history;
using namespace plugins::follow;
using namespace plugins::market_history;
using namespace plugins::witness;*/

/**
 * This is a dummy API so that the wallet can create properly formatted API calls
 */
struct remote_node_api
{
   condenser_api::get_version_return get_version();
   vector< account_name_type > get_active_witnesses();
   optional< block_header > get_block_header( uint32_t );
   optional< database_api::api_signed_block_object > get_block( uint32_t );
   vector< condenser_api::api_operation_object > get_ops_in_block( uint32_t, bool only_virtual = true );
   fc::variant_object get_config();
   condenser_api::extended_dynamic_global_properties get_dynamic_global_properties();
   chain_properties get_chain_properties();
   condenser_api::legacy_price get_current_median_history_price( asset_symbol_type );
   condenser_api::api_feed_history_object get_feed_history( asset_symbol_type);
   condenser_api::api_witness_schedule_object get_witness_schedule();
   hardfork_version get_hardfork_version();
   condenser_api::scheduled_hardfork get_next_scheduled_hardfork();
   vector< vector< account_name_type > > get_key_references( vector< public_key_type > );
   vector< condenser_api::extended_account > get_accounts( vector< account_name_type > );
   vector< account_id_type > get_account_references( account_id_type account_id );
   vector< optional< condenser_api::api_account_object > > lookup_account_names( vector< account_name_type > );
   uint64_t get_account_count();
   vector< database_api::api_owner_authority_history_object > get_owner_history( account_name_type );
   optional< database_api::api_account_recovery_request_object > get_recovery_request( account_name_type );
   optional< condenser_api::api_escrow_object > get_escrow( account_name_type, uint32_t );
   optional< witness::api_account_bandwidth_object > get_account_bandwidth( account_name_type, witness::bandwidth_type );
   vector< optional< condenser_api::api_witness_object > > get_witnesses( vector< witness_id_type > );
   optional< condenser_api::api_witness_object > get_witness_by_account( account_name_type );
   vector< condenser_api::api_witness_object > get_witnesses_by_vote( account_name_type, uint32_t );
   vector< account_name_type > lookup_witness_accounts( string, uint32_t );
   uint64_t get_witness_count();
   string get_transaction_hex( signed_transaction );
   annotated_signed_transaction get_transaction( transaction_id_type );
   set< public_key_type > get_required_signatures( signed_transaction, flat_set< public_key_type > );
   set< public_key_type > get_potential_signatures( signed_transaction );
   bool verify_authority( signed_transaction );
   bool verify_account_authority( string, flat_set< public_key_type > );
   //vector< condenser_api::account_vote > get_account_votes( account_name_type );

   map< uint32_t, condenser_api::api_operation_object > get_account_history( account_name_type, int64_t, uint32_t );
   void broadcast_transaction( signed_transaction );
   network_broadcast_api::broadcast_transaction_synchronous_return broadcast_transaction_synchronous( signed_transaction );
   void broadcast_block( signed_block );
   flat_set< uint32_t > get_market_history_buckets();
   map< uint64_t, condenser_api::api_received_object > list_received_documents(uint32_t, string, string, string, uint32_t);
   condenser_api::api_received_object get_received_document(uint64_t id);

   vector<condenser_api::api_application_object> get_applications(vector<uint32_t>);
   vector<condenser_api::api_application_object> get_applications_by_names(vector<string>);
   vector<condenser_api::api_application_buying_object> get_application_buyings(string, uint32_t, string);
};

} }

FC_API( sophiatx::wallet::remote_node_api,
        (get_version)
        (get_active_witnesses)
        (get_block_header)
        (get_block)
        (get_ops_in_block)
        (get_config)
        (get_dynamic_global_properties)
        (get_chain_properties)
        (get_current_median_history_price)
        (get_feed_history)
        (get_witness_schedule)
        (get_hardfork_version)
        (get_next_scheduled_hardfork)
        (get_key_references)
        (get_accounts)
        (get_account_references)
        (lookup_account_names)
        (get_account_count)
        (get_owner_history)
        (get_recovery_request)
        (get_escrow)
        (get_witnesses)
        (get_witness_by_account)
        (get_witnesses_by_vote)
        (lookup_witness_accounts)
        (get_witness_count)
        (get_transaction_hex)
        (get_transaction)
        (get_required_signatures)
        (get_potential_signatures)
        (verify_authority)
        (verify_account_authority)
        (get_account_history)
        (broadcast_transaction)
        (broadcast_transaction_synchronous)
        (broadcast_block)
        (get_applications)
        (get_applications_by_names)
        (get_application_buyings)
        (list_received_documents)
        (get_received_document)

      )
