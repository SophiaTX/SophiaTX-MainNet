#include <sophiatx/protocol/sophiatx_operations.hpp>

#include <fc/macros.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/base64m.hpp>
#include <locale>

namespace sophiatx { namespace protocol {

   std::string make_random_fixed_string(std::string seed)
   {
      auto hash = fc::ripemd160::hash(seed);
      unsigned char data[21];
      memcpy(data, hash.data(), 20);
      data[20] = 0; //do the padding to avoid '=' at the end of the result string

      std::string s = fc::base64m_encode(data, 21);
      return s;
   }

   void account_create_operation::validate() const
   {
      //TODO: Add this check to the hardfork1
      FC_ASSERT( name_seed.size() <= SOPHIATX_MAX_NAME_SEED_SIZE, "Name seed is too large" );
      owner.validate();
      active.validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }


   void account_update_operation::validate() const
   {
      validate_account_name( account );
      /*if( owner )
         owner->validate();
      if( active )
         active->validate();
      if( posting )
         posting->validate();*/

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   void account_delete_operation::validate() const
   {
      validate_account_name( account );
   }

   void placeholder_a_operation::validate()const
   {
      FC_ASSERT( false, "This is not a valid op" );
   }

   void placeholder_b_operation::validate()const
   {
      FC_ASSERT( false, "This is not a valid op" );
   }

   void transfer_operation::validate() const
   { try {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.symbol != VESTS_SYMBOL, "transferring of SophiaTX Power (STMP) is not allowed." );
      FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
      FC_ASSERT( memo.size() < SOPHIATX_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void transfer_to_vesting_operation::validate() const
   {
      validate_account_name( from );
      FC_ASSERT( is_asset_type( amount, SOPHIATX_SYMBOL ), "Amount must be SOPHIATX" );
      if ( to != account_name_type() ) validate_account_name( to );
      FC_ASSERT( amount > asset( 0, SOPHIATX_SYMBOL ), "Must transfer a nonzero amount" );
   }

   void withdraw_vesting_operation::validate() const
   {
      validate_account_name( account );
      FC_ASSERT( is_asset_type( vesting_shares, VESTS_SYMBOL), "Amount must be VESTS"  );
   }

   void witness_update_operation::validate() const
   {
#ifdef PRIVATE_NET
      FC_ASSERT( owner == SOPHIATX_INIT_MINER_NAME );
#endif //PRIVATE_NET

      validate_account_name( owner );

      FC_ASSERT( url.size() <= SOPHIATX_MAX_WITNESS_URL_LENGTH, "URL is too long" );

      FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );
      props.validate();
   }

   void witness_set_properties_operation::validate() const
   {
      validate_account_name( owner );

      // current signing key must be present
      FC_ASSERT( props.find( "key" ) != props.end(), "No signing key provided" );

      auto itr = props.find( "account_creation_fee" );
      if( itr != props.end() )
      {
         asset account_creation_fee;
         fc::raw::unpack_from_vector( itr->second, account_creation_fee );
         FC_ASSERT( account_creation_fee.symbol == SOPHIATX_SYMBOL, "account_creation_fee must be in SOPHIATX" );
         FC_ASSERT( account_creation_fee.amount >= SOPHIATX_MIN_ACCOUNT_CREATION_FEE , "account_creation_fee smaller than minimum account creation fee" );
      }

      itr = props.find( "maximum_block_size" );
      if( itr != props.end() )
      {
         uint32_t maximum_block_size;
         fc::raw::unpack_from_vector( itr->second, maximum_block_size );
         FC_ASSERT( maximum_block_size >= SOPHIATX_MIN_BLOCK_SIZE_LIMIT, "maximum_block_size smaller than minimum max block size" );
      }

      itr = props.find( "new_signing_key" );
      if( itr != props.end() )
      {
         public_key_type signing_key;
         fc::raw::unpack_from_vector( itr->second, signing_key );
         FC_UNUSED( signing_key ); // This tests the deserialization of the key
      }

      itr = props.find( "exchange_rates" );
      if( itr != props.end() )
      {
         std::vector<price> exchange_rates;
         fc::raw::unpack_from_vector( itr->second, exchange_rates );
         for(const auto &rate: exchange_rates){
            if(rate.base.symbol == SOPHIATX_SYMBOL){
               FC_ASSERT(rate.quote.symbol == SBD1_SYMBOL || rate.quote.symbol == SBD2_SYMBOL ||
                         rate.quote.symbol == SBD3_SYMBOL || rate.quote.symbol == SBD4_SYMBOL ||
                         rate.quote.symbol == SBD5_SYMBOL );
               
            }else{
               FC_ASSERT(rate.quote.symbol == SOPHIATX_SYMBOL);
               FC_ASSERT(rate.base.symbol == SBD1_SYMBOL || rate.base.symbol == SBD2_SYMBOL ||
                         rate.base.symbol == SBD3_SYMBOL || rate.base.symbol == SBD4_SYMBOL ||
                         rate.base.symbol == SBD5_SYMBOL );
            }
            FC_ASSERT(rate.quote.amount > 0 && rate.base.amount > 0);

         }
         //check if all symbols are unique
      }

      itr = props.find( "url" );
      if( itr != props.end() )
      {
         std::string url;
         fc::raw::unpack_from_vector< std::string >( itr->second, url );

         FC_ASSERT( url.size() <= SOPHIATX_MAX_WITNESS_URL_LENGTH, "URL is too long" );
         FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
         FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );
      }

   }

   void account_witness_vote_operation::validate() const
   {
      validate_account_name( account );
      validate_account_name( witness );
   }

   void account_witness_proxy_operation::validate() const
   {
      validate_account_name( account );
      if( proxy.size() )
         validate_account_name( proxy );
      FC_ASSERT( proxy != account, "Cannot proxy to self" );
   }

   void custom_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      validate_account_name(sender);
      for(const auto r: recipients)
         validate_account_name(r);
   }

