#include <sophiatx/chain/economics.hpp>
#include <sophiatx/chain/util/uint256.hpp>
#include <sophiatx/protocol/get_config.hpp>


namespace sophiatx { namespace chain {
using namespace sophiatx::protocol;

void economic_model_object::init_economics(share_type _init_supply, share_type _total_supply){
   share_type coinbase = _total_supply - _init_supply;
   mining_pool_from_coinbase = coinbase * sophiatx::protocol::sophiatx_config::get<int64_t>("SOPHIATX_MINING_POOL_PERCENTAGE") / SOPHIATX_100_PERCENT;
   interest_pool_from_coinbase = coinbase * sophiatx::protocol::sophiatx_config::get<int64_t>("SOPHIATX_INTEREST_POOL_PERCENTAGE") / SOPHIATX_100_PERCENT;
   promotion_pool = coinbase * sophiatx::protocol::sophiatx_config::get<int64_t>("SOPHIATX_PROMOTION_POOL_PERCENTAGE") / SOPHIATX_100_PERCENT;
   initial_promotion_pool = promotion_pool;
   init_supply = _init_supply;
   total_supply = _total_supply;
   coinbase_block_reward = mining_pool_from_coinbase / sophiatx::protocol::sophiatx_config::get<int64_t>("SOPHIATX_COINBASE_BLOCKS");
}

share_type economic_model_object::get_mining_reward(uint32_t block_number)const{
   uint32_t blocks_to_coinbase_end = sophiatx::protocol::sophiatx_config::get<uint32_t>("SOPHIATX_COINBASE_BLOCKS") - block_number;
   //mining reward consist of coinbase reward, which uniformly distributes mining pool among SOPHIATX_COINBASE_BLOCKS rewards,
   // and fees rewards, where each block is rewarded one 1/(SOPHIATX_BLOCKS_PER_DAY * 7) of the current pool.
   share_type reward = mining_pool_from_coinbase / blocks_to_coinbase_end;
   reward += mining_pool_from_fees / (SOPHIATX_BLOCKS_PER_DAY * 7);
   return reward;
}

share_type economic_model_object::withdraw_mining_reward(uint32_t block_number, uint32_t nominator, uint32_t denominator){
   //mining reward consist of coinbase reward, which uniformly distributes mining pool among SOPHIATX_COINBASE_BLOCKS rewards,
   // and fees rewards, where each block is rewarded one 1/(SOPHIATX_BLOCKS_PER_DAY * 7) of the current pool.
   if( block_number <= SOPHIATX_INTEREST_DELAY )
      return 0;
   share_type reward_from_coinbase = coinbase_block_reward;
   reward_from_coinbase = (reward_from_coinbase * nominator) / denominator;
   reward_from_coinbase = std::min(reward_from_coinbase, mining_pool_from_coinbase);

   share_type reward_from_fees = mining_pool_from_fees / (SOPHIATX_BLOCKS_PER_DAY * 7);
   reward_from_fees = (reward_from_fees * nominator) / denominator;
   reward_from_fees = std::min(reward_from_fees, mining_pool_from_fees);

   mining_pool_from_coinbase -= reward_from_coinbase;
   mining_pool_from_fees -= reward_from_fees;

   return reward_from_coinbase + reward_from_fees;
}

void economic_model_object::record_block(uint32_t generated_block, share_type current_supply){
   uint32_t interest_blocks = SOPHIATX_INTEREST_BLOCKS;
   if( generated_block >= interest_blocks )
      accumulated_supply -= historic_supply[ generated_block % interest_blocks ];
   historic_supply[ generated_block % interest_blocks ] = current_supply;
   accumulated_supply+=current_supply;
   //TODO_SOPHIATX - check invariants here.
   //check that current_supply + all_pools = total supply. Already in database.cpp
   //FC_ASSERT(mining_pool_from_coinbase + mining_pool_from_fees + interest_pool_from_coinbase + interest_pool_from_fees +
   //                promotion_pool + current_supply + burn_pool== total_supply);
}

#define SOPHIATX_TOTAL_INTERESTS ((uint64_t(SOPHIATX_TOTAL_SUPPLY) - uint64_t(SOPHIATX_INIT_SUPPLY)) * \
sophiatx::protocol::sophiatx_config::get<uint64_t>("SOPHIATX_INTEREST_POOL_PERCENTAGE") / uint64_t(SOPHIATX_100_PERCENT))

share_type economic_model_object::withdraw_interests(share_type holding, uint32_t period) {
   try {
      FC_ASSERT(holding >= 0 && interest_pool_from_coinbase >= 0 && interest_pool_from_fees >= 0 && accumulated_supply >= holding);
      if(holding == 0)
         return 0;
      share_type total_coinbase_for_period = share_type(std::min(uint64_t(interest_pool_from_coinbase.value),
                                                                 (SOPHIATX_TOTAL_INTERESTS * period /
                                                                 sophiatx::protocol::sophiatx_config::get<uint64_t>("SOPHIATX_COINBASE_BLOCKS"))));
      share_type coinbase_reward = (uint128_t(holding.value) * uint128_t(total_coinbase_for_period.value) /
                                    uint128_t(accumulated_supply.value)).to_uint64();
      share_type fees_reward = (
            uint128_t(interest_pool_from_fees.value * SOPHIATX_INTEREST_BLOCKS / SOPHIATX_INTEREST_FEES_TIME) *
            uint128_t(holding.value) / uint128_t(accumulated_supply.value)).to_uint64();
      interest_pool_from_fees -= fees_reward;
      interest_pool_from_coinbase -= coinbase_reward;
      return (fees_reward + coinbase_reward);
   } FC_CAPTURE_AND_RETHROW((holding)(period)(*this))
}

void economic_model_object::add_fee(share_type fee) {
   auto to_burn = fee * sophiatx::protocol::sophiatx_config::get<int64_t>("SOPHIATX_BURN_FEE_PERCENTAGE") / SOPHIATX_100_PERCENT;
   fee = fee - to_burn;
   auto to_mining_pool = fee * sophiatx::protocol::sophiatx_config::get<int64_t>("SOPHIATX_MINING_POOL_PERCENTAGE") / SOPHIATX_100_PERCENT;
   auto to_promotion_pool = fee * sophiatx::protocol::sophiatx_config::get<int64_t>("SOPHIATX_PROMOTION_POOL_PERCENTAGE") / SOPHIATX_100_PERCENT;
   mining_pool_from_fees += to_mining_pool;
   promotion_pool += to_promotion_pool;
   burn_pool += to_burn;
   share_type to_interests = fee - to_mining_pool - to_promotion_pool;
   interest_pool_from_fees += to_interests;
}

share_type economic_model_object::get_available_promotion_pool(uint32_t block_number) const{
   uint128_t blocks_to_coinbase_end = sophiatx::protocol::sophiatx_config::get<uint128_t>("SOPHIATX_COINBASE_BLOCKS") - block_number;
   //share_type locked_pool = promotion_pool_per_day * blocks_to_coinbase_end / SOPHIATX_BLOCKS_PER_DAY;
   share_type locked_pool = (((((uint128_t)initial_promotion_pool.value * (uint128_t)262144) / sophiatx::protocol::sophiatx_config::get<uint128_t>("SOPHIATX_COINBASE_BLOCKS")) * blocks_to_coinbase_end ) / (uint128_t)262144).to_uint64();
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
