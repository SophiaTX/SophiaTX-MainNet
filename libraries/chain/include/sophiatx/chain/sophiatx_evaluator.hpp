#pragma once

#include <sophiatx/protocol/sophiatx_operations.hpp>

#include <sophiatx/chain/evaluator.hpp>

namespace sophiatx { namespace chain {

using namespace sophiatx::protocol;

SOPHIATX_DEFINE_EVALUATOR( account_create )
SOPHIATX_DEFINE_EVALUATOR( account_update )
SOPHIATX_DEFINE_EVALUATOR( account_delete )
SOPHIATX_DEFINE_EVALUATOR( transfer )
SOPHIATX_DEFINE_EVALUATOR( transfer_to_vesting )
SOPHIATX_DEFINE_EVALUATOR( witness_update )
SOPHIATX_DEFINE_EVALUATOR( witness_stop )
SOPHIATX_DEFINE_EVALUATOR( account_witness_vote )
SOPHIATX_DEFINE_EVALUATOR( account_witness_proxy )
SOPHIATX_DEFINE_EVALUATOR( withdraw_vesting )
SOPHIATX_DEFINE_EVALUATOR( custom )
SOPHIATX_DEFINE_EVALUATOR( custom_json )
SOPHIATX_DEFINE_EVALUATOR( custom_binary )
SOPHIATX_DEFINE_EVALUATOR( feed_publish )
SOPHIATX_DEFINE_EVALUATOR( report_over_production )
SOPHIATX_DEFINE_EVALUATOR( escrow_transfer )
SOPHIATX_DEFINE_EVALUATOR( escrow_approve )
SOPHIATX_DEFINE_EVALUATOR( escrow_dispute )
SOPHIATX_DEFINE_EVALUATOR( escrow_release )
SOPHIATX_DEFINE_EVALUATOR( placeholder_a )
SOPHIATX_DEFINE_EVALUATOR( placeholder_b )
SOPHIATX_DEFINE_EVALUATOR( request_account_recovery )
SOPHIATX_DEFINE_EVALUATOR( recover_account )
SOPHIATX_DEFINE_EVALUATOR( change_recovery_account )

SOPHIATX_DEFINE_EVALUATOR( reset_account )
SOPHIATX_DEFINE_EVALUATOR( set_reset_account )

SOPHIATX_DEFINE_EVALUATOR( application_create )
SOPHIATX_DEFINE_EVALUATOR( application_update )
SOPHIATX_DEFINE_EVALUATOR( application_delete )
SOPHIATX_DEFINE_EVALUATOR( buy_application )
SOPHIATX_DEFINE_EVALUATOR( cancel_application_buying )

SOPHIATX_DEFINE_EVALUATOR( witness_set_properties )
SOPHIATX_DEFINE_EVALUATOR( transfer_from_promotion_pool )
SOPHIATX_DEFINE_EVALUATOR( sponsor_fees )
#ifdef SOPHIATX_ENABLE_SMT
SOPHIATX_DEFINE_EVALUATOR( smt_setup )
SOPHIATX_DEFINE_EVALUATOR( smt_cap_reveal )
SOPHIATX_DEFINE_EVALUATOR( smt_refund )
SOPHIATX_DEFINE_EVALUATOR( smt_setup_emissions )
SOPHIATX_DEFINE_EVALUATOR( smt_set_setup_parameters )
SOPHIATX_DEFINE_EVALUATOR( smt_set_runtime_parameters )
SOPHIATX_DEFINE_EVALUATOR( smt_create )
#endif

} } // sophiatx::chain