   void custom_json_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( fc::is_utf8(json), "JSON Metadata not formatted in UTF8" );
      FC_ASSERT( fc::json::is_valid(json), "JSON Metadata not valid JSON" );
      validate_account_name(sender);
      for(const auto r: recipients)
         validate_account_name(r);
   }

   void custom_binary_operation::validate() const {
      validate_account_name(sender);
      for(const auto r: recipients)
         validate_account_name(r);
   }

   void feed_publish_operation::validate()const
   {
      validate_account_name( publisher );
      if(exchange_rate.base.symbol == SOPHIATX_SYMBOL){
         FC_ASSERT(exchange_rate.quote.symbol == SBD1_SYMBOL || exchange_rate.quote.symbol == SBD2_SYMBOL ||
                   exchange_rate.quote.symbol == SBD3_SYMBOL || exchange_rate.quote.symbol == SBD4_SYMBOL ||
                   exchange_rate.quote.symbol == SBD5_SYMBOL );

      }else{
         FC_ASSERT(exchange_rate.quote.symbol == SOPHIATX_SYMBOL);
         FC_ASSERT(exchange_rate.base.symbol == SBD1_SYMBOL || exchange_rate.base.symbol == SBD2_SYMBOL ||
                   exchange_rate.base.symbol == SBD3_SYMBOL || exchange_rate.base.symbol == SBD4_SYMBOL ||
                   exchange_rate.base.symbol == SBD5_SYMBOL );
      }
      FC_ASSERT(exchange_rate.quote.amount > 0 && exchange_rate.base.amount > 0);
      exchange_rate.validate();
   }


