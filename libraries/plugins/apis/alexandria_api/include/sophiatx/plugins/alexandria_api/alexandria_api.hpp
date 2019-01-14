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
        /**
         * Returns info about the current state of the blockchain
         */
        (info)

        /** Returns info such as client version,git version of graphene/fc,version of boost,openssl.
         * @returns compile time info and client and dependencies versions
         */
        (about)

        /**
         * Returns the information about a block
         * @param num Block num
         * @returns Public block data on the blockchain
         */
        (get_block)

        /**
         * Returns sequence of operations included/generated in a specified block
         * @param block_num Block height of specified block
         * @param only_virtual Whether to only return virtual operations
         */
        (get_ops_in_block)

        /** Return the current price feed history
         * @returns Price feed history data on the blockchain
         */
        (get_feed_history)

        /**
         * Returns the list of witnesses producing blocks in the current round (51 Blocks)
         */
        (get_active_witnesses)

        /**
         * Returns information about the given account.
         * @param account_name the name of the account to provide information about i.e. "account_name"
         * @returns the public account data stored in the blockchain
         */
        (get_account)

        /**
         *
         */
        (get_accounts)

        /**
         * Returns transaction by ID.
         */
        (get_transaction)

        /**
         * This method is used by faucets to create new accounts for other users which must
         * provide their desired keys. The resulting account may not be controllable by this
         * alexandria. There is a fee associated with account creation that is paid by the creator.
         * The current account creation fee can be found with the 'info' alexandria command.
         * @param creator The account creating the new account i.e. "account_name"
         * @param seed The seed to generate the new account name
         * @param json_meta JSON Metadata associated with the new account
         * @param owner public owner key of the new account
         * @param active public active key of the new account
         * @param memo public memo key of the new account
         */
        (create_account)

        /**
         * This method updates the keys of an existing account.
         * @param accountname The name of the account i.e. "account_name"
         * @param json_meta New JSON Metadata to be associated with the account
         * @param owner New public owner key for the account
         * @param active New public active key for the account
         * @param memo New public memo key for the account
         */
        (update_account)

        /**
         * This method updates authority for an exisiting account.
         * Warning: You can create impossible authorities using this method. The method
         * will fail if you create an impossible owner authority, but will allow impossible
         * active authorities.
         *
         * @param account_name The name of the account whose authority you wish to update
         * @param type The authority type. e.g. owner or active
         * @param authority That will be set for specified type
         */
        (update_account_auth)

        /**
         * This method deletes an existing account.
         * @param account_name The name of the account you wish to delete i.e. "account_name"
         */
        (delete_account)

        /**
         *  This method is used to convert a JSON transaction to its transaction ID.
         */
        (get_transaction_id)

        /**
         * Lists all witnesses registered in the blockchain.
         * This returns a list of all account names that own witnesses,and the associated witness id,
         * sorted by name.  This lists witnesses whether they are currently voted in or not.
         * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
         * start by setting \c lowerbound to the empty string \c "",and then each iteration,pass
         * the last witness name returned as the \c lowerbound for the next \c list_witnesss() call.
         * @param lowerbound the name of the first witness to return.  If the named witness does not exist,
         *                   the list will start at the witness that comes after \c lowerbound
         * @param limit the maximum number of witnesss to return (max: 1000)
         * @returns a list of witnesss mapping witness names to witness ids
         */
        (list_witnesses)
        (list_witnesses_by_vote)

        /** Returns information about the given witness.
         * @param owner_account the name or id of the witness account owner i.e. "account_name",or the id of the witness
         * @returns the information about the witness stored in the block chain
         */
        (get_witness)

        /**
         * Update a witness object owned by the given account.
         * @param witness_name The name of the witness account.
         * @param url A URL containing some information about the witness.  The empty string makes it remain the same.
         * @param block_signing_key The new block signing public key.  The empty string disables block production.
         * @param props The chain properties the witness is voting on.
         */
        (update_witness)

        /**
         * Stop being a witness,effectively deleting the witness object owned by the given account.
         * @param witness_name The name of the witness account i.e. "account_name"
         */
        (stop_witness)

        /** Set the voting proxy for an account.
         * If a user does not wish to take an active part in voting,they can choose
         * to allow another account to vote their stake.
         * Setting a vote proxy does not remove your previous votes from the blockchain,
         * they remain there but are ignored.  If you later null out your vote proxy,
         * your previous votes will take effect again.
         * This setting can be changed at any time.
         * @param account_to_modify the name or id of the account to update i.e. "account_name"
         * @param proxy the name of account that should proxy to,or empty string to have no proxy
         */
        (set_voting_proxy)

        /**
         * Vote for a witness to become a block producer. By default an account has not voted
         * positively or negatively for a witness. The account can either vote for with positively
         * votes or against with negative votes. The vote will remain until updated with another
         * vote. Vote strength is determined by the accounts vesting shares.
         * @param account_to_vote_with The account voting for a witness i.e. "account_name"
         * @param witness_to_vote_for The witness that is being voted for i.e. "account_name"
         * @param approve true if the account is voting for the account to be able to be a block produce
         */
        (vote_for_witness)

        /**
         * Transfer funds from one account to another. SPHTX can be transferred.
         * @param from The account the funds are coming from i.e. "account_name"
         * @param to The account the funds are going to i.e. "account_name"
         * @param amount The funds being transferred. i.e. "100.000 SPHTX"
         * @param memo A memo for the transaction,encrypted with the to account's public memo key       */
        (transfer)

        /**
         * Transfer SPHTX into a vesting fund represented by vesting shares (VESTS). VESTS are required to vesting
         * for a minimum of one coin year and can be withdrawn once a week over a two year withdraw period.
         * VESTS are protected against dilution up until 90% of SPHTX is vesting.
         * @param from The account the SPHTX is coming from i.e. "account_name"
         * @param to The account getting the VESTS i.e. "account_name"
         * @param amount The amount of SPHTX to vest i.e. "100.00 SPHTX"
         */
        (transfer_to_vesting)

        /**
         * Set up a vesting withdraw request. The request is fulfilled once a week over the next two year (104 weeks).
         * @param from The account the VESTS are withdrawn from i.e. "account_name"
         * @param vesting_shares The amount of VESTS to withdraw over the next two years. Each week (amount/104) shares are
         *    withdrawn and deposited back as SPHTX. i.e. "10.000000 VESTS"
         */
        (withdraw_vesting)
        (get_owner_history)

        /**
         *  This method will create new application object. There is a fee associated with account creation
         *  that is paid by the creator. The current account creation fee can be found with the
         *  'info' alexandria command.
         *  @param author The account creating the new application i.e. "account_name"
         *  @param app_name The unique name for new application
         *  @param url The url of the new application
         *  @param meta_data The meta data of new application
         *  @param price_param The price parameter that specifies billing for the app
         */
        (create_application)

        /**
         *  This method will update existing application object.
         *  @param author The author of application i.e. "account_name"
         *  @param app_name The name of app that will be updated
         *  @param new_author The new author i.e. "account_name"
         *  @param url Updated url
         *  @param meta_data Updated meta data
         *  @param price_param Updated price param
         */
        (update_application)

        /**
         *  This method will delete specified application object.
         *  @param author The author of application that will be deleted i.e. "account_name"
         *  @param app_name The name of app that will be deleted
         */
        (delete_application)

        /**
         *  This method will create application buy object
         *  @param buyer The buyer of application i.e. "account_name"
         *  @param app_id The id of app that buyer will buy
         */
        (buy_application)

        /**
         *  This method will cancel application buy object
         *  @param app_owner The owner of bought application i.e. "account_name"
         *  @param buyer The buyer of application i.e. "account_name"
         *  @param app_id The id of bought app
         */
        (cancel_application_buying)

        /**
         * Get all app buyings by app_name or buyer
         * @param name Application id or buyers name i.e. "account_name"
         * @param search_type One of "by_buyer","by_app_id"
         * @param count Number of items to retrieve
         * @return
         */
        (get_application_buyings)

        /**
         * Send custom JSON dataget_application_buyings
         * @param app_id Application ID
         * @param from Sender i.e. "account_name"
         * @param to List of receivers i.e. ["account_name"]
         * @param json Data formatted in JSON
         * @return
         */
        (make_custom_json_operation)

        /**
         * Send custom data data
         * @param app_id Application ID
         * @param from Sender i.e. "account_name"
         * @param to List of receivers i.e. ["account_name"]
         * @param data Data formatted in base58.
         * @return
         */
        (make_custom_binary_operation)


        /**
         * Broadcast transaction to node
         * @param tx Signed transaction to be broadcasts
         * @return transaction with block information
         */
        (broadcast_transaction)

        /**
         * Creating single operation form vector of operations
         * @param op_vec Vector of operations that should be in this transaction
         * @return single transaction with all the operations
         */
        (create_transaction)

        /**
         * Creating single operation form operation
         * @param op operation that should be in this transaction
         * @return single transaction with all the operations
         */
        (create_simple_transaction)

        /**
         * Get all app objects
         * @param names - array of names of applications
         * @return array of application objects
         */
        (get_applications)

        /**
         * Get all app objects
         * @param ids - array of ids of applications
         * @return array of application objects
         */
        (get_applications_by_ids)

        /**
         * Calculates digest of provided transaction
         * @param tx - transaction to be digested
         * @return transaction digest
         */
        (get_transaction_digest)

        /**
         * Adds signature to transaction
         * @param tx - transaction to be signed
         * @param signature - signature that will be add to transaction
         * @return signed transaction
         */
        (add_signature)

        /**
         * Add custom fee to the operation
         * @param op Operation where the fee is added
         * @param fee Fee to be added
         */
        (add_fee)

        /**
         * Sign digest with providet private key
         * @param digest - digest fo transaction
         * @param pk - private key for signing (in WIF format)
         * @return signature of digest
         */
        (sign_digest)

        /**
         * This function will create transaction of this operation,sign it with key and broadcast to node
         * @param op - operation to be send
         * @param pk - private key for signing
         */
        (send_and_sign_operation)

        /**
         * This function will sign and broadcast transaction
         * @param tx - transaction to be send
         * @param pk - private key for signing
         */
        (send_and_sign_transaction)

        /**
         * Verify signature
         * @param digest - digest corresponding to signature
         * @param pub_key - public key corresponding to private_key,that signed digest
         * @param signature - signature to be verified
         * @return true if is valid
         */
        (verify_signature)

        /**
         * Generates key pair
         * @return pair of keys
         */
        (generate_key_pair)

        /**
         * Generates key pair based on brain key
         * @param brain_key - brain key for generating key pair
         * @return pair of keys
         */
        (generate_key_pair_from_brain_key)

        /**
         * Returns public key to provided private key
         * @param private_key
         * @return
         */
        (get_public_key)

        /**
         * Decode data to base64
         * @param data - data to decode
         * @return
         */
        (from_base64)

        /**
         * Encode data to base64
         * @param data - data to encode
         * @return
         */
        (to_base64)

        /**
         * Encrypt data
         * @param data - data to encrypt
         * @param public_key - public key of recipient
         * @param private_key - private key of sender
         * @return encrypted data
         */
        (encrypt_data)

        /**
         * Decrypt data
         * @param data - data to decrypt
         * @param public_key - public key of sender
         * @param private_key - private key of recipient
         * @return decrypted data
         */
        (decrypt_data)

        /**
         * Check if account account exists
         * @param account_name - name of the account
         * @return returns true if account exists
         */
        (account_exist)

        /**
         *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
         *  returns operations in the range [from-limit,from]
         *  @param account - account whose history will be returned
         *  @param from - the absolute sequence number,-1 means most recent,limit is the number of operations before from.
         *  @param limit - the maximum number of items that can be queried (0 to 1000],must be less than from
         */
        (get_account_history)

        /**
         * Get all received custom jsons and data.
         * @param app_id Application ID
         * @param account_name Name of the relevant (sender/recipient) account
         * @param search_type One of "by_sender","by_recipient","by_sender_datetime","by_recipient_datetime"
         * @param start Either timestamp in ISO format or index
         * @param count Number of items to retrieve
         * @return
         */
        (get_received_documents)

        /**
         * Returns active authority for given account
         * @param account_name - account name i.e. "account_name"
         */
        (get_active_authority)

        /**
         * Returns owner authority for given account
         * @param account_name - account name i.e. "account_name"
         */
        (get_owner_authority)

        /**
         * Returns memo key for given account
         * @param account_name - account name i.e. "account_name"
         */
        (get_memo_key)

        /**
         * Returns current balance for given account
         * @param account_name - account name i.e. "account_name"
         */
        (get_account_balance)

        /**
         * Returns vestig balance for given account
         * @param account_name - account name i.e. "account_name"
         */
        (get_vesting_balance)

        /**
         * Creates simple authority object from provided public key
         * @param pub_key
         */
        (create_simple_authority)

        /**
         * Creates simple multisig authority object from provided public key
         * @param pub_keys - vector of public keys
         * @param required_signatures  - number of required signatures
         * @return
         */
        (create_simple_multisig_authority)

        /**
         * Creates simple managed authority from provided account_name
         * @param managing_account -  i.e. "account_name"
         */
        (create_simple_managed_authority)

        /**
         * Creates simple multisig managed authority from provided account_name
         * @param managing_accounts - vector of accounts  i.e. ["account_name"]
         * @param required_signatures - number of required signatures
         */
        (create_simple_multisig_managed_authority)

        /**
         * Converts seed to new account name
         * @param seed Seed
         * @return new account name
         */
        (get_account_name_from_seed)

        /**
         * Returns set of public key (authorities) required for signing specific transaction
         * @param tx - transaction for signing
         */
        (get_required_signatures)


        /**
         * Returns a fee for the given operation
         * @param op Operation to evaluate
         * @param symbol Symbol of the fee paying currency
         * @return fee
         */
        (calculate_fee)

        /**
         * Converts the given amount of fiat to sphtx
         * @param fiat The amount to be converted
         * @return Amount of SPHTX if conversion is possible,or returns back fiat if not.
         */
        (fiat_to_sphtx)
        (custom_object_subscription)

        /**
         * Creates sonsor operation
         * @param sponsoring_account - account name that will sponsor fees
         * @param sponsored_account - account that will be sponsored
         * @param is_sponsoring - true or false if you want to enable/disable this operation
         * @return
         */
        (sponsor_account_fees)

        /**
         *  Gets the account information for all accounts specified provided public keys
         */
        (get_key_references)

        /**
         *  Gets blockchain_version, sophiatx_revision, fc_revision, chain_id
         */
        (get_version)

        /**
         *  Gets current dynamic global properties
         */
        (get_dynamic_global_properties)

        /**
         *  Gets current witness schedule object
         */
        (get_witness_schedule_object)

        /**
         *  Gets current hardfork property object
         */
        (get_hardfork_property_object)
      )

   private:
      std::unique_ptr< alexandria_api_impl > my;
};

} } } //sophiatx::plugins::alexandria_api