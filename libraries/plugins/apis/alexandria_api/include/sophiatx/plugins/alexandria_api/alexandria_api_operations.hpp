#include <sophiatx/protocol/operations.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_asset.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

   using namespace sophiatx::protocol;

   typedef account_update_operation               api_account_update_operation;
   typedef escrow_approve_operation               api_escrow_approve_operation;
   typedef escrow_dispute_operation               api_escrow_dispute_operation;
   typedef witness_set_properties_operation       api_witness_set_properties_operation;
   typedef account_witness_vote_operation         api_account_witness_vote_operation;
   typedef account_witness_proxy_operation        api_account_witness_proxy_operation;
   typedef custom_operation                       api_custom_operation;
   typedef custom_json_operation                  api_custom_json_operation;
   typedef custom_binary_operation                api_custom_binary_operation;
   typedef request_account_recovery_operation     api_request_account_recovery_operation;
   typedef recover_account_operation              api_recover_account_operation;
   typedef reset_account_operation                api_reset_account_operation;
   typedef set_reset_account_operation            api_set_reset_account_operation;
   typedef change_recovery_account_operation      api_change_recovery_account_operation;
   typedef shutdown_witness_operation             api_shutdown_witness_operation;
   typedef hardfork_operation                     api_hardfork_operation;
   typedef witness_stop_operation                 api_witness_stop_operation;
   typedef transfer_from_promotion_pool_operation api_transfer_from_promotion_pool_operation;
   typedef account_create_operation               api_account_create_operation;
   typedef account_delete_operation               api_account_delete_operation;
   typedef price                                  api_price;
   typedef transfer_operation                     api_transfer_operation;
   typedef escrow_transfer_operation              api_escrow_transfer_operation;
   typedef escrow_release_operation               api_escrow_release_operation;
   typedef transfer_to_vesting_operation          api_transfer_to_vesting_operation;
   typedef withdraw_vesting_operation             api_withdraw_vesting_operation;
   typedef withdraw_vesting_operation             api_withdraw_vesting_operation;
   typedef witness_update_operation               api_witness_update_operation;
   typedef feed_publish_operation                 api_feed_publish_operation;
   typedef interest_operation                     api_interest_operation;
   typedef fill_vesting_withdraw_operation        api_fill_vesting_withdraw_operation;
   typedef producer_reward_operation              api_producer_reward_operation;
   typedef promotion_pool_withdraw_operation      api_promotion_pool_withdraw_operation;
   typedef application_create_operation           api_application_create_operation;
   typedef application_update_operation           api_application_update_operation;
   typedef application_delete_operation           api_application_delete_operation;
   typedef sponsor_fees_operation                 api_sponsor_fees_operation;
   typedef buy_application_operation              api_buy_application_operation;
   typedef cancel_application_buying_operation    api_cancel_application_buying_operation;

   typedef placeholder_a_operation                api_placeholder_a_operation;
   typedef placeholder_b_operation                api_placeholder_b_operation;

   typedef fc::static_variant<
            api_transfer_operation,
            api_transfer_to_vesting_operation,
            api_withdraw_vesting_operation,
            api_feed_publish_operation,

            api_account_create_operation,
            api_account_update_operation,
            api_account_delete_operation,

            api_witness_update_operation,
            api_witness_stop_operation,
            api_account_witness_vote_operation,
            api_account_witness_proxy_operation,
            api_witness_set_properties_operation,

            api_custom_operation,
            api_custom_json_operation,
            api_custom_binary_operation,


            api_request_account_recovery_operation,
            api_recover_account_operation,
            api_change_recovery_account_operation,
            api_escrow_transfer_operation,
            api_escrow_dispute_operation,
            api_escrow_release_operation,
            api_escrow_approve_operation,

            api_reset_account_operation,
            api_set_reset_account_operation,

            api_application_create_operation,
            api_application_update_operation,
            api_application_delete_operation,
            api_buy_application_operation,
            api_cancel_application_buying_operation,

            api_transfer_from_promotion_pool_operation,
            api_sponsor_fees_operation,

            api_interest_operation,
            api_fill_vesting_withdraw_operation,
            api_shutdown_witness_operation,
            api_hardfork_operation,
            api_producer_reward_operation,
            api_promotion_pool_withdraw_operation,

            api_placeholder_a_operation
         > api_operation;

   struct api_operation_conversion_visitor
   {
      api_operation_conversion_visitor( api_operation& api_op ) : l_op( api_op ) {}

      typedef bool result_type;

      api_operation& l_op;
      bool operator()( const transfer_operation& op )const                       { l_op = op; return true; }
      bool operator()( const transfer_to_vesting_operation& op )const            { l_op = op; return true; }
      bool operator()( const withdraw_vesting_operation& op )const               { l_op = op; return true; }
      bool operator()( const feed_publish_operation& op )const                   { l_op = op; return true; }
      bool operator()( const account_create_operation& op )const                 { l_op = op; return true; }
      bool operator()( const account_update_operation& op )const                 { l_op = op; return true; }
      bool operator()( const account_delete_operation& op )const                 { l_op = op; return true; }
      bool operator()( const witness_update_operation& op )const                 { l_op = op; return true; }
      bool operator()( const witness_stop_operation& op )const                   { l_op = op; return true; }
      bool operator()( const account_witness_vote_operation& op )const           { l_op = op; return true; }
      bool operator()( const account_witness_proxy_operation& op )const          { l_op = op; return true; }
      bool operator()( const witness_set_properties_operation& op )const         { l_op = op; return true; }
      bool operator()( const custom_operation& op )const                         { l_op = op; return true; }
      bool operator()( const custom_json_operation& op )const                    { l_op = op; return true; }
      bool operator()( const custom_binary_operation& op )const                  { l_op = op; return true; }
      bool operator()( const request_account_recovery_operation& op )const       { l_op = op; return true; }
      bool operator()( const recover_account_operation& op )const                { l_op = op; return true; }
      bool operator()( const change_recovery_account_operation& op )const        { l_op = op; return true; }
      bool operator()( const escrow_transfer_operation& op )const                { l_op = op; return true; }
      bool operator()( const escrow_approve_operation& op )const                 { l_op = op; return true; }
      bool operator()( const escrow_dispute_operation& op )const                 { l_op = op; return true; }
      bool operator()( const escrow_release_operation& op )const                 { l_op = op; return true; }
      bool operator()( const reset_account_operation& op )const                  { l_op = op; return true; }
      bool operator()( const set_reset_account_operation& op )const              { l_op = op; return true; }
      bool operator()( const application_create_operation& op )const             { l_op = op; return true; }
      bool operator()( const application_update_operation& op )const             { l_op = op; return true; }
      bool operator()( const application_delete_operation& op )const             { l_op = op; return true; }
      bool operator()( const buy_application_operation& op )const                { l_op = op; return true; }
      bool operator()( const cancel_application_buying_operation& op )const      { l_op = op; return true; }
      bool operator()( const transfer_from_promotion_pool_operation& op )const   { l_op = op; return true; }
      bool operator()( const sponsor_fees_operation& op )const                   { l_op = op; return true; }

      bool operator()( const interest_operation& op )const                       { l_op = op; return true; }
      bool operator()( const fill_vesting_withdraw_operation& op )const          { l_op = op; return true; }
      bool operator()( const shutdown_witness_operation& op )const               { l_op = op; return true; }
      bool operator()( const hardfork_operation& op )const                       { l_op = op; return true; }
      bool operator()( const producer_reward_operation& op )const                { l_op = op; return true; }
      bool operator()( const promotion_pool_withdraw_operation& op )const        { l_op = op; return true; }

      // Should only be SMT ops
      template< typename T >
      bool operator()( const T& )const { return false; }
};

