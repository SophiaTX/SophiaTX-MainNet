#include<sophiatx/wallet/remote_node_api.hpp>

namespace sophiatx { namespace wallet{

// This class exists only to provide method signature information to fc::api, not to execute calls.

alexandria_api::info_return remote_node_api::info(json_rpc::void_type)
{
   FC_ASSERT( false );
}

alexandria_api::about_return about(json_rpc::void_type) {
   FC_ASSERT( false );
}

//condenser_api::get_version_return remote_node_api::get_version()
//{
//   FC_ASSERT( false );
//}


alexandria_api::get_active_witnesses_return remote_node_api::get_active_witnesses(json_rpc::void_type)
{
   FC_ASSERT( false );
}

//optional< block_header > remote_node_api::get_block_header( uint32_t )
//{
//   FC_ASSERT( false );
//}

//optional< database_api::api_signed_block_object > remote_node_api::get_block( uint32_t )
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_block_return remote_node_api::get_block( alexandria_api::get_block_args args ) {
   FC_ASSERT( false );
}

alexandria_api::get_ops_in_block_return remote_node_api::get_ops_in_block( alexandria_api::get_ops_in_block_args )
{
   FC_ASSERT( false );
}

//fc::variant_object remote_node_api::get_config()
//{
//   FC_ASSERT( false );
//}

//condenser_api::extended_dynamic_global_properties remote_node_api::get_dynamic_global_properties()
//{
//   FC_ASSERT( false );
//}
//
//chain_properties remote_node_api::get_chain_properties()
//{
//   FC_ASSERT( false );
//}

//condenser_api::legacy_price remote_node_api::get_current_median_history_price( asset_symbol_type )
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_feed_history_return remote_node_api::get_feed_history( alexandria_api::get_feed_history_args )
{
   FC_ASSERT( false );
}

//condenser_api::api_witness_schedule_object remote_node_api::get_witness_schedule()
//{
//   FC_ASSERT( false );
//}
//
//hardfork_version remote_node_api::get_hardfork_version()
//{
//   FC_ASSERT( false );
//}
//
//condenser_api::scheduled_hardfork remote_node_api::get_next_scheduled_hardfork()
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_account_return remote_node_api::get_account(alexandria_api::get_account_args)
{
   FC_ASSERT( false );
}

alexandria_api::get_accounts_return remote_node_api::get_accounts(alexandria_api::get_accounts_args) {
   FC_ASSERT( false );
}

alexandria_api::create_account_return remote_node_api::create_account(alexandria_api::create_account_args) {
   FC_ASSERT( false );
}

//vector< vector< account_name_type > > remote_node_api::get_key_references( vector< public_key_type > )
//{
//   FC_ASSERT( false );
//}
//
//vector< condenser_api::extended_account > remote_node_api::get_accounts( vector< account_name_type > )
//{
//   FC_ASSERT( false );
//}
//
//vector< account_id_type > remote_node_api::get_account_references( account_id_type account_id )
//{
//   FC_ASSERT( false );
//}
//
//vector< optional< condenser_api::api_account_object > > remote_node_api::lookup_account_names( vector< account_name_type > )
//{
//   FC_ASSERT( false );
//}
//
//
//uint64_t remote_node_api::get_account_count()
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_owner_history_return remote_node_api::get_owner_history( alexandria_api::get_owner_history_args )
{
   FC_ASSERT( false );
}

//optional< database_api::api_account_recovery_request_object > remote_node_api::get_recovery_request( account_name_type )
//{
//   FC_ASSERT( false );
//}
//
//optional< condenser_api::api_escrow_object > remote_node_api::get_escrow( account_name_type, uint32_t )
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_witness_return remote_node_api::get_witness(alexandria_api::get_witness_args) {
   FC_ASSERT( false );
}

//vector< optional< condenser_api::api_witness_object > > remote_node_api::get_witnesses( vector< witness_id_type > )
//{
//   FC_ASSERT( false );
//}
//
//optional< condenser_api::api_witness_object > remote_node_api::get_witness_by_account( account_name_type )
//{
//   FC_ASSERT( false );
//}
//
//vector< condenser_api::api_witness_object > remote_node_api::get_witnesses_by_vote( account_name_type, uint32_t )
//{
//   FC_ASSERT( false );
//}

alexandria_api::list_witnesses_return remote_node_api::list_witnesses( alexandria_api::list_witnesses_args )
{
   FC_ASSERT( false );
}

//uint64_t remote_node_api::get_witness_count()
//{
//   FC_ASSERT( false );
//}
//
//
//string remote_node_api::get_transaction_hex( signed_transaction )
//{
//   FC_ASSERT( false );
//}
//
//set< public_key_type > remote_node_api::get_required_signatures( signed_transaction, flat_set< public_key_type > )
//{
//   FC_ASSERT( false );
//}
//
//set< public_key_type > remote_node_api::get_potential_signatures( signed_transaction )
//{
//   FC_ASSERT( false );
//}
//
//bool remote_node_api::verify_authority( signed_transaction )
//{
//   FC_ASSERT( false );
//}
//
//bool remote_node_api::verify_account_authority( string, flat_set< public_key_type > )
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_account_history_return remote_node_api::get_account_history( alexandria_api::get_account_history_args )
{
   FC_ASSERT( false );
}

alexandria_api::broadcast_transaction_return remote_node_api::broadcast_transaction( alexandria_api::broadcast_transaction_args )
{
   FC_ASSERT( false );
}

//network_broadcast_api::broadcast_transaction_synchronous_return remote_node_api::broadcast_transaction_synchronous( signed_transaction )
//{
//   FC_ASSERT( false );
//}
//
//void remote_node_api::broadcast_block( signed_block )
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_applications_return get_applications(alexandria_api::get_applications_args)
{
    FC_ASSERT( false );
}

//vector<condenser_api::api_application_object> remote_node_api::get_applications_by_names(vector<string>)
//{
//   FC_ASSERT( false );
//}

alexandria_api::get_application_buyings_return get_application_buyings(alexandria_api::get_application_buyings_args)
{
    FC_ASSERT( false );
}

alexandria_api::get_received_documents_return remote_node_api::get_received_documents( alexandria_api::get_received_documents_args ) {
   FC_ASSERT( false);
}

//condenser_api::api_received_object  remote_node_api::get_received_document(uint64_t id){
//   FC_ASSERT( false);
//};

} }
