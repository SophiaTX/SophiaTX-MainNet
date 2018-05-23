#include <steem/chain/economics.hpp>
#include <steem/chain/util/uint256.hpp>


namespace steem { namespace chain {
namespace {
   uint128_t multiplier(uint64_t(0x0fffffffffffffff), uint64_t(0xffffffffffffffff));
   share_type get_next_block_interests(uint32_t block, share_type remaining_interests){
      if(block >= SOPHIATX_COINBASE_BLOCKS )
         return 0;
      return remaining_interests / (SOPHIATX_COINBASE_BLOCKS - block + 1);
   }
}

using namespace steem::protocol;


void economic_model_object::init_economics(share_type _init_supply, share_type _total_supply){
   share_type coinbase = _total_supply - _init_supply;
   mining_pool_from_coinbase = coinbase * SOPHIATX_MINING_POOL_PERCENTAGE / STEEM_100_PERCENT;
   interest_pool_from_coinbase = coinbase * SOPHIATX_INTEREST_POOL_PERCENTAGE / STEEM_100_PERCENT;
   unallocated_interests = interest_pool_from_coinbase;
   promotion_pool = coinbase * SOPHIATX_PROMOTION_POOL_PERCENTAGE / STEEM_100_PERCENT;
   initial_promotion_pool = promotion_pool;
   init_supply = _init_supply;
   total_supply = _total_supply;
}

share_type economic_model_object::get_mining_reward(uint32_t block_number)const{
   uint32_t blocks_to_coinbase_end = SOPHIATX_COINBASE_BLOCKS - block_number;
   //mining reward consist of coinbase reward, which uniformly distributes mining pool among SOPHIATX_COINBASE_BLOCKS rewards,
   // and fees rewards, where each block is rewarded one 1/(STEEM_BLOCKS_PER_DAY * 7) of the current pool.
   share_type reward = mining_pool_from_coinbase / blocks_to_coinbase_end;
   reward += mining_pool_from_fees / (STEEM_BLOCKS_PER_DAY * 7);
   return reward;
}

share_type economic_model_object::withdraw_mining_reward(uint32_t block_number, uint32_t nominator, uint32_t denominator){
   uint32_t blocks_to_coinbase_end = SOPHIATX_COINBASE_BLOCKS - block_number+1;
   //mining reward consist of coinbase reward, which uniformly distributes mining pool among SOPHIATX_COINBASE_BLOCKS rewards,
   // and fees rewards, where each block is rewarded one 1/(STEEM_BLOCKS_PER_DAY * 7) of the current pool.
   share_type reward_from_coinbase = mining_pool_from_coinbase / blocks_to_coinbase_end;
   reward_from_coinbase = (reward_from_coinbase * nominator) / denominator;
   reward_from_coinbase = std::min(reward_from_coinbase, mining_pool_from_coinbase);

   share_type reward_from_fees = mining_pool_from_fees / (STEEM_BLOCKS_PER_DAY * 7);
   reward_from_fees = (reward_from_fees * nominator) / denominator;
   reward_from_fees = std::min(reward_from_fees, mining_pool_from_fees);

   mining_pool_from_coinbase -= reward_from_coinbase;
   mining_pool_from_fees -= reward_from_fees;

   return reward_from_coinbase + reward_from_fees;
}

void economic_model_object::record_block(uint32_t generated_block, share_type current_supply){
   uint128_t next_block_interests = get_next_block_interests(generated_block, unallocated_interests).value;
   unallocated_interests -= next_block_interests.lo;
   FC_ASSERT((unallocated_interests)>= util::to256(uint64_t(0)));
   uint128_t supply_share = multiplier / uint128_t(current_supply.value);
   interest_coinbase_accumulator += supply_share * next_block_interests;
   interest_fees_accumulator += supply_share * uint128_t(interest_block_fees.value);
   interest_block_fees = 0;
   //TODO_SOPHIATX - check invariants here.
}

share_type economic_model_object::get_interests(share_type holding, uint128_t last_supply_acumulator, uint128_t last_fees_acumulator, uint32_t last_interest, uint32_t current_block)const{
   u256 coinbase_reward = util::to256(interest_coinbase_accumulator - last_supply_acumulator) * u256(holding.value) / util::to256(multiplier);
   u256 fees_reward = util::to256(interest_fees_accumulator - last_fees_acumulator) * u256(holding.value) / util::to256(multiplier);

   FC_ASSERT(( coinbase_reward+fees_reward) < u256(total_supply.value) );
   return ( coinbase_reward+fees_reward).convert_to<uint64_t>();
}

share_type economic_model_object::withdraw_interests(share_type holding, uint128_t last_supply_acumulator, uint128_t last_fees_acumulator, uint32_t last_interest, uint32_t current_block){

   u256 coinbase_reward = util::to256(interest_coinbase_accumulator - last_supply_acumulator) * u256(holding.value) / util::to256(multiplier);
   u256 fees_reward = util::to256(interest_fees_accumulator - last_fees_acumulator) * u256(holding.value) / util::to256(multiplier);

   interest_pool_from_coinbase -= coinbase_reward;
   interest_pool_from_fees -= fees_reward;

   FC_ASSERT(( coinbase_reward+fees_reward) < u256(total_supply.value) );
   return ( coinbase_reward+fees_reward).convert_to<uint64_t>();
}

void economic_model_object::add_fee(share_type fee) {
   auto to_mining_pool = fee * SOPHIATX_MINING_POOL_PERCENTAGE / STEEM_100_PERCENT;
   auto to_promotion_pool = fee * SOPHIATX_PROMOTION_POOL_PERCENTAGE / STEEM_100_PERCENT;
   mining_pool_from_fees += to_mining_pool;
   promotion_pool += to_promotion_pool;
   share_type to_interests = fee - to_mining_pool - to_promotion_pool;
   interest_pool_from_fees += to_interests;
   interest_block_fees += to_interests;
}

share_type economic_model_object::get_available_promotion_pool(uint32_t block_number) const{
   uint32_t blocks_to_coinbase_end = SOPHIATX_COINBASE_BLOCKS - block_number;
   //share_type locked_pool = promotion_pool_per_day * blocks_to_coinbase_end / STEEM_BLOCKS_PER_DAY;
   share_type locked_pool = (((initial_promotion_pool * 65536) / SOPHIATX_COINBASE_BLOCKS) * blocks_to_coinbase_end ) / 65536;
   FC_ASSERT(promotion_pool >= locked_pool);
   return promotion_pool - locked_pool;
}

share_type economic_model_object::withdraw_from_promotion_pool(share_type amount, uint32_t block_number) {
   share_type free_pool = get_available_promotion_pool(block_number);
   share_type to_withdraw = std::min(free_pool, amount);
   promotion_pool -= to_withdraw;
   FC_ASSERT(to_withdraw.value > 0);
   return to_withdraw;
}



}}//namespace
