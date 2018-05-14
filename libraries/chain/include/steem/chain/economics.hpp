#pragma once
#include <steem/protocol/config.hpp>
#include <steem/protocol/types.hpp>
#include <steem/chain/steem_object_types.hpp>




namespace steem { namespace chain {

using namespace steem::protocol;

//technically, anything in economic_model_object pool does not exists yet; any transfers (by withdraw and add methods) must affect current_supply
class economic_model_object: public object< economic_model_object_type, economic_model_object> {
public:
   template< typename Constructor, typename Allocator >
   economic_model_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   economic_model_object(){}

   id_type           id;

   share_type mining_pool_from_coinbase;
   share_type mining_pool_from_fees;
   share_type interest_pool_from_coinbase;
   share_type interest_pool_from_fees;
   uint128_t interest_coinbase_accumulator = 0;
   uint128_t interest_fees_accumulator = 0;
   share_type promotion_pool;
   share_type promotion_pool_per_day;
   share_type init_supply;
   share_type total_supply;
   share_type unallocated_interests;
   share_type interest_block_fees = 0;

   void init_economics(share_type init_supply, share_type total_supply);
   void record_block(uint32_t block, share_type current_supply);
   share_type get_mining_reward(uint32_t block_number);
   share_type withdraw_mining_reward(uint32_t block_number, uint32_t nominator, uint32_t denominator);
   share_type get_interests(share_type holding, uint128_t last_supply_acumulator, uint128_t last_fees_acumulator, uint32_t last_interest, uint32_t current_block);
   share_type withdraw_interests(share_type holding, uint128_t last_supply_acumulator, uint128_t last_fees_acumulator, uint32_t last_interest, uint32_t current_block);
   share_type get_available_promotion_pool(uint32_t block_number);
   share_type withdraw_from_promotion_pool(share_type amount, uint32_t block_number);
   void add_fee(share_type fee);

};


typedef multi_index_container<
      economic_model_object,
      indexed_by<
            ordered_unique< tag< by_id >,
                  member< economic_model_object, economic_model_object::id_type, &economic_model_object::id > >
      >,
      allocator< economic_model_object >
> economic_model_index;

}}//namespace


FC_REFLECT(steem::chain::economic_model_object,
           (mining_pool_from_coinbase)
           (mining_pool_from_fees)
           (interest_pool_from_coinbase)
           (interest_pool_from_fees)
           (interest_coinbase_accumulator)
           (interest_fees_accumulator)
           (promotion_pool)
           (promotion_pool_per_day)
           (init_supply)
           (total_supply)
           (unallocated_interests)
           (interest_block_fees)
)
CHAINBASE_SET_INDEX_TYPE( steem::chain::economic_model_object, steem::chain::economic_model_index )
