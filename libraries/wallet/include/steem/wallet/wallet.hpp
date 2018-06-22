#pragma once

#include <steem/wallet/remote_node_api.hpp>

#include <steem/utilities/key_conversion.hpp>

#include <fc/macros.hpp>
#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/api.hpp>

namespace steem { namespace wallet {

using namespace std;

using namespace steem::utilities;
using namespace steem::protocol;

struct memo_data {

   static optional<memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(memo_data) && str[0] == '#') {
            auto data = fc::from_base58( str.substr(1) );
            auto m  = fc::raw::unpack_from_vector<memo_data>( data );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return optional<memo_data>();
   }

   public_key_type from;
   public_key_type to;
   uint64_t        nonce = 0;
   uint32_t        check = 0;
   vector<char>    encrypted;

   operator string()const {
      auto data = fc::raw::pack_to_vector(*this);
      auto base58 = fc::to_base58( data );
      return '#'+base58;
   }
};



struct brain_key_info
{
   string               brain_priv_key;
   public_key_type      pub_key;
   string               wif_priv_key;
};

struct wallet_data
{
   vector<char>              cipher_keys; /** encrypted keys */

   string                    ws_server = "ws://localhost:8090";
   string                    ws_user;
   string                    ws_password;
};

enum authority_type
{
   owner,
   active
};

namespace detail {
class wallet_api_impl;
}

/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 */
class wallet_api
{
   public:
      wallet_api( const wallet_data& initial_data, const steem::protocol::chain_id_type& _steem_chain_id, fc::api< remote_node_api > rapi );
      virtual ~wallet_api();

      bool copy_wallet_file( string destination_filename );


      /** Returns a list of all commands supported by the wallet API.
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
      variant_object                      about() const;

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
      condenser_api::api_feed_history_object get_feed_history( asset_symbol_type symbol)const;

      /**
       * Returns the list of witnesses producing blocks in the current round (21 Blocks)
       */
      vector< account_name_type > get_active_witnesses()const;

      /**
       * Returns the state info associated with the URL
       */
      condenser_api::state get_state( string url );

      /**
       *  Gets the account information for all accounts for which this wallet has a private key
       */
      vector< condenser_api::api_account_object > list_my_accounts();

      /** Lists all accounts registered in the blockchain.
       * This returns a list of all account names and their account ids, sorted by account name.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
       *
       * @param lowerbound the name of the first account to return.  If the named account does not exist,
       *                   the list will start at the account that comes after \c lowerbound
       * @param limit the maximum number of accounts to return (max: 1000)
       * @returns a list of accounts mapping account names to account ids
       */
      vector< account_name_type > list_accounts(const string& lowerbound, uint32_t limit);

      /** Returns the block chain's rapidly-changing properties.
       * The returned object contains information that changes every block interval
       * such as the head block number, the next witness, etc.
       * @see \c get_global_properties() for less-frequently changing properties
       * @returns the dynamic global properties
       */
      condenser_api::extended_dynamic_global_properties get_dynamic_global_properties() const;

      /** Returns information about the given account.
       *
       * @param account_name the name of the account to provide information about
       * @returns the public account data stored in the blockchain
       */
      condenser_api::api_account_object get_account( string account_name ) const;

      /** Returns the current wallet filename.
       *
       * This is the filename that will be used when automatically saving the wallet.
       *
       * @see set_wallet_filename()
       * @return the wallet filename
       */
      string                            get_wallet_filename() const;

      /**
       * Get the WIF private key corresponding to a public key.  The
       * private key must already be in the wallet.
       */
      string                            get_private_key( public_key_type pubkey )const;

      /**
       *  @param account  - the name of the account to retrieve key for
       *  @param role     - active | owner  | memo
       *  @param password - the password to be used at key generation
       *  @return public key corresponding to generated private key, and private key in WIF format.
       */
      pair<public_key_type,string>  get_private_key_from_password( string account, string role, string password )const;


