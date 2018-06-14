#include <steem/protocol/operations.hpp>
#include <steem/plugins/condenser_api/condenser_api_legacy_asset.hpp>

namespace steem { namespace plugins { namespace condenser_api {

   using namespace steem::protocol;

   typedef account_update_operation               legacy_account_update_operation;
   typedef placeholder_a_operation                legacy_placeholder_a_operation;
   typedef placeholder_b_operation                legacy_placeholder_b_operation;
   typedef escrow_approve_operation               legacy_escrow_approve_operation;
   typedef escrow_dispute_operation               legacy_escrow_dispute_operation;
   typedef witness_set_properties_operation       legacy_witness_set_properties_operation;
   typedef account_witness_vote_operation         legacy_account_witness_vote_operation;
   typedef account_witness_proxy_operation        legacy_account_witness_proxy_operation;
   typedef custom_operation                       legacy_custom_operation;
   typedef custom_json_operation                  legacy_custom_json_operation;
   typedef custom_binary_operation                legacy_custom_binary_operation;
   typedef report_over_production_operation       legacy_report_over_production_operation;
   typedef request_account_recovery_operation     legacy_request_account_recovery_operation;
   typedef recover_account_operation              legacy_recover_account_operation;
   typedef reset_account_operation                legacy_reset_account_operation;
   typedef set_reset_account_operation            legacy_set_reset_account_operation;
   typedef change_recovery_account_operation      legacy_change_recovery_account_operation;
   typedef shutdown_witness_operation             legacy_shutdown_witness_operation;
   typedef hardfork_operation                     legacy_hardfork_operation;
   typedef witness_stop_operation                 legacy_witness_stop_operation;
   typedef transfer_from_promotion_pool_operation legacy_transfer_from_promotion_pool_operation;
   typedef account_create_operation               legacy_account_create_operation;
   typedef account_delete_operation               legacy_account_delete_operation;
   typedef price                                  legacy_price;
   typedef transfer_operation                     legacy_transfer_operation;
   typedef escrow_transfer_operation              legacy_escrow_transfer_operation;
   typedef escrow_release_operation               legacy_escrow_release_operation;
   typedef transfer_to_vesting_operation          legacy_transfer_to_vesting_operation;
   typedef withdraw_vesting_operation             legacy_withdraw_vesting_operation;
   typedef withdraw_vesting_operation             legacy_withdraw_vesting_operation;
   typedef witness_update_operation               legacy_witness_update_operation;
   typedef feed_publish_operation                 legacy_feed_publish_operation;
   typedef interest_operation                     legacy_interest_operation;
   typedef fill_vesting_withdraw_operation        legacy_fill_vesting_withdraw_operation;
   typedef producer_reward_operation              legacy_producer_reward_operation;
   typedef promotion_pool_withdraw_operation      legacy_promotion_pool_withdraw_operation;
   typedef application_create_operation           legacy_application_create_operation;
   typedef application_update_operation           legacy_application_update_operation;
   typedef application_delete_operation           legacy_application_delete_operation;
   typedef buy_application_operation              legacy_buy_application_operation;
   typedef cancel_application_buying_operation    legacy_cancel_application_buying_operation;


   typedef fc::static_variant<
            legacy_transfer_operation,
            legacy_transfer_to_vesting_operation,
            legacy_withdraw_vesting_operation,
            legacy_feed_publish_operation,
            legacy_account_create_operation,
            legacy_account_update_operation,
            legacy_account_delete_operation,
            legacy_witness_update_operation,
            legacy_witness_stop_operation,
            legacy_account_witness_vote_operation,
            legacy_account_witness_proxy_operation,
            legacy_custom_operation,
            legacy_report_over_production_operation,
            legacy_custom_json_operation,
            legacy_placeholder_a_operation,
            legacy_placeholder_b_operation,
            legacy_request_account_recovery_operation,
            legacy_recover_account_operation,
            legacy_change_recovery_account_operation,
            legacy_escrow_transfer_operation,
            legacy_escrow_dispute_operation,
            legacy_escrow_release_operation,
            legacy_escrow_approve_operation,
            legacy_custom_binary_operation,
            legacy_reset_account_operation,
            legacy_set_reset_account_operation,
            legacy_witness_set_properties_operation,
            legacy_interest_operation,
            legacy_fill_vesting_withdraw_operation,
            legacy_shutdown_witness_operation,
            legacy_hardfork_operation,
            legacy_producer_reward_operation,
            legacy_promotion_pool_withdraw_operation,
            legacy_transfer_from_promotion_pool_operation,
            legacy_application_create_operation,
            legacy_application_update_operation,
            legacy_application_delete_operation,
            legacy_buy_application_operation,
            legacy_cancel_application_buying_operation
         > legacy_operation;

