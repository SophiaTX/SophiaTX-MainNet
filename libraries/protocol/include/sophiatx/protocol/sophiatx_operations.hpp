#pragma once
#include <sophiatx/protocol/base.hpp>
#include <sophiatx/protocol/block_header.hpp>
#include <sophiatx/protocol/asset.hpp>
#include <sophiatx/protocol/validation.hpp>

#include <fc/crypto/equihash.hpp>


namespace sophiatx { namespace protocol {

   std::string make_random_fixed_string(std::string seed);


   struct account_create_operation : public base_operation
   {
      account_name_type creator;
      std::string        name_seed;
      authority         owner;
      authority         active;
      public_key_type   memo_key;
      std::string       json_metadata;

      account_name_type get_fee_payer()const { return creator;} ;
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);} ; //<the account creation fee is set by witnesses...

      bool has_special_fee()const {return true;};
      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
   };


   struct account_update_operation : public base_operation
   {
      account_name_type             account;
      optional< authority >         owner;
      optional< authority >         active;
      public_key_type               memo_key;
      string                        json_metadata;

      void validate()const;
      account_name_type get_fee_payer()const { return account;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);} ;

      void get_required_owner_authorities( flat_set<account_name_type>& a )const
      { if( owner ) a.insert( account ); }

      void get_required_active_authorities( flat_set<account_name_type>& a )const
      { if( !owner ) a.insert( account ); }
   };


   struct account_delete_operation : public base_operation
   {
      account_name_type  account;

      void validate()const;
      account_name_type get_fee_payer()const { return account;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);} ;

      void get_required_owner_authorities( flat_set<account_name_type>& a )const
      { a.insert( account ); }

   };


   struct placeholder_a_operation : public base_operation
   {
      void validate()const;
   };

   struct placeholder_b_operation : public base_operation
   {
      void validate()const;
   };


   /**
    * @ingroup operations
    *
    * @brief Transfers SOPHIATX from one account to another.
    */
   struct transfer_operation : public base_operation
   {
      account_name_type from;
      /// Account to transfer asset to
      account_name_type to;
      /// The amount of asset to transfer from @ref from to @ref to
      asset             amount;

      /// The memo is plain-text, any encryption on the memo is up to
      /// a higher level protocol.
      string            memo;

      account_name_type get_fee_payer()const { return from;};

      void              validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ if(amount.symbol != VESTS_SYMBOL) a.insert(from); }
      void get_required_owner_authorities( flat_set<account_name_type>& a )const { if(amount.symbol == VESTS_SYMBOL) a.insert(from); }
   };


   /**
    *  The purpose of this operation is to enable someone to send money contingently to
    *  another individual. The funds leave the *from* account and go into a temporary balance
    *  where they are held until *from* releases it to *to* or *to* refunds it to *from*.
    *
    *  In the event of a dispute the *agent* can divide the funds between the to/from account.
    *  Disputes can be raised any time before or on the dispute deadline time, after the escrow
    *  has been approved by all parties.
    *
    *  This operation only creates a proposed escrow transfer. Both the *agent* and *to* must
    *  agree to the terms of the arrangement by approving the escrow.
    *
    *  The escrow agent is paid the fee on approval of all parties. It is up to the escrow agent
    *  to determine the fee.
    *
    *  Escrow transactions are uniquely identified by 'from' and 'escrow_id', the 'escrow_id' is defined
    *  by the sender.
    */
   struct escrow_transfer_operation : public base_operation
   {
      account_name_type from;
      account_name_type to;
      account_name_type agent;
      uint32_t          escrow_id = 30;

      asset             sophiatx_amount = asset( 0, SOPHIATX_SYMBOL );
      asset             escrow_fee;

      time_point_sec    ratification_deadline;
      time_point_sec    escrow_expiration;

      string            json_meta;

      account_name_type get_fee_payer()const { return from;};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(from); }
   };


   /**
    *  The agent and to accounts must approve an escrow transaction for it to be valid on
    *  the blockchain. Once a part approves the escrow, the cannot revoke their approval.
    *  Subsequent escrow approve operations, regardless of the approval, will be rejected.
    */
   struct escrow_approve_operation : public base_operation
   {
      account_name_type from;
      account_name_type to;
      account_name_type agent;
      account_name_type who; // Either to or agent
      uint32_t          escrow_id = 30;
      bool              approve = true;

      account_name_type get_fee_payer()const { return who;};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(who); }
   };


   /**
    *  If either the sender or receiver of an escrow payment has an issue, they can
    *  raise it for dispute. Once a payment is in dispute, the agent has authority over
    *  who gets what.
    */
   struct escrow_dispute_operation : public base_operation
   {

      account_name_type from;
      account_name_type to;
      account_name_type agent;
      account_name_type who;

      uint32_t          escrow_id = 30;

      account_name_type get_fee_payer()const { return who;};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(who); }
   };


   /**
    *  This operation can be used by anyone associated with the escrow transfer to
    *  release funds if they have permission.
    *
    *  The permission scheme is as follows:
    *  If there is no dispute and escrow has not expired, either party can release funds to the other.
    *  If escrow expires and there is no dispute, either party can release funds to either party.
    *  If there is a dispute regardless of expiration, the agent can release funds to either party
    *     following whichever agreement was in place between the parties.
    */
   struct escrow_release_operation : public base_operation
   {
      account_name_type from;
      account_name_type to; ///< the original 'to'
      account_name_type agent;
      account_name_type who; ///< the account that is attempting to release the funds, determines valid 'receiver'
      account_name_type receiver; ///< the account that should receive funds (might be from, might be to)

      uint32_t          escrow_id = 30;
      asset             sophiatx_amount = asset( 0, SOPHIATX_SYMBOL ); ///< the amount of sophiatx to release

      account_name_type get_fee_payer()const { return who;};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(who); }
   };


   /**
    *  This operation converts SOPHIATX into VFS (Vesting Fund Shares) at
    *  the current exchange rate. With this operation it is possible to
    *  give another account vesting shares so that faucets can
    *  pre-fund new accounts with vesting shares.
    */
   struct transfer_to_vesting_operation : public base_operation
   {
      account_name_type from;
      account_name_type to; ///< if null, then same as from
      asset             amount; ///< must be SOPHIATX

      account_name_type get_fee_payer()const { return from;};

      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(from); }
   };


   /**
    * At any given point in time an account can be withdrawing from their
    * vesting shares. A user may change the number of shares they wish to
    * cash out at any time between 0 and their total vesting stake.
    *
    * After applying this operation, vesting_shares will be withdrawn
    * at a rate of vesting_shares/104 per week for two years starting
    * one week after this operation is included in the blockchain.
    *
    * This operation is not valid if the user has no vesting shares.
    */
   struct withdraw_vesting_operation : public base_operation
   {
      account_name_type account;
      asset             vesting_shares;

      account_name_type get_fee_payer()const { return account;};

      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
   };


   /**
    * Witnesses must vote on how to set certain chain properties to ensure a smooth
    * and well functioning network.  Any time @owner is in the active set of witnesses these
    * properties will be used to control the blockchain configuration.
    */
   struct chain_properties
   {
      /**
       *  This fee, paid in SOPHIATX, is converted into VESTING SHARES for the new account. Accounts
       *  without vesting shares cannot earn usage rations and therefore are powerless. This minimum
       *  fee requires all accounts to have some kind of commitment to the network that includes the
       *  ability to vote and make transactions.
       */
      asset account_creation_fee = asset( SOPHIATX_MIN_ACCOUNT_CREATION_FEE, SOPHIATX_SYMBOL );

      /**
       *  This witnesses vote for the maximum_block_size which is used by the network
       *  to tune rate limiting and capacity
       */
      uint32_t          maximum_block_size = SOPHIATX_MIN_BLOCK_SIZE_LIMIT * 2;

      flat_map<asset_symbol_type, price> price_feeds;


      void validate()const
      {

         FC_ASSERT( account_creation_fee.amount >= SOPHIATX_MIN_ACCOUNT_CREATION_FEE);
         FC_ASSERT( maximum_block_size >= SOPHIATX_MIN_BLOCK_SIZE_LIMIT);
         for (const auto&i : price_feeds){
            FC_ASSERT(i.first == SBD1_SYMBOL_SER || i.first == SBD2_SYMBOL_SER ||
                      i.first == SBD3_SYMBOL_SER || i.first == SBD4_SYMBOL_SER || i.first == SBD5_SYMBOL_SER );
            if(i.second.base.symbol == SOPHIATX_SYMBOL){
               FC_ASSERT(i.second.quote.symbol == i.first);
            }else{
               FC_ASSERT(i.second.base.symbol == i.first && i.second.quote.symbol == SOPHIATX_SYMBOL);
            }
            FC_ASSERT(i.second.quote.amount > 0 && i.second.base.amount > 0);
         }

      }
   };


   /**
    *
    *  If the owner isn't a witness they will become a witness.  Witnesses
    *  are charged a fee equal to 1 weeks worth of witness pay which in
    *  turn is derived from the current share supply.  The fee is
    *  only applied if the owner is not already a witness.
    *
    *  If the block_signing_key is null then the witness is removed from
    *  contention (effectively an witness_stop_operaton).  The network will pick
    *  the top 21 witnesses for producing blocks.
    */
   struct witness_update_operation : public base_operation
   {
      account_name_type owner;
      string            url;
      public_key_type   block_signing_key;
      chain_properties  props;

      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};
      account_name_type get_fee_payer()const { return owner;};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(owner); }
   };

   struct witness_set_properties_operation : public base_operation
   {
      account_name_type                   owner;
      flat_map< string, vector< char > >  props;
      extensions_type                     extensions;

      account_name_type get_fee_payer()const { return owner;};

      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};


      void validate()const;
      void get_required_authorities( vector< authority >& a )const
      {
         auto key_itr = props.find( "key" );

         if( key_itr != props.end() )
         {
            public_key_type signing_key;
            fc::raw::unpack_from_vector( key_itr->second, signing_key );
            a.push_back( authority( 1, signing_key, 1 ) );
         }
         else
            a.push_back( authority( 1, SOPHIATX_NULL_ACCOUNT, 1 ) ); // The null account auth is impossible to satisfy
      }
   };

   struct witness_stop_operation : public base_operation
   {
      account_name_type owner;
      account_name_type get_fee_payer()const { return owner;};

      void validate()const {};
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(owner); }
   };

   /**
    * All accounts with a VFS can vote for or against any witness.
    *
    * If a proxy is specified then all existing votes are removed.
    */
   struct account_witness_vote_operation : public base_operation
   {
      account_name_type account;
      account_name_type witness;
      bool              approve = true;

      account_name_type get_fee_payer()const { return account;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void validate() const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
   };


   struct account_witness_proxy_operation : public base_operation
   {
      account_name_type account;
      account_name_type proxy;

      account_name_type get_fee_payer()const { return account;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
   };

namespace{
asset get_custom_fee(uint32_t payload_size, asset_symbol_type in_symbol){
   asset base = asset(100000, SOPHIATX_SYMBOL);
   if(in_symbol == SBD1_SYMBOL )//USD
      base = asset(100000, SBD1_SYMBOL);
   if(in_symbol == SBD2_SYMBOL )//EUR
      base = asset(80000, SBD2_SYMBOL);
   if(in_symbol == SBD3_SYMBOL ) //CHF
      base = asset(100000, SBD3_SYMBOL);
   if(in_symbol == SBD4_SYMBOL ) //CNY
      base = asset(640000, SBD4_SYMBOL);
   if(in_symbol == SBD5_SYMBOL ) //CNY
      base = asset(75000, SBD5_SYMBOL);

   //pay base fee + for every 1kB exceeding first 512 bytes
   uint32_t size_multi = (payload_size + 511)/1024;
   return base * (1 + size_multi);
};
}
   /**
    * @brief provides a generic way to add higher level protocols on top of witness consensus
    * @ingroup operations
    *
    * There is no validation for this operation other than that required auths are valid
    */
   struct custom_operation : public base_operation
   {
      account_name_type             sender;
      flat_set<account_name_type>   recipients;
      uint64_t                      app_id = 0;
      vector< char >                data;

      account_name_type get_fee_payer()const { return sender;};
      asset get_required_fee(asset_symbol_type in_symbol) const{ return get_custom_fee(data.size(), in_symbol);}

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(sender); }
   };


   /** serves the same purpose as custom_operation but also supports required posting authorities. Unlike custom_operation,
    * this operation is designed to be human readable/developer friendly.
    **/
   struct custom_json_operation : public base_operation
   {
      account_name_type             sender;
      flat_set<account_name_type>   recipients;
      uint64_t                      app_id; ///< must be less than 32 characters long
      string                        json; ///< must be proper utf8 / JSON string.

      account_name_type get_fee_payer()const { return sender;};
      asset get_required_fee(asset_symbol_type in_symbol) const{ return get_custom_fee(json.size(), in_symbol);}


      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(sender); }
   };


   struct custom_binary_operation : public base_operation
   {
      account_name_type             sender;
      flat_set<account_name_type>   recipients;
      uint64_t                      app_id; ///< must be less than 32 characters long
      vector< char >                data;

      account_name_type get_fee_payer()const { return sender;};
      asset get_required_fee(asset_symbol_type in_symbol)const { return get_custom_fee(data.size(), in_symbol);}


      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(sender); }
   };


   /**
    *  Feeds can only be published by the top N witnesses which are included in every round and are
    *  used to define the exchange rate between sophiatx and the dollar.
    */
   struct feed_publish_operation : public base_operation
   {
      account_name_type publisher;
      price             exchange_rate;

      account_name_type get_fee_payer()const { return publisher;};

      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void  validate()const;
      void  get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(publisher); }
   };


   /**
    * This operation is used to report a miner who signs two blocks
    * at the same time. To be valid, the violation must be reported within
    * SOPHIATX_MAX_WITNESSES blocks of the head block (1 round) and the
    * producer must be in the ACTIVE witness set.
    *
    * Users not in the ACTIVE witness set should not have to worry about their
    * key getting compromised and being used to produced multiple blocks so
    * the attacker can report it and steel their vesting sophiatx.
    *
    * The result of the operation is to transfer the full VESTING SOPHIATX balance
    * of the block producer to the reporter.
    */
   struct report_over_production_operation : public base_operation
   {
      account_name_type    reporter;
      signed_block_header  first_block;
      signed_block_header  second_block;

      account_name_type get_fee_payer()const { return reporter;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void validate()const;
   };


   /**
    * All account recovery requests come from a listed recovery account. This
    * is secure based on the assumption that only a trusted account should be
    * a recovery account. It is the responsibility of the recovery account to
    * verify the identity of the account holder of the account to recover by
    * whichever means they have agreed upon. The blockchain assumes identity
    * has been verified when this operation is broadcast.
    *
    * This operation creates an account recovery request which the account to
    * recover has 24 hours to respond to before the request expires and is
    * invalidated.
    *
    * There can only be one active recovery request per account at any one time.
    * Pushing this operation for an account to recover when it already has
    * an active request will either update the request to a new new owner authority
    * and extend the request expiration to 24 hours from the current head block
    * time or it will delete the request. To cancel a request, simply set the
    * weight threshold of the new owner authority to 0, making it an open authority.
    *
    * Additionally, the new owner authority must be satisfiable. In other words,
    * the sum of the key weights must be greater than or equal to the weight
    * threshold.
    *
    * This operation only needs to be signed by the the recovery account.
    * The account to recover confirms its identity to the blockchain in
    * the recover account operation.
    */
   struct request_account_recovery_operation : public base_operation
   {
      account_name_type recovery_account;       ///< The recovery account is listed as the recovery account on the account to recover.

      account_name_type account_to_recover;     ///< The account to recover. This is likely due to a compromised owner authority.

      authority         new_owner_authority;    ///< The new owner authority the account to recover wishes to have. This is secret
                                                ///< known by the account to recover and will be confirmed in a recover_account_operation

      extensions_type   extensions;             ///< Extensions. Not currently used.

      account_name_type get_fee_payer()const { return recovery_account;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};


      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( recovery_account ); }


      void validate() const;
   };


   /**
    * Recover an account to a new authority using a previous authority and verification
    * of the recovery account as proof of identity. This operation can only succeed
    * if there was a recovery request sent by the account's recover account.
    *
    * In order to recover the account, the account holder must provide proof
    * of past ownership and proof of identity to the recovery account. Being able
    * to satisfy an owner authority that was used in the past 30 days is sufficient
    * to prove past ownership. The get_owner_history function in the database API
    * returns past owner authorities that are valid for account recovery.
    *
    * Proving identity is an off chain contract between the account holder and
    * the recovery account. The recovery request contains a new authority which
    * must be satisfied by the account holder to regain control. The actual process
    * of verifying authority may become complicated, but that is an application
    * level concern, not a blockchain concern.
    *
    * This operation requires both the past and future owner authorities in the
    * operation because neither of them can be derived from the current chain state.
    * The operation must be signed by keys that satisfy both the new owner authority
    * and the recent owner authority. Failing either fails the operation entirely.
    *
    * If a recovery request was made inadvertantly, the account holder should
    * contact the recovery account to have the request deleted.
    *
    * The two setp combination of the account recovery request and recover is
    * safe because the recovery account never has access to secrets of the account
    * to recover. They simply act as an on chain endorsement of off chain identity.
    * In other systems, a fork would be required to enforce such off chain state.
    * Additionally, an account cannot be permanently recovered to the wrong account.
    * While any owner authority from the past 30 days can be used, including a compromised
    * authority, the account can be continually recovered until the recovery account
    * is confident a combination of uncompromised authorities were used to
    * recover the account. The actual process of verifying authority may become
    * complicated, but that is an application level concern, not the blockchain's
    * concern.
    */
   struct recover_account_operation : public base_operation
   {
      account_name_type account_to_recover;        ///< The account to be recovered

      authority         new_owner_authority;       ///< The new owner authority as specified in the request account recovery operation.

      authority         recent_owner_authority;    ///< A previous owner authority that the account holder will use to prove past ownership of the account to be recovered.

      extensions_type   extensions;                ///< Extensions. Not currently used.

      void get_required_authorities( vector< authority >& a )const
      {
         a.push_back( new_owner_authority );
         a.push_back( recent_owner_authority );
      }

      account_name_type get_fee_payer()const { return account_to_recover;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void validate() const;
   };


   /**
    *  This operation allows recovery_accoutn to change account_to_reset's owner authority to
    *  new_owner_authority after 60 days of inactivity.
    */
   struct reset_account_operation : public base_operation {
      account_name_type reset_account;
      account_name_type account_to_reset;
      authority         new_owner_authority;

      account_name_type get_fee_payer()const { return reset_account;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( reset_account ); }
      void validate()const;
   };

   /**
    * This operation allows 'account' owner to control which account has the power
    * to execute the 'reset_account_operation' after 60 days.
    */
   struct set_reset_account_operation : public base_operation {

      account_name_type account;
      account_name_type current_reset_account;
      account_name_type reset_account;


      account_name_type get_fee_payer()const { return account;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void validate()const;
      void get_required_owner_authorities( flat_set<account_name_type>& a )const
      {
         if( current_reset_account.size() )
            a.insert( account );
      }

   };

   enum application_price_param
   {
      permanent,
      time_based,
      none
   };

   struct application_create_operation : public base_operation {

      account_name_type       author;
      string                  name;
      string                  url;
      string                  metadata;
      uint8_t                 price_param;

      account_name_type get_fee_payer()const { return author;};

      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( author ); }
      void validate()const;
   };

   struct application_update_operation : public base_operation {

      account_name_type             author;
      optional<account_name_type>   new_author;
      string                        name;
      string                        url;
      string                        metadata;
      optional<uint8_t>             price_param;

      account_name_type get_fee_payer()const { return author;};

      void get_required_active_authorities( flat_set<account_name_type>& a )const {  a.insert( author ); }
      void validate()const;
   };

   struct application_delete_operation : public base_operation {

      account_name_type             author;
      string                        name;

      account_name_type get_fee_payer()const { return author;};

      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( author ); }
      void validate()const;
   };

   struct buy_application_operation : public base_operation {

      account_name_type             buyer;
      int64_t                       app_id;

      account_name_type get_fee_payer()const { return buyer;};

      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( buyer ); }
      void validate()const;
   };

   struct cancel_application_buying_operation : public base_operation {

      account_name_type             app_owner;
      account_name_type             buyer;
      int64_t                       app_id;

      account_name_type get_fee_payer()const { return app_owner;};

      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( app_owner ); }
      void validate()const;
   };


   /**
    * Each account lists another account as their recovery account.
    * The recovery account has the ability to create account_recovery_requests
    * for the account to recover. An account can change their recovery account
    * at any time with a 30 day delay. This delay is to prevent
    * an attacker from changing the recovery account to a malicious account
    * during an attack. These 30 days match the 30 days that an
    * owner authority is valid for recovery purposes.
    *
    * On account creation the recovery account is set either to the creator of
    * the account (The account that pays the creation fee and is a signer on the transaction)
    * or to the empty string if the account was mined. An account with no recovery
    * has the top voted witness as a recovery account, at the time the recover
    * request is created. Note: This does mean the effective recovery account
    * of an account with no listed recovery account can change at any time as
    * witness vote weights. The top voted witness is explicitly the most trusted
    * witness according to stake.
    */
   struct change_recovery_account_operation : public base_operation
   {
      asset             fee;

      account_name_type account_to_recover;     ///< The account that would be recovered in case of compromise
      account_name_type new_recovery_account;   ///< The account that creates the recover request
      extensions_type   extensions;             ///< Extensions. Not currently used.


      account_name_type get_fee_payer()const { return account_to_recover;};
      asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

      void get_required_owner_authorities( flat_set<account_name_type>& a )const{ a.insert( account_to_recover ); }
      void validate() const;
   };


   /**
    * Recover funds from the promotion pool. Only initminer can do that.
    */
    struct transfer_from_promotion_pool_operation : public base_operation
    {

       account_name_type transfer_to;
       asset             amount;
       extensions_type   extensions;             ///< Extensions. Not currently used.


       account_name_type get_fee_payer()const { return transfer_to;};
       asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

       void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( SOPHIATX_INIT_MINER_NAME ); }

       void validate() const;
    };

    /**
     * Allow sponsoring someone elses fees. In order to remove sponsor, just send with your account in "sponsored" and keep the "sponsor" empty. On other side, to 
     * stop sponsoring, set "is_sponsoring" to false. 
     */
     struct sponsor_fees_operation : public base_operation
     {
        account_name_type sponsor;
        account_name_type sponsored;
        bool              is_sponsoring;
        extensions_type   extensions;             ///< Extensions. Not currently used.


        account_name_type get_fee_payer()const { if(sponsor == "") return sponsored; return sponsor;};
        asset get_required_fee(asset_symbol_type in_symbol)const{ return asset(0, in_symbol);};

        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(get_fee_payer()); }

        void validate() const;
     };


} } // sophiatx::protocol