struct convert_from_api_operation_visitor
{
   convert_from_api_operation_visitor() {}

   typedef operation result_type;

   /*operation operator()( const api_transfer_operation& op )const
   {
      return operation( transfer_operation( op ) );
   }

   operation operator()( const api_transfer_to_vesting_operation& op )const
   {
      return operation( transfer_to_vesting_operation( op ) );
   }

   operation operator()( const api_withdraw_vesting_operation& op )const
   {
      return operation( withdraw_vesting_operation( op ) );
   }

   operation operator()( const api_feed_publish_operation& op )const
   {
      return operation( feed_publish_operation( op ) );
   }

   operation operator()( const api_account_create_operation& op )const
   {
      return operation( account_create_operation( op ) );
   }

   operation operator()( const api_account_delete_operation& op )const
   {
      return operation( account_delete_operation( op ) );
   }

   operation operator()( const api_witness_update_operation& op )const
   {
      return operation( witness_update_operation( op ) );
   }

   operation operator()( const api_witness_stop_operation& op )const
   {
      return operation( witness_stop_operation( op ) );
   }

   operation operator()( const api_escrow_transfer_operation& op )const
   {
      return operation( escrow_transfer_operation( op ) );
   }

   operation operator()( const api_escrow_release_operation& op )const
   {
      return operation( escrow_release_operation( op ) );
   }

   operation operator()( const api_interest_operation& op )const
   {
      return operation( interest_operation( op ) );
   }

   operation operator()( const api_fill_vesting_withdraw_operation& op )const
   {
      return operation( fill_vesting_withdraw_operation( op ) );
   }

   operation operator()( const api_producer_reward_operation& op )const
   {
      return operation( producer_reward_operation( op ) );
   }

   operation operator()( const api_promotion_pool_withdraw_operation& op)const
   {
      return operation( promotion_pool_withdraw_operation(op) );
   }

   operation operator()( const api_transfer_from_promotion_pool_operation& op)const
   {
      return operation( transfer_from_promotion_pool_operation(op));
   }

   operation operator()( const api_application_create_operation& op)const
   {
      return operation( application_create_operation(op));
   }

   operation operator()( const api_application_update_operation& op)const
   {
      return operation( application_update_operation(op));
   }

   operation operator()( const api_application_delete_operation& op)const
   {
      return operation( application_delete_operation(op));
   }

   operation operator()( const api_buy_application_operation& op)const
   {
      return operation( buy_application_operation(op));
   }

   operation operator()( const api_cancel_application_buying_operation& op)const
   {
      return operation( cancel_application_buying_operation(op));
   }*/

   operation operator() (const api_placeholder_a_operation& op)const{
      return operation( transfer_operation() );
   }
   template< typename T >
   operation operator()( const T& t )const
   {
      return operation( t );
   }
};

} } } // sophiatx::plugins::alexandria_api

namespace fc {

void to_variant( const sophiatx::plugins::alexandria_api::api_operation&, fc::variant& );
void from_variant( const fc::variant&, sophiatx::plugins::alexandria_api::api_operation& );

}

FC_REFLECT_TYPENAME( sophiatx::plugins::alexandria_api::api_operation )