   struct legacy_operation_conversion_visitor
   {
      legacy_operation_conversion_visitor( legacy_operation& legacy_op ) : l_op( legacy_op ) {}

      typedef bool result_type;

      legacy_operation& l_op;

      bool operator()( const account_update_operation& op )const                 { l_op = op; return true; }
      bool operator()( const placeholder_a_operation& op )const                  { l_op = op; return true; }
      bool operator()( const placeholder_b_operation& op )const                  { l_op = op; return true; }
      bool operator()( const escrow_approve_operation& op )const                 { l_op = op; return true; }
      bool operator()( const escrow_dispute_operation& op )const                 { l_op = op; return true; }
      bool operator()( const witness_set_properties_operation& op )const         { l_op = op; return true; }
      bool operator()( const account_witness_vote_operation& op )const           { l_op = op; return true; }
      bool operator()( const account_witness_proxy_operation& op )const          { l_op = op; return true; }
      bool operator()( const custom_operation& op )const                         { l_op = op; return true; }
      bool operator()( const custom_json_operation& op )const                    { l_op = op; return true; }
      bool operator()( const custom_binary_operation& op )const                  { l_op = op; return true; }
      bool operator()( const report_over_production_operation& op )const         { l_op = op; return true; }
      bool operator()( const request_account_recovery_operation& op )const       { l_op = op; return true; }
      bool operator()( const recover_account_operation& op )const                { l_op = op; return true; }
      bool operator()( const reset_account_operation& op )const                  { l_op = op; return true; }
      bool operator()( const set_reset_account_operation& op )const              { l_op = op; return true; }
      bool operator()( const change_recovery_account_operation& op )const        { l_op = op; return true; }
      bool operator()( const shutdown_witness_operation& op )const               { l_op = op; return true; }
      bool operator()( const hardfork_operation& op )const                       { l_op = op; return true; }

      bool operator()( const transfer_operation& op )const
      {
         l_op = legacy_transfer_operation( op );
         return true;
      }

      bool operator()( const transfer_to_vesting_operation& op )const
      {
         l_op = legacy_transfer_to_vesting_operation( op );
         return true;
      }

      bool operator()( const withdraw_vesting_operation& op )const
      {
         l_op = legacy_withdraw_vesting_operation( op );
         return true;
      }

      bool operator()( const feed_publish_operation& op )const
      {
         l_op = legacy_feed_publish_operation( op );
         return true;
      }

      bool operator()( const account_create_operation& op )const
      {
         l_op = legacy_account_create_operation( op );
         return true;
      }

      bool operator()( const account_delete_operation& op )const
      {
         l_op = legacy_account_delete_operation( op );
         return true;
      }


      bool operator()( const witness_update_operation& op )const
      {
         l_op = legacy_witness_update_operation( op );
         return true;
      }

      bool operator()( const witness_stop_operation& op )const
      {
         l_op = legacy_witness_stop_operation( op );
         return true;
      }

      bool operator()( const escrow_transfer_operation& op )const
      {
         l_op = legacy_escrow_transfer_operation( op );
         return true;
      }

      bool operator()( const escrow_release_operation& op )const
      {
         l_op = legacy_escrow_release_operation( op );
         return true;
      }

      bool operator()( const interest_operation& op )const
      {
         l_op = legacy_interest_operation( op );
         return true;
      }

      bool operator()( const fill_vesting_withdraw_operation& op )const
      {
         l_op = legacy_fill_vesting_withdraw_operation( op );
         return true;
      }

      bool operator()( const producer_reward_operation& op )const
      {
         l_op = legacy_producer_reward_operation( op );
         return true;
      }