FC_REFLECT_DERIVED( sophiatx::protocol::reset_account_operation, (sophiatx::protocol::base_operation), (reset_account)(account_to_reset)(new_owner_authority) )
FC_REFLECT_DERIVED( sophiatx::protocol::set_reset_account_operation, (sophiatx::protocol::base_operation), (account)(current_reset_account)(reset_account) )


FC_REFLECT_DERIVED( sophiatx::protocol::report_over_production_operation, (sophiatx::protocol::base_operation), (reporter)(first_block)(second_block) )
FC_REFLECT_DERIVED( sophiatx::protocol::feed_publish_operation, (sophiatx::protocol::base_operation), (publisher)(exchange_rate) )

FC_REFLECT( sophiatx::protocol::chain_properties,
            (account_creation_fee)
            (maximum_block_size)
            (price_feeds)
          )

FC_REFLECT_DERIVED( sophiatx::protocol::account_create_operation, (sophiatx::protocol::base_operation),
            (creator)
            (name_seed)
            (owner)
            (active)
            (memo_key)
            (json_metadata) )

FC_REFLECT_DERIVED( sophiatx::protocol::account_update_operation, (sophiatx::protocol::base_operation),
            (account)
            (owner)
            (active)
            (memo_key)
            (json_metadata) )

FC_REFLECT_DERIVED( sophiatx::protocol::account_delete_operation, (sophiatx::protocol::base_operation), (account) )
FC_REFLECT_DERIVED( sophiatx::protocol::transfer_operation, (sophiatx::protocol::base_operation), (from)(to)(amount)(memo) )
FC_REFLECT_DERIVED( sophiatx::protocol::transfer_to_vesting_operation, (sophiatx::protocol::base_operation), (from)(to)(amount) )
FC_REFLECT_DERIVED( sophiatx::protocol::withdraw_vesting_operation, (sophiatx::protocol::base_operation), (account)(vesting_shares) )
FC_REFLECT_DERIVED( sophiatx::protocol::witness_update_operation, (sophiatx::protocol::base_operation), (owner)(url)(block_signing_key)(props) )
FC_REFLECT_DERIVED( sophiatx::protocol::witness_stop_operation, (sophiatx::protocol::base_operation), (owner) )
FC_REFLECT_DERIVED( sophiatx::protocol::witness_set_properties_operation, (sophiatx::protocol::base_operation), (owner)(props)(extensions) )
FC_REFLECT_DERIVED( sophiatx::protocol::account_witness_vote_operation, (sophiatx::protocol::base_operation), (account)(witness)(approve) )
FC_REFLECT_DERIVED( sophiatx::protocol::account_witness_proxy_operation, (sophiatx::protocol::base_operation), (account)(proxy) )
FC_REFLECT_DERIVED( sophiatx::protocol::custom_operation, (sophiatx::protocol::base_operation), (sender)(recipients)(app_id)(data) )
FC_REFLECT_DERIVED( sophiatx::protocol::custom_json_operation, (sophiatx::protocol::base_operation), (sender)(recipients)(app_id)(json) )
FC_REFLECT_DERIVED( sophiatx::protocol::custom_binary_operation, (sophiatx::protocol::base_operation), (sender)(recipients)(app_id)(data) )
#ifdef SOPHIATX_ENABLE_SMT
FC_REFLECT( sophiatx::protocol::votable_asset_info_v1, (max_accepted_payout)(allow_curation_rewards) )
FC_REFLECT( sophiatx::protocol::allowed_vote_assets, (votable_assets) )
#endif

