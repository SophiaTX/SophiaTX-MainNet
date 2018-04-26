#pragma once
#include <steem/protocol/config.hpp>
#include <steem/protocol/types.hpp>


namespace steem { namespace chain {

using namespace steem::protocol;

//technically, anything in economic_model pool does not exists yet; any transfers (by withdraw and add methods) must affect current_supply
class economic_model {
   share_type mining_pool_from_coinbase;
   share_type mining_pool_from_fees;
   share_type interest_pool_from_coinbase;
   share_type interest_pool_from_fees;
   share_type promotion_pool;
   share_type promotion_pool_per_block;

   void init_economics(share_type init_supply, share_type total_supply);
   share_type get_mining_reward(uint32_t block_number);
   share_type withdraw_mining_reward(uint32_t block_number);
   share_type get_interests(share_type holding, uint32_t holding_from_block, uint32_t current_block);
   share_type withdraw_interests(share_type holding, uint32_t holding_from_block, uint32_t current_block);
   share_type get_available_promotion_pool(uint32_t block_number);
   share_type withdraw_from_promotion_pool(share_type amount, uint32_t block_number);
   void add_fee(share_type fee);
};

}}//namespace


FC_REFLECT(steem::chain::economic_model,
           (mining_pool_from_coinbase)
           (mining_pool_from_fees)
           (interect_pool_from_coinbase)
           (interect_pool_from_fees)
           (promotion_pool)
)