      /**
       * Returns transaction by ID.
       */
      annotated_signed_transaction get_transaction( transaction_id_type trx_id )const;

      /** Checks whether the wallet has just been created and has not yet had a password set.
       *
       * Calling \c set_password will transition the wallet to the locked state.
       * @return true if the wallet is new
       * @ingroup Wallet Management
       */
      bool    is_new()const;

      /** Checks whether the wallet is locked (is unable to use its private keys).
       *
       * This state can be changed by calling \c lock() or \c unlock().
       * @return true if the wallet is locked
       * @ingroup Wallet Management
       */
      bool    is_locked()const;

      /** Locks the wallet immediately.
       * @ingroup Wallet Management
       */
      void    lock();

      /** Unlocks the wallet.
       *
       * The wallet remain unlocked until the \c lock is called
       * or the program exits.
       * @param password the password previously set with \c set_password()
       * @ingroup Wallet Management
       */
      void    unlock(string password);

      /** Sets a new password on the wallet.
       *
       * The wallet must be either 'new' or 'unlocked' to
       * execute this command.
       * @ingroup Wallet Management
       */
      void    set_password(string password);

      /** Dumps all private keys owned by the wallet.
       *
       * The keys are printed in WIF format.  You can import these keys into another wallet
       * using \c import_key()
       * @returns a map containing the private keys, indexed by their public key
       */
      map<public_key_type, string> list_keys();

      /** Returns detailed help on a single API command.
       * @param method the name of the API command you want help with
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  gethelp(const string& method)const;

      /** Loads a specified Graphene wallet.
       *
       * The current wallet is closed before the new wallet is loaded.
       *
       * @warning This does not change the filename that will be used for future
       * wallet writes, so this may cause you to overwrite your original
       * wallet unless you also call \c set_wallet_filename()
       *
       * @param wallet_filename the filename of the wallet JSON file to load.
       *                        If \c wallet_filename is empty, it reloads the
       *                        existing wallet file
       * @returns true if the specified wallet is loaded
       */
      bool    load_wallet_file(string wallet_filename = "");

      /** Saves the current wallet to the given filename.
       *
       * @warning This does not change the wallet filename that will be used for future
       * writes, so think of this function as 'Save a Copy As...' instead of
       * 'Save As...'.  Use \c set_wallet_filename() to make the filename
       * persist.
       * @param wallet_filename the filename of the new wallet JSON file to create
       *                        or overwrite.  If \c wallet_filename is empty,
       *                        save to the current filename.
       */
      void    save_wallet_file(string wallet_filename = "");

      /** Sets the wallet filename used for future writes.
       *
       * This does not trigger a save, it only changes the default filename
       * that will be used the next time a save is triggered.
       *
       * @param wallet_filename the new filename to use for future saves
       */
      void    set_wallet_filename(string wallet_filename);

      /** Suggests a safe brain key to use for creating your account.
       * \c create_account_with_brain_key() requires you to specify a 'brain key',
       * a long passphrase that provides enough entropy to generate cyrptographic
       * keys.  This function will suggest a suitably random string that should
       * be easy to write down (and, with effort, memorize).
       * @returns a suggested brain_key
       */
      brain_key_info suggest_brain_key()const;

      /** Converts a signed_transaction in JSON form to its binary representation.
       *
       * TODO: I don't see a broadcast_transaction() function, do we need one?
       *
       * @param tx the transaction to serialize
       * @returns the binary form of the transaction.  It will not be hex encoded,
       *          this returns a raw string that may have null characters embedded
       *          in it
       */
      string serialize_transaction(signed_transaction tx) const;

      /** Imports a WIF Private Key into the wallet to be used to sign transactions by an account.
       *
       * example: import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
       *
       * @param wif_key the WIF Private Key to import
       */
      bool import_key( string wif_key );