      bool operator()( const promotion_pool_withdraw_operation& op) const
      {
         l_op = legacy_promotion_pool_withdraw_operation(op);
         return true;
      }

      bool operator()( const transfer_from_promotion_pool_operation& op) const
      {
         l_op = legacy_transfer_from_promotion_pool_operation(op);
         return true;
      }

      bool operator()( const application_create_operation& op) const
      {
         l_op = legacy_application_create_operation(op);
         return true;
      }

      bool operator()( const application_update_operation& op) const
      {
         l_op = legacy_application_update_operation(op);
         return true;
      }

      bool operator()( const application_delete_operation& op) const
      {
         l_op = legacy_application_delete_operation(op);
         return true;
      }

      bool operator()( const buy_application_operation& op) const
      {
         l_op = legacy_buy_application_operation(op);
         return true;
      }
      bool operator()( const cancel_application_buying_operation& op) const
      {
         l_op = legacy_cancel_application_buying_operation(op);
         return true;
      }

      // Should only be SMT ops
      template< typename T >
      bool operator()( const T& )const { return false; }
};

struct convert_from_legacy_operation_visitor
{
   convert_from_legacy_operation_visitor() {}

   typedef operation result_type;

   operation operator()( const legacy_transfer_operation& op )const
   {
      return operation( transfer_operation( op ) );
   }

   operation operator()( const legacy_transfer_to_vesting_operation& op )const
   {
      return operation( transfer_to_vesting_operation( op ) );
   }

   operation operator()( const legacy_withdraw_vesting_operation& op )const
   {
      return operation( withdraw_vesting_operation( op ) );
   }

   operation operator()( const legacy_feed_publish_operation& op )const
   {
      return operation( feed_publish_operation( op ) );
   }

   operation operator()( const legacy_account_create_operation& op )const
   {
      return operation( account_create_operation( op ) );
   }

   operation operator()( const legacy_account_delete_operation& op )const
   {
      return operation( account_delete_operation( op ) );
   }

   operation operator()( const legacy_witness_update_operation& op )const
   {
      return operation( witness_update_operation( op ) );
   }

   operation operator()( const legacy_witness_stop_operation& op )const
   {
      return operation( witness_stop_operation( op ) );
   }

   operation operator()( const legacy_escrow_transfer_operation& op )const
   {
      return operation( escrow_transfer_operation( op ) );
   }

   operation operator()( const legacy_escrow_release_operation& op )const
   {
      return operation( escrow_release_operation( op ) );
   }

   operation operator()( const legacy_interest_operation& op )const
   {
      return operation( interest_operation( op ) );
   }

   operation operator()( const legacy_fill_vesting_withdraw_operation& op )const
   {
      return operation( fill_vesting_withdraw_operation( op ) );
   }

   operation operator()( const legacy_producer_reward_operation& op )const
   {
      return operation( producer_reward_operation( op ) );
   }

   operation operator()( const legacy_promotion_pool_withdraw_operation& op)const
   {
      return operation( promotion_pool_withdraw_operation(op) );
   }

   operation operator()( const legacy_transfer_from_promotion_pool_operation& op)const
   {
      return operation( transfer_from_promotion_pool_operation(op));
   }

   operation operator()( const legacy_application_create_operation& op)const
   {
      return operation( application_create_operation(op));
   }

   operation operator()( const legacy_application_update_operation& op)const
   {
      return operation( application_update_operation(op));
   }

   operation operator()( const legacy_application_delete_operation& op)const
   {
      return operation( application_delete_operation(op));
   }

   operation operator()( const legacy_buy_application_operation& op)const
   {
      return operation( buy_application_operation(op));
   }

   operation operator()( const legacy_cancel_application_buying_operation& op)const
   {
      return operation( cancel_application_buying_operation(op));
   }

   template< typename T >
   operation operator()( const T& t )const
   {
      return operation( t );
   }
};

} } } // steem::plugins::condenser_api

namespace fc {

void to_variant( const steem::plugins::condenser_api::legacy_operation&, fc::variant& );
void from_variant( const fc::variant&, steem::plugins::condenser_api::legacy_operation& );

}

FC_REFLECT_TYPENAME( steem::plugins::condenser_api::legacy_operation )