   void escrow_transfer_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      FC_ASSERT( escrow_fee.amount >= 0, "fee cannot be negative" );
      FC_ASSERT( sophiatx_amount.amount > 0, "sophiatx amount cannot be negative" );
      FC_ASSERT( from != agent && to != agent, "agent must be a third party" );
      FC_ASSERT( (escrow_fee.symbol == SOPHIATX_SYMBOL) , "fee must be SOPHIATX" );
      FC_ASSERT( sophiatx_amount.symbol == SOPHIATX_SYMBOL, "sophiatx amount must contain SOPHIATX" );
      FC_ASSERT( ratification_deadline < escrow_expiration, "ratification deadline must be before escrow expiration" );
      if ( json_meta.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_meta), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_meta), "JSON Metadata not valid JSON" );
      }
   }

   void escrow_approve_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      FC_ASSERT( who == to || who == agent, "to or agent must approve escrow" );
   }

   void escrow_dispute_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      FC_ASSERT( who == from || who == to, "who must be from or to" );
   }

   void escrow_release_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      validate_account_name( receiver );
      FC_ASSERT( who == from || who == to || who == agent, "who must be from or to or agent" );
      FC_ASSERT( receiver == from || receiver == to, "receiver must be from or to" );
      FC_ASSERT( sophiatx_amount.amount > 0, "sophiatx amount must be positive" );
      FC_ASSERT( sophiatx_amount.symbol == SOPHIATX_SYMBOL, "sophiatx amount must contain SOPHIATX" );
   }

   void request_account_recovery_operation::validate()const
   {
      validate_account_name( recovery_account );
      validate_account_name( account_to_recover );
      new_owner_authority.validate();
   }

   void recover_account_operation::validate()const
   {
      validate_account_name( account_to_recover );
      FC_ASSERT( !( new_owner_authority == recent_owner_authority ), "Cannot set new owner authority to the recent owner authority" );
      FC_ASSERT( !new_owner_authority.is_impossible(), "new owner authority cannot be impossible" );
      FC_ASSERT( !recent_owner_authority.is_impossible(), "recent owner authority cannot be impossible" );
      FC_ASSERT( new_owner_authority.weight_threshold, "new owner authority cannot be trivial" );
      new_owner_authority.validate();
      recent_owner_authority.validate();
   }

   void change_recovery_account_operation::validate()const
   {
      validate_account_name( account_to_recover );
      validate_account_name( new_recovery_account );
   }

   void reset_account_operation::validate()const
   {
      validate_account_name( reset_account );
      validate_account_name( account_to_reset );
      FC_ASSERT( !new_owner_authority.is_impossible(), "new owner authority cannot be impossible" );
      FC_ASSERT( new_owner_authority.weight_threshold, "new owner authority cannot be trivial" );
      new_owner_authority.validate();
   }

   void set_reset_account_operation::validate()const
   {
      validate_account_name( account );
      if( current_reset_account.size() )
         validate_account_name( current_reset_account );
      validate_account_name( reset_account );
      FC_ASSERT( current_reset_account != reset_account, "new reset account cannot be current reset account" );
   }

   void application_create_operation::validate() const
   {
      validate_account_name( author );

      FC_ASSERT( name.size() <= SOPHIATX_MAX_PERMLINK_LENGTH, "Name is too long" );
      FC_ASSERT( name.size() > 0, "Name size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( name ), "Name is not valid UTF8" );

      FC_ASSERT( url.size() <= SOPHIATX_MAX_WITNESS_URL_LENGTH, "URL is too long" );
      FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );

      if ( metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(metadata), "JSON Metadata not formatted in UTF8" );
      }
      FC_ASSERT( price_param < static_cast<uint8_t >(none), "Undefined price param" );
   }

   void application_update_operation::validate() const
   {
      validate_account_name( author );
      if(new_author)
         validate_account_name( *new_author );

      FC_ASSERT( name.size() <= SOPHIATX_MAX_PERMLINK_LENGTH, "Name is too long" );
      FC_ASSERT( name.size() > 0, "Name size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( name ), "Name is not valid UTF8" );

      FC_ASSERT( url.size() <= SOPHIATX_MAX_WITNESS_URL_LENGTH, "URL is too long" );
      FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );

      if ( metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(metadata), "JSON Metadata not formatted in UTF8" );
      }
      FC_ASSERT( *price_param < static_cast<uint8_t >(none), "Undefined price param" );
   }

   void application_delete_operation::validate() const
   {
      validate_account_name( author );

      FC_ASSERT( name.size() <= SOPHIATX_MAX_PERMLINK_LENGTH, "Name is too long" );
      FC_ASSERT( name.size() > 0, "Name size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( name ), "Name is not valid UTF8" );
   }

#ifdef SOPHIATX_ENABLE_SMT
   void claim_reward_balance2_operation::validate()const
   {
      validate_account_name( account );
      FC_ASSERT( reward_tokens.empty() == false, "Must claim something." );
      FC_ASSERT( reward_tokens.begin()->amount >= 0, "Cannot claim a negative amount" );
      bool is_substantial_reward = reward_tokens.begin()->amount > 0;
      for( auto itl = reward_tokens.begin(), itr = itl+1; itr != reward_tokens.end(); ++itl, ++itr )
      {
         FC_ASSERT( itl->symbol.to_nai() <= itr->symbol.to_nai(), 
                    "Reward tokens have not been inserted in ascending order." );
         FC_ASSERT( itl->symbol.to_nai() != itr->symbol.to_nai(), 
                    "Duplicate symbol ${s} inserted into claim reward operation container.", ("s", itl->symbol) );
         FC_ASSERT( itr->amount >= 0, "Cannot claim a negative amount" );
         is_substantial_reward |= itr->amount > 0;
      }
      FC_ASSERT( is_substantial_reward, "Must claim something." );
   }
#endif

   void transfer_from_promotion_pool_operation::validate()const
   {
      validate_account_name(transfer_to);
      FC_ASSERT(amount.amount > 0);
   }

   void sponsor_fees_operation::validate() const
   {
      if(sponsor != "") validate_account_name(sponsor);
      validate_account_name(sponsored);
   }
   
   void buy_application_operation::validate() const
   {
      validate_account_name( buyer );
   }

   void cancel_application_buying_operation::validate() const
   {
       validate_account_name( app_owner );
       validate_account_name( buyer );
   }

} } // sophiatx::protocol