      /** Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
       *
       * This takes a user-supplied brain key and normalizes it into the form used
       * for generating private keys.  In particular, this upper-cases all ASCII characters
       * and collapses multiple spaces into one.
       * @param s the brain key as supplied by the user
       * @returns the brain key in its normalized form
       */
      string normalize_brain_key(string s) const;

      /**
       *  This method will genrate new owner, active, and memo keys for the new account which
       *  will be controlable by this wallet. There is a fee associated with account creation
       *  that is paid by the creator. The current account creation fee can be found with the
       *  'info' wallet command.
       *
       *  @param creator The account creating the new account
       *  @param new_account_name The name of the new account
       *  @param json_meta JSON Metadata associated with the new account
       *  @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction create_account( string creator, string new_account_name, string json_meta, bool broadcast );

      /**
       * This method is used by faucets to create new accounts for other users which must
       * provide their desired keys. The resulting account may not be controllable by this
       * wallet. There is a fee associated with account creation that is paid by the creator.
       * The current account creation fee can be found with the 'info' wallet command.
       *
       * @param creator The account creating the new account
       * @param newname The name of the new account
       * @param json_meta JSON Metadata associated with the new account
       * @param owner public owner key of the new account
       * @param active public active key of the new account
       * @param memo public memo key of the new account
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction create_account_with_keys( string creator,
                                            string newname,
                                            string json_meta,
                                            public_key_type owner,
                                            public_key_type active,
                                            public_key_type memo,
                                            bool broadcast )const;



      /**
       * This method updates the keys of an existing account.
       *
       * @param accountname The name of the account
       * @param json_meta New JSON Metadata to be associated with the account
       * @param owner New public owner key for the account
       * @param active New public active key for the account
       * @param memo New public memo key for the account
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account( string accountname,
                                         string json_meta,
                                         public_key_type owner,
                                         public_key_type active,
                                         public_key_type memo,
                                         bool broadcast )const;

      /**
       * This method updates the key of an authority for an exisiting account.
       * Warning: You can create impossible authorities using this method. The method
       * will fail if you create an impossible owner authority, but will allow impossible
       * active authorities.
       *
       * @param account_name The name of the account whose authority you wish to update
       * @param type The authority type. e.g. owner or active
       * @param key The public key to add to the authority
       * @param weight The weight the key should have in the authority. A weight of 0 indicates the removal of the key.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction update_account_auth_key( string account_name, authority_type type, public_key_type key, weight_type weight, bool broadcast );

      /**
       * This method updates the account of an authority for an exisiting account.
       * Warning: You can create impossible authorities using this method. The method
       * will fail if you create an impossible owner authority, but will allow impossible
       * active authorities.
       *
       * @param account_name The name of the account whose authority you wish to update
       * @param type The authority type. e.g. owner or active
       * @param auth_account The account to add the the authority
       * @param weight The weight the account should have in the authority. A weight of 0 indicates the removal of the account.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction update_account_auth_account( string account_name, authority_type type, string auth_account, weight_type weight, bool broadcast );

      /**
       * This method updates the weight threshold of an authority for an account.
       * Warning: You can create impossible authorities using this method as well
       * as implicitly met authorities. The method will fail if you create an implicitly
       * true authority and if you create an impossible owner authoroty, but will allow
       * impossible active authorities.
       *
       * @param account_name The name of the account whose authority you wish to update
       * @param type The authority type. e.g. owner or active
       * @param threshold The weight threshold required for the authority to be met
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account_auth_threshold( string account_name, authority_type type, uint32_t threshold, bool broadcast );

      /**
       * This method updates the account JSON metadata
       *
       * @param account_name The name of the account you wish to update
       * @param json_meta The new JSON metadata for the account. This overrides existing metadata
       * @param broadcast ture if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account_meta( string account_name, string json_meta, bool broadcast );

      /**
       * This method updates the memo key of an account
       *
       * @param account_name The name of the account you wish to update
       * @param key The new memo public key
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account_memo_key( string account_name, public_key_type key, bool broadcast );

     /**
      * This method deletes an existing account.
      *
      * @param account_name The name of the account you wish to delete
      * @param broadcast true if you wish to broadcast the transaction.
      */
      annotated_signed_transaction delete_account( string account_name, bool broadcast );

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
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction update_witness(string witness_name,
                                        string url,
                                        public_key_type block_signing_key,
                                        const chain_properties& props,
                                        bool broadcast = false);

