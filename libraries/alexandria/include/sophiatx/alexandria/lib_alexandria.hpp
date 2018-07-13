#pragma once

#include <sophiatx/alexandria/remote_node_api.hpp>

#include <sophiatx/utilities/key_conversion.hpp>

#include <fc/macros.hpp>
#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/api.hpp>
#include <sophiatx/plugins/condenser_api/condenser_api.hpp>

namespace sophiatx { namespace alexandria {

using namespace std;

using namespace sophiatx::utilities;
using namespace sophiatx::protocol;


struct brain_key_info
{
   string               brain_priv_key;
   public_key_type      pub_key;
   string               wif_priv_key;
};

struct key_pair
{
   public_key_type      pub_key;
   string               wif_priv_key;
};

enum authority_type
{
   owner,
   active
};

struct memo_data {

   static fc::optional<memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(memo_data)) {
            auto data = fc::from_base58( str );
            auto m  = fc::raw::unpack_from_vector<memo_data>( data );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return fc::optional<memo_data>();
   }

   int64_t         nonce = 0;
   uint64_t        check = 0;
   vector<char>    encrypted;

   operator string()const {
      auto data = fc::raw::pack_to_vector(*this);
      auto base58 = fc::to_base58( data );
      return base58;
   }
};

namespace detail {
class alexandria_api_impl;
}

class alexandria_api
{
   public:
      alexandria_api( fc::api< remote_node_api > rapi );
      virtual ~alexandria_api();

      /** Returns a list of all commands supported by the alexandria API.
       *
       * This lists each command, along with its arguments and return types.
       * For more detailed help on a single command, use \c get_help()
       *
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string                              help()const;

      /**
       * Returns info about the current state of the blockchain
       */
      variant                             info();

      /** Returns info such as client version, git version of graphene/fc, version of boost, openssl.
       * @returns compile time info and client and dependencies versions
       */
      variant_object                      about();

      /** Returns the information about a block
       *
       * @param num Block num
       *
       * @returns Public block data on the blockchain
       */
      optional< database_api::api_signed_block_object > get_block( uint32_t num );

      /** Returns sequence of operations included/generated in a specified block
       *
       * @param block_num Block height of specified block
       * @param only_virtual Whether to only return virtual operations
       */
      vector< condenser_api::api_operation_object > get_ops_in_block( uint32_t block_num, bool only_virtual = true );

      /** Return the current price feed history
       *
       * @returns Price feed history data on the blockchain
       */
      condenser_api::api_feed_history_object get_feed_history( string symbol)const;

      /**
       * Returns the list of witnesses producing blocks in the current round (21 Blocks)
       */
      vector< account_name_type > get_active_witnesses()const;

      /** Returns information about the given account.
       *
       * @param account_name the name of the account to provide information about
       * @returns the public account data stored in the blockchain
       */
      condenser_api::api_account_object get_account( string account_name ) const;

      /**
       * Returns transaction by ID.
       */
      annotated_signed_transaction get_transaction( transaction_id_type trx_id )const;

      /** Returns detailed help on a single API command.
       * @param method the name of the API command you want help with
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  gethelp(const string& method)const;

      /**
       * This method is used by faucets to create new accounts for other users which must
       * provide their desired keys. The resulting account may not be controllable by this
       * alexandria. There is a fee associated with account creation that is paid by the creator.
       * The current account creation fee can be found with the 'info' alexandria command.
       *
       * @param creator The account creating the new account
       * @param seed The seed to generate the new account name
       * @param json_meta JSON Metadata associated with the new account
       * @param owner public owner key of the new account
       * @param active public active key of the new account
       * @param memo public memo key of the new account
       */
      operation create_account( string creator,
                                string seed,
                                string json_meta,
                                public_key_type owner,
                                public_key_type active,
                                public_key_type memo)const;



