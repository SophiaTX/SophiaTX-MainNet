#pragma once

#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_args.hpp>

#define ALEXANDRIA_API_SINGLE_QUERY_LIMIT 1000

namespace sophiatx { namespace plugins { namespace alexandria_api {

class alexandria_api_impl;

class alexandria_api
{
   public:
      alexandria_api();
      ~alexandria_api();

      void init();

      DECLARE_API(

//         /// alexandria api
//         (help)(gethelp)
//         (about)
//         (hello)
//
//         /// query api
//         (info)
         (list_witnesses)
         (list_witnesses_by_vote)
         (get_witness)
         (get_block)
         (get_ops_in_block)
         (get_feed_history)
         (get_application_buyings)
         (get_applications)
//         (get_applications_by_ids)
//         (get_received_documents)
//         (get_active_witnesses)
//         (get_transaction)
//         (get_required_signatures)
//
//         ///account api
//         (get_account_name_from_seed)
//         (account_exist)
//         (get_account)
//         (get_account_history)
//         (get_active_authority)
//         (get_owner_authority)
//         (get_memo_key)
//         (get_account_balance)
//         (get_vesting_balance)
//         (create_simple_authority)
//         (create_simple_multisig_authority)
//         (create_simple_managed_authority)
//         (create_simple_multisig_managed_authority)
//
//         /// transaction api
//         (create_account)
//         (update_account)
//         (delete_account)
//         (update_witness)
//         (set_voting_proxy)
//         (vote_for_witness)
//         (transfer)
//         (sponsor_account_fees)
//
//         (transfer_to_vesting)
//         (withdraw_vesting)
//         (create_application)
//         (update_application)
//         (delete_application)
//         (buy_application)
//         (cancel_application_buying)
//
//         (make_custom_json_operation)
//         (make_custom_binary_operation)
//
//         /// helper api
//         (broadcast_transaction)
//         (create_transaction)
//         (create_simple_transaction)
//         (calculate_fee)
//         (fiat_to_sphtx)
//
//         ///local api
//         (get_transaction_digest)
//         (add_signature)
//         (add_fee)
//
//         (sign_digest)
//         (send_and_sign_operation)
//         (send_and_sign_transaction)
//         (verify_signature)
//         (generate_key_pair)
//         (generate_key_pair_from_brain_key)
//         (get_public_key)
//         (from_base64)
//         (to_base64)
//         (encrypt_data)
//         (decrypt_data)
//         (custom_object_subscription)

      )

   private:
      std::unique_ptr< alexandria_api_impl > my;
};

} } } //sophiatx::plugins::alexandria_api