FC_REFLECT_DERIVED( sophiatx::protocol::escrow_transfer_operation, (sophiatx::protocol::base_operation), (from)(to)(sophiatx_amount)(escrow_id)(agent)(escrow_fee)(json_meta)(ratification_deadline)(escrow_expiration) );
FC_REFLECT_DERIVED( sophiatx::protocol::escrow_approve_operation, (sophiatx::protocol::base_operation), (from)(to)(agent)(who)(escrow_id)(approve) );
FC_REFLECT_DERIVED( sophiatx::protocol::escrow_dispute_operation, (sophiatx::protocol::base_operation), (from)(to)(agent)(who)(escrow_id) );
FC_REFLECT_DERIVED( sophiatx::protocol::escrow_release_operation, (sophiatx::protocol::base_operation), (from)(to)(agent)(who)(receiver)(escrow_id)(sophiatx_amount) );
FC_REFLECT_DERIVED( sophiatx::protocol::placeholder_a_operation, (sophiatx::protocol::base_operation), );
FC_REFLECT_DERIVED( sophiatx::protocol::placeholder_b_operation, (sophiatx::protocol::base_operation), );
FC_REFLECT_DERIVED( sophiatx::protocol::request_account_recovery_operation, (sophiatx::protocol::base_operation), (recovery_account)(account_to_recover)(new_owner_authority)(extensions) );
FC_REFLECT_DERIVED( sophiatx::protocol::recover_account_operation, (sophiatx::protocol::base_operation), (account_to_recover)(new_owner_authority)(recent_owner_authority)(extensions) );
FC_REFLECT_DERIVED( sophiatx::protocol::application_create_operation, (sophiatx::protocol::base_operation), (author)(name)(url)(metadata)(price_param) )
FC_REFLECT_DERIVED( sophiatx::protocol::application_update_operation, (sophiatx::protocol::base_operation), (author)(new_author)(name)(url)(metadata)(price_param) )
FC_REFLECT_DERIVED( sophiatx::protocol::application_delete_operation, (sophiatx::protocol::base_operation), (author)(name) )
FC_REFLECT_ENUM( sophiatx::protocol::application_price_param, (permanent)(time_based)(none) )
FC_REFLECT_DERIVED( sophiatx::protocol::buy_application_operation, (sophiatx::protocol::base_operation), (buyer)(app_id) )
FC_REFLECT_DERIVED( sophiatx::protocol::cancel_application_buying_operation, (sophiatx::protocol::base_operation), (app_owner)(buyer)(app_id) )
FC_REFLECT_DERIVED( sophiatx::protocol::change_recovery_account_operation, (sophiatx::protocol::base_operation), (account_to_recover)(new_recovery_account)(extensions) );
FC_REFLECT_DERIVED( sophiatx::protocol::transfer_from_promotion_pool_operation, (sophiatx::protocol::base_operation), (transfer_to)(amount)(extensions))
FC_REFLECT_DERIVED( sophiatx::protocol::sponsor_fees_operation, (sophiatx::protocol::base_operation), (sponsor)(sponsored)(is_sponsoring) )