      /**
       * This method updates the keys of an existing account.
       *
       * @param accountname The name of the account
       * @param json_meta New JSON Metadata to be associated with the account
       * @param owner New public owner key for the account
       * @param active New public active key for the account
       * @param memo New public memo key for the account
       */
      operation update_account( string accountname,
                                         string json_meta,
                                         public_key_type owner,
                                         public_key_type active,
                                         public_key_type memo )const;

     /**
      * This method deletes an existing account.
      *
      * @param account_name The name of the account you wish to delete
      */
      operation delete_account( string account_name);

      /**
       *  This method is used to convert a JSON transaction to its transaction ID.
       */
      transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }

      /** Lists all witnesses registered in the blockchain.
       * This returns a list of all account names that own witnesses, and the associated witness id,
       * sorted by name.  This lists witnesses whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last witness name returned as the \c lowerbound for the next \c list_witnesss() call.
       *
       * @param lowerbound the name of the first witness to return.  If the named witness does not exist,
       *                   the list will start at the witness that comes after \c lowerbound
       * @param limit the maximum number of witnesss to return (max: 1000)
       * @returns a list of witnesss mapping witness names to witness ids
       */
      vector< account_name_type > list_witnesses(const string& lowerbound, uint32_t limit);

      /** Returns information about the given witness.
       * @param owner_account the name or id of the witness account owner, or the id of the witness
       * @returns the information about the witness stored in the block chain
       */
      optional< condenser_api::api_witness_object > get_witness(string owner_account);

      /**
       * Update a witness object owned by the given account.
       *
       * @param witness_name The name of the witness account.
       * @param url A URL containing some information about the witness.  The empty string makes it remain the same.
       * @param block_signing_key The new block signing public key.  The empty string disables block production.
       * @param props The chain properties the witness is voting on.
       */
      operation update_witness(string witness_name,
                                string url,
                                public_key_type block_signing_key,
                                const chain_properties& props);

      /**
       * Stop being a witness, effectively deleting the witness object owned by the given account.
       *
       * @param witness_name The name of the witness account.
       */
      operation stop_witness(string witness_name);

      /** Set the voting proxy for an account.
       *
       * If a user does not wish to take an active part in voting, they can choose
       * to allow another account to vote their stake.
       *
       * Setting a vote proxy does not remove your previous votes from the blockchain,
       * they remain there but are ignored.  If you later null out your vote proxy,
       * your previous votes will take effect again.
       *
       * This setting can be changed at any time.
       *
       * @param account_to_modify the name or id of the account to update
       * @param proxy the name of account that should proxy to, or empty string to have no proxy
       */
      operation set_voting_proxy(string account_to_modify, string proxy);

      /**
       * Vote for a witness to become a block producer. By default an account has not voted
       * positively or negatively for a witness. The account can either vote for with positively
       * votes or against with negative votes. The vote will remain until updated with another
       * vote. Vote strength is determined by the accounts vesting shares.
       *
       * @param account_to_vote_with The account voting for a witness
       * @param witness_to_vote_for The witness that is being voted for
       * @param approve true if the account is voting for the account to be able to be a block produce
       */
      operation vote_for_witness(string account_to_vote_with,
                                          string witness_to_vote_for,
                                          bool approve = true);

      /**
       * Transfer funds from one account to another. sophiatx can be transferred.
       *
       * @param from The account the funds are coming from
       * @param to The account the funds are going to
       * @param amount The funds being transferred. i.e. "100.000 sophiatx"
       * @param memo A memo for the transaction, encrypted with the to account's public memo key       */
      operation transfer(string from, string to, asset amount, string memo);

      /**
       * Transfer sophiatx into a vesting fund represented by vesting shares (VESTS). VESTS are required to vesting
       * for a minimum of one coin year and can be withdrawn once a week over a two year withdraw period.
       * VESTS are protected against dilution up until 90% of sophiatx is vesting.
       *
       * @param from The account the sophiatx is coming from
       * @param to The account getting the VESTS
       * @param amount The amount of sophiatx to vest i.e. "100.00 sophiatx"
       */
      operation transfer_to_vesting(string from, string to, asset amount);

