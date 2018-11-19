#include<sophiatx/wallet/remote_node_api.hpp>

namespace sophiatx { namespace wallet{

using namespace sophiatx::plugins;

// This class exists only to provide method signature information to fc::api, not to execute calls.

alexandria_api::info_return remote_node_api::info(json_rpc::void_type)
{
   FC_ASSERT( false );
}

alexandria_api::about_return about(json_rpc::void_type) {
   FC_ASSERT( false );
}

alexandria_api::get_version_return remote_node_api::get_version(json_rpc::void_type)
{
   FC_ASSERT( false );
}

alexandria_api::get_active_witnesses_return remote_node_api::get_active_witnesses(json_rpc::void_type)
{
   FC_ASSERT( false );
}

alexandria_api::get_block_return remote_node_api::get_block( alexandria_api::get_block_args args ) {
   FC_ASSERT( false );
}

alexandria_api::get_ops_in_block_return remote_node_api::get_ops_in_block( alexandria_api::get_ops_in_block_args )
{
   FC_ASSERT( false );
}

alexandria_api::get_dynamic_global_properties_return remote_node_api::get_dynamic_global_properties(json_rpc::void_type)
{
   FC_ASSERT( false );
}

alexandria_api::get_feed_history_return remote_node_api::get_feed_history( alexandria_api::get_feed_history_args )
{
   FC_ASSERT( false );
}

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

alexandria_api::get_key_references_return remote_node_api::get_key_references( alexandria_api::get_key_references_args )
{
   FC_ASSERT( false );
}

alexandria_api::get_owner_history_return remote_node_api::get_owner_history( alexandria_api::get_owner_history_args )
{
   FC_ASSERT( false );
}

alexandria_api::get_witness_return remote_node_api::get_witness(alexandria_api::get_witness_args) {
   FC_ASSERT( false );
}

alexandria_api::list_witnesses_return remote_node_api::list_witnesses( alexandria_api::list_witnesses_args )
{
   FC_ASSERT( false );
}

alexandria_api::get_account_history_return remote_node_api::get_account_history( alexandria_api::get_account_history_args )
{
   FC_ASSERT( false );
}

alexandria_api::broadcast_transaction_return remote_node_api::broadcast_transaction( alexandria_api::broadcast_transaction_args )
{
   FC_ASSERT( false );
}

alexandria_api::get_applications_return get_applications(alexandria_api::get_applications_args)
{
    FC_ASSERT( false );
}

alexandria_api::get_application_buyings_return get_application_buyings(alexandria_api::get_application_buyings_args)
{
    FC_ASSERT( false );
}

alexandria_api::get_received_documents_return remote_node_api::get_received_documents( alexandria_api::get_received_documents_args ) {
   FC_ASSERT( false);
}

} }
