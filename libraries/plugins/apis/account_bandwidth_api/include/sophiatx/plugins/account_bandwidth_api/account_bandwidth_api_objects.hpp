#ifndef SOPHIATX_ACCOUNT_BANDWIDTH_API_OBJECTS_HPP
#define SOPHIATX_ACCOUNT_BANDWIDTH_API_OBJECTS_HPP

#include <sophiatx/protocol/types.hpp>
#include <sophiatx/chain/account_bandwidth_object.hpp>

namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

struct account_bandwidth
{
   account_bandwidth() = default;
   account_bandwidth(const chain::account_bandwidth_object& abo) :
         act_fee_free_bandwidth(abo.act_fee_free_bandwidth),
         act_fee_free_ops_count(abo.act_fee_free_ops_count),
         last_block_num_reset(abo.last_block_num_reset),
         next_block_num_reset(abo.last_block_num_reset + SOPHIATX_LIMIT_BANDWIDTH_BLOCKS)
   {}

   uint64_t          act_fee_free_bandwidth = 0;
   uint64_t          act_fee_free_ops_count = 0;
   uint32_t          last_block_num_reset   = 0;
   uint32_t          next_block_num_reset   = 0;
};

} } } // sophiatx::plugins::account_bandwidth_api

FC_REFLECT( sophiatx::plugins::account_bandwidth_api::account_bandwidth,
            (act_fee_free_bandwidth)(act_fee_free_ops_count)(last_block_num_reset) )

#endif //SOPHIATX_ACCOUNT_BANDWIDTH_API_OBJECTS_HPP
