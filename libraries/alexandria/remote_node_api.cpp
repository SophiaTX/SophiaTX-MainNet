#include<steem/alexandria/remote_node_api.hpp>

namespace steem { namespace wallet{

// This class exists only to provide method signature information to fc::api, not to execute calls.

condenser_api::get_version_return remote_node_api::get_version()
{
   FC_ASSERT( false );
}

condenser_api::state remote_node_api::get_state( string )
{
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::get_active_witnesses()
{
   FC_ASSERT( false );
}

optional< block_header > remote_node_api::get_block_header( uint32_t )
{
   FC_ASSERT( false );
}

optional< database_api::api_signed_block_object > remote_node_api::get_block( uint32_t )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_operation_object > remote_node_api::get_ops_in_block( uint32_t, bool only_virtual )
{
   FC_ASSERT( false );
}

fc::variant_object remote_node_api::get_config()
{
   FC_ASSERT( false );
}

condenser_api::extended_dynamic_global_properties remote_node_api::get_dynamic_global_properties()
{
   FC_ASSERT( false );
}

chain_properties remote_node_api::get_chain_properties()
{
   FC_ASSERT( false );
}

condenser_api::legacy_price remote_node_api::get_current_median_history_price( asset_symbol_type )
{
   FC_ASSERT( false );
}

condenser_api::api_feed_history_object remote_node_api::get_feed_history( asset_symbol_type )
{
   FC_ASSERT( false );
}

condenser_api::api_witness_schedule_object remote_node_api::get_witness_schedule()
{
   FC_ASSERT( false );
}

hardfork_version remote_node_api::get_hardfork_version()
{
   FC_ASSERT( false );
}

condenser_api::scheduled_hardfork remote_node_api::get_next_scheduled_hardfork()
{
   FC_ASSERT( false );
}

vector< vector< account_name_type > > remote_node_api::get_key_references( vector< public_key_type > )
{
   FC_ASSERT( false );
}

vector< condenser_api::extended_account > remote_node_api::get_accounts( vector< account_name_type > )
{
   FC_ASSERT( false );
}

vector< account_id_type > remote_node_api::get_account_references( account_id_type account_id )
{
   FC_ASSERT( false );
}

vector< optional< condenser_api::api_account_object > > remote_node_api::lookup_account_names( vector< account_name_type > )
{
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::lookup_accounts( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

uint64_t remote_node_api::get_account_count()
{
   FC_ASSERT( false );
}

vector< database_api::api_owner_authority_history_object > remote_node_api::get_owner_history( account_name_type )
{
   FC_ASSERT( false );
}

optional< database_api::api_account_recovery_request_object > remote_node_api::get_recovery_request( account_name_type )
{
   FC_ASSERT( false );
}

optional< condenser_api::api_escrow_object > remote_node_api::get_escrow( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}


vector< optional< condenser_api::api_witness_object > > remote_node_api::get_witnesses( vector< witness_id_type > )
{
   FC_ASSERT( false );
}

optional< condenser_api::api_witness_object > remote_node_api::get_witness_by_account( account_name_type )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_witness_object > remote_node_api::get_witnesses_by_vote( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::lookup_witness_accounts( string, uint32_t )
{
   FC_ASSERT( false );
}

uint64_t remote_node_api::get_witness_count()
{
   FC_ASSERT( false );
}


string remote_node_api::get_transaction_hex( signed_transaction )
{
   FC_ASSERT( false );
}

annotated_signed_transaction remote_node_api::get_transaction( transaction_id_type )
{
   FC_ASSERT( false );
}

set< public_key_type > remote_node_api::get_required_signatures( signed_transaction, flat_set< public_key_type > )
{
   FC_ASSERT( false );
}

set< public_key_type > remote_node_api::get_potential_signatures( signed_transaction )
{
   FC_ASSERT( false );
}

bool remote_node_api::verify_authority( signed_transaction )
{
   FC_ASSERT( false );
}

bool remote_node_api::verify_account_authority( string, flat_set< public_key_type > )
{
   FC_ASSERT( false );
}

map< uint32_t, condenser_api::api_operation_object > remote_node_api::get_account_history( account_name_type, uint64_t, uint32_t )
{
   FC_ASSERT( false );
}

void remote_node_api::broadcast_transaction( signed_transaction )
{
   FC_ASSERT( false );
}

network_broadcast_api::broadcast_transaction_synchronous_return remote_node_api::broadcast_transaction_synchronous( signed_transaction )
{
   FC_ASSERT( false );
}

void remote_node_api::broadcast_block( signed_block )
{
   FC_ASSERT( false );
}

vector<condenser_api::api_application_object> remote_node_api::get_applications(vector<string>)
{
    FC_ASSERT( false );
}

vector<condenser_api::api_application_buying_object> remote_node_api::get_application_buyings(std::string, uint32_t, std::string)
{
    FC_ASSERT( false );
}

map< uint64_t, condenser_api::api_received_object >  remote_node_api::get_received_documents(uint32_t app_id, string account_name, string search_type, string start, uint32_t count){
   FC_ASSERT( false);
};

} }