      /**
       * Stop being a witness, effectively deleting the witness object owned by the given account.
       *
       * @param witness_name The name of the witness account.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction stop_witness(string witness_name,
                                               bool broadcast = false);

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
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction set_voting_proxy(string account_to_modify,
                                          string proxy,
                                          bool broadcast = false);

      /**
       * Vote for a witness to become a block producer. By default an account has not voted
       * positively or negatively for a witness. The account can either vote for with positively
       * votes or against with negative votes. The vote will remain until updated with another
       * vote. Vote strength is determined by the accounts vesting shares.
       *
       * @param account_to_vote_with The account voting for a witness
       * @param witness_to_vote_for The witness that is being voted for
       * @param approve true if the account is voting for the account to be able to be a block produce
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction vote_for_witness(string account_to_vote_with,
                                          string witness_to_vote_for,
                                          bool approve = true,
                                          bool broadcast = false);

      /**
       * Transfer funds from one account to another. STEEM can be transferred.
       *
       * @param from The account the funds are coming from
       * @param to The account the funds are going to
       * @param amount The funds being transferred. i.e. "100.000 STEEM"
       * @param memo A memo for the transactionm, encrypted with the to account's public memo key
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction transfer(string from, string to, asset amount, string memo, bool broadcast = false);

      /**
       * Transfer funds from one account to another using escrow. STEEM can be transferred.
       *
       * @param from The account the funds are coming from
       * @param to The account the funds are going to
       * @param agent The account acting as the agent in case of dispute
       * @param escrow_id A unique id for the escrow transfer. (from, escrow_id) must be a unique pair
       * @param steem_amount The amount of STEEM to transfer
       * @param fee The fee paid to the agent
       * @param ratification_deadline The deadline for 'to' and 'agent' to approve the escrow transfer
       * @param escrow_expiration The expiration of the escrow transfer, after which either party can claim the funds
       * @param json_meta JSON encoded meta data
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction escrow_transfer(
         string from,
         string to,
         string agent,
         uint32_t escrow_id,
         asset steem_amount,
         asset fee,
         time_point_sec ratification_deadline,
         time_point_sec escrow_expiration,
         string json_meta,
         bool broadcast = false
      );

      /**
       * Approve a proposed escrow transfer. Funds cannot be released until after approval. This is in lieu of requiring
       * multi-sig on escrow_transfer
       *
       * @param from The account that funded the escrow
       * @param to The destination of the escrow
       * @param agent The account acting as the agent in case of dispute
       * @param who The account approving the escrow transfer (either 'to' or 'agent)
       * @param escrow_id A unique id for the escrow transfer
       * @param approve true to approve the escrow transfer, otherwise cancels it and refunds 'from'
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction escrow_approve(
         string from,
         string to,
         string agent,
         string who,
         uint32_t escrow_id,
         bool approve,
         bool broadcast = false
      );

      /**
       * Raise a dispute on the escrow transfer before it expires
       *
       * @param from The account that funded the escrow
       * @param to The destination of the escrow
       * @param agent The account acting as the agent in case of dispute
       * @param who The account raising the dispute (either 'from' or 'to')
       * @param escrow_id A unique id for the escrow transfer
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction escrow_dispute(
         string from,
         string to,
         string agent,
         string who,
         uint32_t escrow_id,
         bool broadcast = false
      );

      /**
       * Release funds help in escrow
       *
       * @param from The account that funded the escrow
       * @param to The account the funds are originally going to
       * @param agent The account acting as the agent in case of dispute
       * @param who The account authorizing the release
       * @param receiver The account that will receive funds being released
       * @param escrow_id A unique id for the escrow transfer
       * @param steem_amount The amount of STEEM that will be released
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction escrow_release(
         string from,
         string to,
         string agent,
         string who,
         string receiver,
         uint32_t escrow_id,
         asset steem_amount,
         bool broadcast = false
      );

      /**
       * Transfer STEEM into a vesting fund represented by vesting shares (VESTS). VESTS are required to vesting
       * for a minimum of one coin year and can be withdrawn once a week over a two year withdraw period.
       * VESTS are protected against dilution up until 90% of STEEM is vesting.
       *
       * @param from The account the STEEM is coming from
       * @param to The account getting the VESTS
       * @param amount The amount of STEEM to vest i.e. "100.00 STEEM"
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction transfer_to_vesting(string from, string to, asset amount, bool broadcast = false);

   /**
    *  @param from       - the account that initiated the transfer
    *  @param request_id - an unique ID assigned by from account, the id is used to cancel the operation and can be reused after the transfer completes
    *  @param to         - the account getting the transfer
    *  @param amount     - the amount of assets to be transfered
    *  @param memo A memo for the transactionm, encrypted with the to account's public memo key
    *  @param broadcast true if you wish to broadcast the transaction
    */
      annotated_signed_transaction transfer_from_savings( string from, uint32_t request_id, string to, asset amount, string memo, bool broadcast = false );