        /**
        * Set up a vesting withdraw request. The request is fulfilled once a week over the next two year (104 weeks).
        *
        * @param from The account the VESTS are withdrawn from
        * @param vesting_shares The amount of VESTS to withdraw over the next two years. Each week (amount/104) shares are
        *    withdrawn and deposited back as sophiatx. i.e. "10.000000 VESTS"
        */
      operation withdraw_vesting( string from, asset vesting_shares);

      vector< database_api::api_owner_authority_history_object > get_owner_history( string account )const;

      /**
      *  This method will create new application object. There is a fee associated with account creation
      *  that is paid by the creator. The current account creation fee can be found with the
      *  'info' alexandria command.
      *
      *  @param author The account creating the new application
      *  @param app_name The unique name for new application
      *  @param url The url of the new application
      *  @param meta_data The meta data of new application
      *  @param price_param The price parameter that specifies billing for the app
      */
      operation create_application( string author, string app_name, string url, string meta_data, uint8_t price_param);

      /**
      *  This method will update existing application object.
      *
      *  @param author The author of application
      *  @param app_name The name of app that will be updated
      *  @param new_author The new author
      *  @param url Updated url
      *  @param meta_data Updated meta data
      *  @param price_param Updated price param
      */
      operation update_application( string author, string app_name, string new_author, string url, string meta_data,
                                    uint8_t price_param);

      /**
      *  This method will delete specified application object.
      *
      *  @param author The author of application that will be deleted
      *  @param app_name The name of app that will be deleted
      */
      operation delete_application( string author, string app_name);

      /**
      *  This method will create application buy object
      *
      *  @param buyer The buyer of application
      *  @param app_id The id of app that buyer will buy
      */
      operation buy_application( string buyer, int64_t app_id);

      /**
      *  This method will cancel application buy object
      *
      *  @param app_owner The owner of bought application
      *  @param buyer The buyer of application
      *  @param app_id The id of bought app
      */
      operation cancel_application_buying( string app_owner, string buyer, int64_t app_id);

      /**
       * Get all app buyings by app_name or buyer
       * @param name Application id or buyers name
       * @param search_type One of "by_buyer", "by_app_id"
       * @param count Number of items to retrieve
       * @return
       */
      vector< condenser_api::api_application_buying_object >  get_application_buyings(string name, string search_type, uint32_t count);

      std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

      std::shared_ptr<detail::alexandria_api_impl> my;

      /**
       * Send custom JSON data
       * @param app_id Application ID
       * @param from Sender
       * @param to List of receivers
       * @param json Data formatted in JSON
       * @return
       */
      operation make_custom_json_operation(uint32_t app_id, string from, vector<string> to, string json);

      /**
       * Send custom data data
       * @param app_id Application ID
       * @param from Sender
       * @param to List of receivers
       * @param data Data formatted in base58.
       * @return
       */
      operation make_custom_binary_operation(uint32_t app_id, string from, vector<string> to, string data);

      /**
       * Get all received custom jsons and data.
       * @param app_id Application ID
       * @param account_name Name of the relevant (sender/recipient) account
       * @param search_type One of "by_sender", "by_recipient", "by_sender_datetime", "by_recipient_datetime"
       * @param start Either timestamp in ISO format or index
       * @param count Number of items to retrieve
       * @return
       */
      map< uint64_t, condenser_api::api_received_object >  get_received_documents(uint32_t app_id, string account_name, string search_type, string start, uint32_t count);

      /**
       * Broadcast transaction to node
       * @param tx Signed transaction to be broadcasts
       * @return transaction with block information
       */
      annotated_signed_transaction broadcast_transaction(signed_transaction tx) const;

      /**
       * Creating single operation form vector of operations
       * @param op_vec Vector of operations that should be in this transaction
       * @return single transaction with all the operations
       */
      signed_transaction create_transaction(vector<operation> op_vec) const;

