#include <steem/chain/economics.hpp>




namespace steem { namespace chain {

using namespace steem::protocol;

void economic_model::add_fee(share_type fee) {
   mining_pool_from_fees += fee * SOPHIATX_MINING_POOL_PERCENTAGE / STEEM_100_PERCENT;
   promotion_pool += fee * SOPHIATX_PROMOTION_POOL_PERCENTAGE / STEEM_100_PERCENT;
   interest_pool_from_fees += fee * SOPHIATX_INTEREST_POOL_PERCENTAGE / STEEM_100_PERCENT;
}

share_type economic_model::get_available_promotion_pool(uint32_t block_number) {
   uint32_t blocks_to_coinbase_end = SOPHIATX_COINBASE_BLOCKS - block_number;
   share_type locked_pool = promotion_pool_per_block * blocks_to_coinbase_end;
   FC_ASSERT(promotion_pool > locked_pool);
   return promotion_pool - locked_pool;
}

share_type economic_model::withdraw_from_promotion_pool(share_type amount, uint32_t block_number) {
   share_type free_pool = get_available_promotion_pool(block_number);
   share_type to_withdraw = std::min(free_pool, amount);
   promotion_pool -= to_withdraw;
   FC_ASSERT(to_withdraw.value > 0);
   return to_withdraw;
}



}}//namespace