   /**
    * Set up a vesting withdraw request. The request is fulfilled once a week over the next two year (104 weeks).
    *
    * @param from The account the VESTS are withdrawn from
    * @param vesting_shares The amount of VESTS to withdraw over the next two years. Each week (amount/104) shares are
    *    withdrawn and deposited back as STEEM. i.e. "10.000000 VESTS"
    * @param broadcast true if you wish to broadcast the transaction
    */
      annotated_signed_transaction withdraw_vesting( string from, asset vesting_shares, bool broadcast = false );

      /**
       * A witness can public a price feed for the STEEM:SBD market. The median price feed is used
       * to process conversion requests from SBD to STEEM.
       *
       * @param witness The witness publishing the price feed
       * @param exchange_rate The desired exchange rate
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction publish_feed(string witness, price exchange_rate, bool broadcast );

      /** Signs a transaction.
       *
       * Given a fully-formed transaction that is only lacking signatures, this signs
       * the transaction with the necessary keys and optionally broadcasts the transaction
       * @param tx the unsigned transaction
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

      /** Returns an uninitialized object representing a given blockchain operation.
       *
       * This returns a default-initialized object of the given type; it can be used
       * during early development of the wallet when we don't yet have custom commands for
       * creating all of the operations the blockchain supports.
       *
       * Any operation the blockchain supports can be created using the transaction builder's
       * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
       * know what the JSON form of the operation looks like.  This will give you a template
       * you can fill in.  It's better than nothing.
       *
       * @param operation_type the type of operation to return, must be one of the
       *                       operations defined in `steem/chain/operations.hpp`
       *                       (e.g., "global_parameters_update_operation")
       * @return a default-constructed operation of the given type
       */
      operation get_prototype_operation(string operation_type);

      /**
       * Sets the amount of time in the future until a transaction expires.
       */
      void set_transaction_expiration(uint32_t seconds);