      /**
       * Creating single operation form operation
       * @param op operation that should be in this transaction
       * @return single transaction with all the operations
       */
      signed_transaction create_simple_transaction(operation op) const;

      /**
      * Get all app objects
      * @param names - array of names of applications
      * @return array of application objects
      */
      vector< condenser_api::api_application_object >  get_applications(vector<string> names);

      /**
       * Calculates digest of provided transaction
       * @param tx - transaction to be digested
       * @return transaction digest
       */
      digest_type get_transaction_digest(signed_transaction tx);

      /**
       * Adds signature to transaction
       * @param tx - transaction to be signed
       * @param signature - signature that will be add to transaction
       * @return signed transaction
       */
      signed_transaction add_signature(signed_transaction tx, fc::ecc::compact_signature signature) const;

      /**
       * Sign digest with providet private key
       * @param digest - digest fo transaction
       * @param pk - private key for signing (in WIF format)
       * @return signature of digest
       */
      fc::ecc::compact_signature sign_digest(digest_type digest, string pk) const;

      /**
       * This function will create transaction of this operation, sign it with key and broadcast to node
       * @param op - operation to be send
       * @param pk - private key for signing
       */
      annotated_signed_transaction send_and_sign_operation(operation op, string pk);

      /**
       * This function will sign and broadcast transaction
       * @param tx - transaction to be send
       * @param pk - private key for signing
       */
      annotated_signed_transaction send_and_sign_transaction(signed_transaction tx, string pk);

      /**
       * Verify signature
       * @param digest - digest corresponding to signature
       * @param pub_key - public key corresponding to private_key, that signed digest
       * @param signature - signature to be verified
       * @return true if is valid
       */
      bool verify_signature(digest_type digest, public_key_type pub_key, fc::ecc::compact_signature signature) const;

      /**
       * Generates key pair
       * @return pair of keys
       */
      key_pair generate_key_pair() const;

      /**
       * Generates key pair based on brain key
       * @param brain_key - brain key for generating key pair
       * @return pair of keys
       */
      key_pair generate_key_pair_from_brain_key(string brain_key) const;

      /**
       * Returns public key to provided private key
       * @param private_key
       * @return
       */
      public_key_type get_public_key(string private_key) const;

      /**
       * Decode data to base58
       * @param data - data to decode
       * @return
       */
      std::vector<char> from_base58(string data) const;

      /**
       * Encode data to base58
       * @param data - data to encode
       * @return
       */
      string to_base58(std::vector<char> data) const;

      /**
       * Encrypt data
       * @param data - data to encrypt
       * @param public_key - public key of recipient
       * @param private_key - private key of sender
       * @return encrypted data
       */
      string encrypt_data(string data, public_key_type public_key, string private_key) const;

      /**
       * Decrypt data
       * @param data - data to decrypt
       * @param public_key - public key of sender
       * @param private_key - private key of recipient
       * @return decrypted data
       */
      string decrypt_data(string data, public_key_type public_key, string private_key) const;

      /**
       * Check if account account exists
       * @param account_name - name of the account
       * @return returns true if account exists
       */
      bool account_exist(string account_name) const;

      /**
      *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
      *  returns operations in the range [from-limit, from]
      *
      *  @param account - account whose history will be returned
      *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
      *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
      */
      map< uint32_t, condenser_api::api_operation_object > get_account_history( string account, uint32_t from, uint32_t limit );

      /**
       * Returns active authority for given account
       * @param account_name - account name
       */
      authority get_active_authority(string account_name) const;

      /**
       * Returns owner authority for given account
       * @param account_name
       */
      authority get_owner_authority(string account_name) const;

      /**
       * Returns memo key for given account
       * @param account_name
        */
      public_key_type get_memo_key(string account_name) const;

