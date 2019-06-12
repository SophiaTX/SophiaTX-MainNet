/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <sophiatx/protocol/types.hpp>

#include <fc/crypto/sha256.hpp>
#include <sophiatx/protocol/config.hpp>

#include <string>
#include <vector>

namespace sophiatx {
namespace chain {
using std::string;
using std::vector;
using namespace sophiatx::protocol;

struct genesis_state_type {

    struct initial_account_type {
        initial_account_type(const account_name_type &name = string(),
                             const public_key_type &key = public_key_type(),
                             const share_type &balance = 0
        )
                : name(name),
                  key(key),
                  balance(balance) {}

        account_name_type name;
        public_key_type key;
        share_type balance;
    };

    public_key_type initial_public_key = sophiatx::protocol::public_key_type(SOPHIATX_INIT_PUBLIC_KEY_STR);

    int64_t initial_balace = SOPHIATX_INIT_SUPPLY; ///initminer balance with typo (must stay like this for mainnet chain id_

    int64_t initial_supply = SOPHIATX_INIT_SUPPLY;
#undef SOPHIATX_INIT_SUPPLY
    int64_t total_supply = SOPHIATX_TOTAL_SUPPLY;
#undef SOPHIATX_TOTAL_SUPPLY

    uint32_t max_witnesses = SOPHIATX_MAX_WITNESSES;
#undef SOPHIATX_MAX_WITNESSES
    uint32_t hardfork_req_witnesses = SOPHIATX_HARDFORK_REQUIRED_WITNESSES;
#undef SOPHIATX_HARDFORK_REQUIRED_WITNESSES
    uint32_t max_voted_witnesses = SOPHIATX_MAX_VOTED_WITNESSES_HF0;
#undef SOPHIATX_HARDFORK_REQUIRED_WITNESSES
    uint32_t max_runner_witnesses = SOPHIATX_MAX_RUNNER_WITNESSES_HF0;
#undef SOPHIATX_MAX_RUNNER_WITNESSES_HF0
    uint32_t max_account_witness_votes = SOPHIATX_MAX_ACCOUNT_WITNESS_VOTES;
#undef SOPHIATX_MAX_ACCOUNT_WITNESS_VOTES


    uint64_t initial_witness_req_vesting_balance = SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE;
#undef SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE
    uint64_t final_witness_req_vesting_balance = SOPHIATX_FINAL_WITNESS_REQUIRED_VESTING_BALANCE;
#undef SOPHIATX_FINAL_WITNESS_REQUIRED_VESTING_BALANCE
    uint32_t witness_vesting_increase_days = SOPHIATX_WITNESS_VESTING_INCREASE_DAYS_HF_1_1;
#undef SOPHIATX_WITNESS_VESTING_INCREASE_DAYS_HF_1_1


    uint32_t block_interval = SOPHIATX_BLOCK_INTERVAL;
#undef SOPHIATX_BLOCK_INTERVAL
    uint32_t max_transaction_size = SOPHIATX_MAX_TRANSACTION_SIZE;
#undef SOPHIATX_MAX_TRANSACTION_SIZE
    uint32_t max_block_size = 2048;

    uint32_t promotion_pool_percentage = SOPHIATX_PROMOTION_POOL_PERCENTAGE;
#undef SOPHIATX_PROMOTION_POOL_PERCENTAGE
    uint32_t mining_pool_percentage = SOPHIATX_MINING_POOL_PERCENTAGE;
#undef SOPHIATX_MINING_POOL_PERCENTAGE
    uint32_t interest_pool_percentage = SOPHIATX_INTEREST_POOL_PERCENTAGE;
#undef SOPHIATX_INTEREST_POOL_PERCENTAGE
    uint32_t burn_fee_percentage = SOPHIATX_BURN_FEE_PERCENTAGE;
#undef SOPHIATX_BURN_FEE_PERCENTAGE
    uint32_t coinbase_years = SOPHIATX_COINBASE_YEARS;
#undef SOPHIATX_COINBASE_YEARS

    uint32_t min_acc_creation_fee = SOPHIATX_MIN_ACCOUNT_CREATION_FEE;
    uint32_t base_fee = 10000;
    uint32_t base_fee_usd = 10000;
    uint32_t base_fee_eur = 8000;
    uint32_t base_fee_chf = 10000;
    uint32_t base_fee_cny = 64000;
    uint32_t base_fee_gbp = 7500;

    uint32_t limit_bandwidth_blocks = SOPHIATX_LIMIT_BANDWIDTH_BLOCKS;
    uint32_t max_allowed_bandwidth = SOPHIATX_MAX_ALLOWED_BANDWIDTH;
    uint32_t max_allowed_ops = SOPHIATX_LIMIT_BANDWIDTH_BLOCKS;

    chain_id_type initial_chain_id;
    time_point_sec genesis_time = SOPHIATX_GENESIS_TIME;
    bool is_private_net = false;


    vector<initial_account_type> initial_accounts;

    /**
     * Get the chain_id corresponding to this genesis state.
     *
     * This is the SHA256 serialization of the genesis_state.
     */
    chain_id_type compute_chain_id() const { return initial_chain_id; };
};

}
} // namespace sophiatx::chain

FC_REFLECT(sophiatx::chain::genesis_state_type::initial_account_type, (name)(key)(balance))

FC_REFLECT(sophiatx::chain::genesis_state_type, (initial_public_key)
        (initial_balace)
        (total_supply)
        (max_witnesses)
        (hardfork_req_witnesses)
        (max_voted_witnesses)
        (max_runner_witnesses)
        (max_account_witness_votes)
        (initial_witness_req_vesting_balance)
        (final_witness_req_vesting_balance)
        (witness_vesting_increase_days)
        (block_interval)
        (max_transaction_size)
        (max_block_size)
        (promotion_pool_percentage)
        (mining_pool_percentage)
        (interest_pool_percentage)
        (burn_fee_percentage)
        (coinbase_years)
        (min_acc_creation_fee)
        (base_fee)
        (base_fee_usd)
        (base_fee_eur)
        (base_fee_chf)
        (base_fee_cny)
        (base_fee_gbp)
        (limit_bandwidth_blocks)
        (max_allowed_bandwidth)
        (max_allowed_ops)
        (initial_chain_id)
        (genesis_time)
        (is_private_net)
        (initial_accounts)
)