      /**
       * Create an account recovery request as a recover account. The syntax for this command contains a serialized authority object
       * so there is an example below on how to pass in the authority.
       *
       * request_account_recovery "your_account" "account_to_recover" {"weight_threshold": 1,"account_auths": [], "key_auths": [["new_public_key",1]]} true
       *
       * @param recovery_account The name of your account
       * @param account_to_recover The name of the account you are trying to recover
       * @param new_authority The new owner authority for the recovered account. This should be given to you by the holder of the compromised or lost account.
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction request_account_recovery( string recovery_account, string account_to_recover, authority new_authority, bool broadcast );

      /**
       * Recover your account using a recovery request created by your recovery account. The syntax for this commain contains a serialized
       * authority object, so there is an example below on how to pass in the authority.
       *
       * recover_account "your_account" {"weight_threshold": 1,"account_auths": [], "key_auths": [["old_public_key",1]]} {"weight_threshold": 1,"account_auths": [], "key_auths": [["new_public_key",1]]} true
       *
       * @param account_to_recover The name of your account
       * @param recent_authority A recent owner authority on your account
       * @param new_authority The new authority that your recovery account used in the account recover request.
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction recover_account( string account_to_recover, authority recent_authority, authority new_authority, bool broadcast );

      /**
       * Change your recovery account after a 30 day delay.
       *
       * @param owner The name of your account
       * @param new_recovery_account The name of the recovery account you wish to have
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction change_recovery_account( string owner, string new_recovery_account, bool broadcast );

      vector< database_api::api_owner_authority_history_object > get_owner_history( string account )const;

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
      *  This method will create new application object. There is a fee associated with account creation
      *  that is paid by the creator. The current account creation fee can be found with the
      *  'info' wallet command.
      *
      *  @param author The account creating the new application
      *  @param app_name The unique name for new application
      *  @param url The url of the new application
      *  @param meta_data The meta data of new application
      *  @param price_param The price parameter that specifies billing for the app
      *  @param broadcast true if you wish to broadcast the transaction
      */
      annotated_signed_transaction create_application( string author, string app_name,
                                                       string url, string meta_data, uint8_t price_param,
                                                       bool broadcast );

      /**
      *  This method will update existing application object.
      *
      *  @param author The author of application
      *  @param app_name The name of app that will be updated
      *  @param new_author The new author
      *  @param url Updated url
      *  @param meta_data Updated meta data
      *  @param price_param Updated price param
      *  @param broadcast true if you wish to broadcast the transaction
      */
      annotated_signed_transaction update_application( string author, string app_name,
                                                       string new_author, string url, string meta_data,
                                                       uint8_t price_param, bool broadcast );

      /**
      *  This method will delete specified application object.
      *
      *  @param author The author of application that will be deleted
      *  @param app_name The name of app that will be deleted
      *  @param broadcast true if you wish to broadcast the transaction
      */
      annotated_signed_transaction delete_application( string author, string app_name, bool broadcast );

      /**
      *  This method will create application buy object
      *
      *  @param buyer The buyer of application
      *  @param app_id The id of app that buyer will buy
      *  @param broadcast true if you wish to broadcast the transaction
      */
      annotated_signed_transaction buy_application( string buyer, int64_t app_id, bool broadcast );

      /**
      *  This method will cancel application buy object
      *
      *  @param app_owner The owner of bought application
      *  @param buyer The buyer of application
      *  @param app_id The id of bought app
      *  @param broadcast true if you wish to broadcast the transaction
      */
      annotated_signed_transaction cancel_application_buying( string app_owner, string buyer, int64_t app_id, bool broadcast );

      /**
       * Get all app buyings by app_name or buyer
       * @param name Application id or buyers name
       * @param search_type One of "by_buyer", "by_app_id"
       * @param count Number of items to retrieve
       * @return
       */
      vector< condenser_api::api_application_buying_object >  get_application_buyings(string name, string search_type, uint32_t count);


      std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

      fc::signal<void(bool)> lock_changed;
      std::shared_ptr<detail::wallet_api_impl> my;
      void encrypt_keys();

      /**
       * Checks memos against private keys on account and imported in wallet
       */
      void check_memo( const string& memo, const condenser_api::api_account_object& account )const;