      /**
       * Returns current balance for given account
       * @param account_name
       */
      int64_t get_account_balance(string account_name) const;

      /**
       * Returns vestig balance for given account
       * @param account_name
       */
      int64_t get_vesting_balance(string account_name) const;

      /**
       * Creates simple authority object from provided public key
       * @param pub_key
       */
      authority create_simple_authority(public_key_type pub_key) const;

      /**
       * Creates simple multisig authority object from provided public key
       * @param pub_keys - vector of public keys
       * @param required_signatures  - number of required signatures
       * @return
       */
      authority create_simple_multisig_authority(vector<public_key_type> pub_keys, uint32_t required_signatures) const;

      /**
       * Creates simple managed authority from provided account_name
       * @param managing_account
       */
      authority create_simple_managed_authority(string managing_account) const;

      /**
       * Creates simple multisig managed authority from provided account_name
       * @param managing_accounts - vector of accounts
       * @param required_signatures - number of required signatures
       */
      authority create_simple_multisig_managed_authority(vector<string> managing_accounts, uint32_t required_signatures) const;

      /**
       * Converts seed to new account name
       * @param seed Seed
       * @return new account name
       */
      string get_account_name_from_seed(string seed) const;

      /**
       * Returns set of public key (authorities) required for signing specific transaction
       * @param tx - transaction for signing
       */
      set< public_key_type > get_required_signatures( signed_transaction tx) const;


      /**
       * Returns a fee for the given operation
       * @param op Operation to evaluate
       * @param symbol Symbol of the fee paying currency
       * @return fee
       */
       asset calculate_fee(operation op, asset_symbol_type symbol)const;

       /**
        * Converts the given amount of fiat to sphtx
        * @param fiat The amount to be converted
        * @return Amount of SPHTX if conversion is possible, or returns back fiat if not.
        */
       asset fiat_to_sphtx(asset fiat)const;
};

} }

FC_REFLECT_ENUM( sophiatx::alexandria::authority_type, (owner)(active) )
FC_REFLECT( sophiatx::alexandria::key_pair, (pub_key)(wif_priv_key) )
FC_REFLECT( sophiatx::alexandria::memo_data, (nonce)(check)(encrypted) )

FC_API( sophiatx::alexandria::alexandria_api,
        /// alexandria api
        (help)(gethelp)
        (about)

        /// query api
        (info)
        (list_witnesses)
        (get_witness)
        (get_block)
        (get_ops_in_block)
        (get_feed_history)
        (get_application_buyings)
        (get_applications)
        (get_received_documents)
        (get_active_witnesses)
        (get_transaction)
        (get_required_signatures)

        ///account api
        (get_account_name_from_seed)
        (account_exist)
        (get_account)
        (get_account_history)
        (get_active_authority)
        (get_owner_authority)
        (get_memo_key)
        (get_account_balance)
        (get_vesting_balance)
        (create_simple_authority)
        (create_simple_multisig_authority)
        (create_simple_managed_authority)
        (create_simple_multisig_managed_authority)

        /// transaction api
        (create_account)
        (update_account)
        (delete_account)
        (update_witness)
        (set_voting_proxy)
        (vote_for_witness)
        (transfer)

        (transfer_to_vesting)
        (withdraw_vesting)
        (create_application)
        (update_application)
        (delete_application)
        (buy_application)
        (cancel_application_buying)

        (make_custom_json_operation)
        (make_custom_binary_operation)

        /// helper api
        (broadcast_transaction)
        (create_transaction)
        (create_simple_transaction)
        (calculate_fee)
        (fiat_to_sphtx)

        ///local api
        (get_transaction_digest)
        (add_signature)
        (sign_digest)
        (send_and_sign_operation)
        (send_and_sign_transaction)
        (verify_signature)
        (generate_key_pair)
        (generate_key_pair_from_brain_key)
        (get_public_key)
        (from_base58)
        (to_base58)
        (encrypt_data)
        (decrypt_data)
      )

