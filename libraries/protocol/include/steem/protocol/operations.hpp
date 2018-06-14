#pragma once

#include <steem/protocol/types.hpp>

#include <steem/protocol/operation_util.hpp>
#include <steem/protocol/steem_operations.hpp>
#include <steem/protocol/steem_virtual_operations.hpp>
#include <steem/protocol/smt_operations.hpp>

namespace steem { namespace protocol {

   /** NOTE: do not change the order of any operations prior to the virtual operations
    * or it will trigger a hardfork.
    */
   typedef fc::static_variant<

            transfer_operation,
            transfer_to_vesting_operation,
            withdraw_vesting_operation,
            feed_publish_operation,

            account_create_operation,
            account_update_operation,
            account_delete_operation,

            witness_update_operation,
            account_witness_vote_operation,
            account_witness_proxy_operation,

            custom_operation,
            custom_json_operation,
            custom_binary_operation,

            report_over_production_operation,

            placeholder_a_operation,               // A new op can go here
            placeholder_b_operation,               // A new op can go here
            request_account_recovery_operation,
            recover_account_operation,
            change_recovery_account_operation,
            escrow_transfer_operation,
            escrow_dispute_operation,
            escrow_release_operation,
            escrow_approve_operation,

            reset_account_operation,
            set_reset_account_operation,

            application_create_operation,
            application_update_operation,
            application_delete_operation,
            buy_application_operation,
            cancel_application_buying_operation,
#ifdef STEEM_ENABLE_SMT
            claim_reward_balance2_operation,
#endif
            witness_set_properties_operation,
            witness_stop_operation,

#ifdef STEEM_ENABLE_SMT
            /// SMT operations
            smt_setup_operation,
            smt_cap_reveal_operation,
            smt_refund_operation,
            smt_setup_emissions_operation,
            smt_set_setup_parameters_operation,
            smt_set_runtime_parameters_operation,
            smt_create_operation,
#endif
            /// virtual operations below this point

            interest_operation,
            fill_vesting_withdraw_operation,
            shutdown_witness_operation,
            hardfork_operation,
            producer_reward_operation,
            promotion_pool_withdraw_operation,
            transfer_from_promotion_pool_operation
         > operation;

   /*void operation_get_required_authorities( const operation& op,
                                            flat_set<string>& active,
                                            flat_set<string>& owner,
                                            flat_set<string>& posting,
                                            vector<authority>&  other );

   void operation_validate( const operation& op );*/

   bool is_market_operation( const operation& op );

   bool is_virtual_operation( const operation& op );

} } // steem::protocol

/*namespace fc {
    void to_variant( const steem::protocol::operation& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  steem::protocol::operation& vo );
}*/

STEEM_DECLARE_OPERATION_TYPE( steem::protocol::operation )
FC_REFLECT_TYPENAME( steem::protocol::operation )
