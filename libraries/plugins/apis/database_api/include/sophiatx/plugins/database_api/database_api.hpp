#pragma once

#include <sophiatx/plugins/json_rpc/utility.hpp>

#include <sophiatx/plugins/database_api/database_api_args.hpp>
#include <sophiatx/plugins/database_api/database_api_objects.hpp>

namespace sophiatx { namespace plugins { namespace database_api {

class database_api_impl;

class database_api
{
   public:
      database_api();
      ~database_api();

      DECLARE_API(

         /////////////
         // Globals //
         /////////////

         /**
         * @brief Retrieve compile-time constants
         */
         (get_config)

         /**
         * @brief Retrieve the current @ref dynamic_global_property_object
         */
         (get_dynamic_global_properties)
         (get_witness_schedule)
         (get_hardfork_properties)
         (get_current_price_feed)
         (get_feed_history)

         ///////////////
         // Witnesses //
         ///////////////
         (list_witnesses)
         (find_witnesses)
         (list_witness_votes)
         (get_active_witnesses)

         //////////////
         // Accounts //
         //////////////

         /**
         * @brief List accounts ordered by specified key
         *
         */
         (list_accounts)
         /**
         * @brief Find accounts by primary key (account name)
         */
         (find_accounts)
         (list_owner_histories)
         (find_owner_histories)
         (list_account_recovery_requests)
         (find_account_recovery_requests)
         (list_change_recovery_account_requests)
         (find_change_recovery_account_requests)
         (list_escrows)
         (find_escrows)

         //////////////////
         // Applications //
         //////////////////

         (list_applications)
         (get_application_buyings)

         ////////////////////////////
         // Authority / validation //
         ////////////////////////////

         /// @brief Get a hexdump of the serialized binary form of a transaction
         (get_transaction_hex)

         /**
         *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
         *  and return the minimal subset of public keys that should add signatures to the transaction.
         */
         (get_required_signatures)

         /**
         *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
         *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
         *  to get the minimum subset.
         */
         (get_potential_signatures)

         /**
         * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
         */
         (verify_authority)

         /**
         * @return true if the signers have enough authority to authorize an account
         */
         (verify_account_authority)

         /*
          * This is a general purpose API that checks signatures against accounts for an arbitrary sha256 hash
          * using the existing authority structures in SophiaTX
          */
         (verify_signatures)

         /*
          * This is used to get remaining promotion pool balance
          */
         (get_promotion_pool_balance)

         /*
          * Get amount of SPHTX burned
          */
         (get_burned_balance)
      )

   private:
      std::unique_ptr< database_api_impl > my;
};

} } } //sophiatx::plugins::database_api