      /**
       *  Returns the encrypted memo if memo starts with '#' otherwise returns memo
       */
      string get_encrypted_memo( string from, string to, string memo );

      /**
       * Returns the decrypted memo if possible given wallet's known private keys
       */
      string decrypt_memo( string memo );

      /**
       * Send custom JSON data
       * @param app_id Application ID
       * @param from Sender
       * @param to List of receivers
       * @param json Data formatted in JSON
       * @param broadcast true if you wish to broadcast the transaction
       * @return
       */
      annotated_signed_transaction send_custom_json_document(uint32_t app_id, string from, vector<string> to, string json, bool broadcast);

      /**
       * Send custom data data
       * @param app_id Application ID
       * @param from Sender
       * @param to List of receivers
       * @param data Data formatted in base58.
       *  @param broadcast true if you wish to broadcast the transaction
       * @return
       */
      annotated_signed_transaction send_custom_binary_document(uint32_t app_id, string from, vector<string> to, string data, bool broadcast);

      /**
       * Get all recevied custom jsons and data.
       * @param app_id Application ID
       * @param account_name Name of the relevant (sender/recipient) account
       * @param search_type One of "by_sender", "by_recipient", "by_sender_datetime", "by_recipient_datetime"
       * @param start Either timestamp in ISO format or index
       * @param count Number of items to retrieve
       * @return
       */
      map< uint64_t, condenser_api::api_received_object >  get_received_documents(uint32_t app_id, string account_name, string search_type, string start, uint32_t count);

};

struct plain_keys {
   fc::sha512                  checksum;
   map<public_key_type,string> keys;
};

} }

FC_REFLECT( steem::wallet::wallet_data,
            (cipher_keys)
            (ws_server)
            (ws_user)
            (ws_password)
          )

FC_REFLECT( steem::wallet::brain_key_info, (brain_priv_key)(wif_priv_key) (pub_key))

FC_REFLECT( steem::wallet::plain_keys, (checksum)(keys) )

FC_REFLECT_ENUM( steem::wallet::authority_type, (owner)(active) )

FC_API( steem::wallet::wallet_api,
        /// wallet api
        (help)(gethelp)
        (about)(is_new)(is_locked)(lock)(unlock)(set_password)
        (load_wallet_file)(save_wallet_file)

        /// key api
        (import_key)
        (suggest_brain_key)
        (list_keys)
        (get_private_key)
        (get_private_key_from_password)
        (normalize_brain_key)

        /// query api
        (info)
        (list_my_accounts)
        (list_accounts)
        (list_witnesses)
        (get_witness)
        (get_account)
        (get_block)
        (get_ops_in_block)
        (get_feed_history)
        (get_account_history)
        (get_state)

        /// transaction api
        (create_account)
        (create_account_with_keys)
        (update_account)
        (update_account_auth_key)
        (update_account_auth_account)
        (update_account_auth_threshold)
        (update_account_meta)
        (update_account_memo_key)
        (delete_account)
        (update_witness)
        (set_voting_proxy)
        (vote_for_witness)
        (transfer)
        (escrow_transfer)
        (escrow_approve)
        (escrow_dispute)
        (escrow_release)
        (transfer_to_vesting)
        (withdraw_vesting)
        (publish_feed)
        (set_transaction_expiration)
        (request_account_recovery)
        (recover_account)
        (change_recovery_account)
        (get_owner_history)
        (get_encrypted_memo)
        (decrypt_memo)
        (create_application)
        (update_application)
        (delete_application)
        (buy_application)
        (cancel_application_buying)
        (get_application_buyings)

        /// helper api
        (get_prototype_operation)
        (serialize_transaction)
        (sign_transaction)

        (get_active_witnesses)
        (get_transaction)

        (send_custom_json_document)
        (send_custom_binary_document)
        (get_received_documents)
      )

FC_REFLECT( steem::wallet::memo_data, (from)(to)(nonce)(check)(encrypted) )
