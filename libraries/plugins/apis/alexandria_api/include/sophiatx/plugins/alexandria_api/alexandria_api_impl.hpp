#pragma once

#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_args.hpp>

#include <sophiatx/plugins/database_api/database_api.hpp>
#include <sophiatx/plugins/block_api/block_api.hpp>
#include <sophiatx/plugins/account_history_api/account_history_api.hpp>
#include <sophiatx/plugins/account_by_key_api/account_by_key_api.hpp>
#include <sophiatx/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <sophiatx/plugins/custom_api/custom_api.hpp>
#include <sophiatx/plugins/subscribe_api/subscribe_api.hpp>
#include <sophiatx/plugins/witness_api/witness_api.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

class alexandria_api_impl {
public:
   alexandria_api_impl();
   ~alexandria_api_impl();

   /**
    * Getters and setters
    */
   chain::database &get_db() const;

   const shared_ptr<block_api::block_api> &get_block_api() const;
   void set_block_api(const shared_ptr<block_api::block_api> &block_api);

   const shared_ptr<database_api::database_api> &get_database_api() const;
   void set_database_api(const shared_ptr<database_api::database_api> &database_api);

   const shared_ptr<account_history::account_history_api> &get_account_history_api() const;
   void set_account_history_api(const shared_ptr<account_history::account_history_api> &account_history_api);

   const shared_ptr<account_by_key::account_by_key_api> &get_account_by_key_api() const;
   void set_account_by_key_api(const shared_ptr<account_by_key::account_by_key_api> &account_by_key_api);

   const shared_ptr<network_broadcast_api::network_broadcast_api> &get_network_broadcast_api() const;
   void set_network_broadcast_api(const shared_ptr<network_broadcast_api::network_broadcast_api> &network_broadcast_api);

   const shared_ptr<witness::witness_api> &get_witness_api() const;
   void set_witness_api(const shared_ptr<witness::witness_api> &witness_api);

   const shared_ptr<custom::custom_api> &get_custom_api() const;
   void set_custom_api(const shared_ptr<custom::custom_api> &custom_api);

   const shared_ptr<subscribe::subscribe_api> &get_subscribe_api() const;
   void set_subscribe_api(const shared_ptr<subscribe::subscribe_api> &subscribe_api);

   /**
    * API methods declarations
    */
   DECLARE_API_IMPL (

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
//         (get_applications)
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

   chain::database &_db;

private:
   std::shared_ptr< database_api::database_api >                     _database_api;
   std::shared_ptr< block_api::block_api >                           _block_api;
   std::shared_ptr< account_history::account_history_api >           _account_history_api;
   std::shared_ptr< account_by_key::account_by_key_api >             _account_by_key_api;
   std::shared_ptr< network_broadcast_api::network_broadcast_api >   _network_broadcast_api;
   std::shared_ptr< witness::witness_api >                           _witness_api;
   std::shared_ptr< custom::custom_api >                             _custom_api;
   std::shared_ptr< subscribe::subscribe_api >                       _subscribe_api;


   /**
    * @brief Checks if concrete api shared_ptr is valid, it asserts if not
    *
    * @tparam T type of api to be checked
    * @param api to be checked
    */
   template<typename T>
   void checkApiEnabled(const std::shared_ptr< T >& api) {
      FC_ASSERT(api != nullptr, "API: " + std::string(typeid(T).name()) + " plugin not enabled.");
   }

};

} } } // sophiatx::plugins::alexandria_api
