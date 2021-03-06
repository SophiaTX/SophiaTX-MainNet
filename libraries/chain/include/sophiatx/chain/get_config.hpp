#pragma once

#include <sophiatx/protocol/config.hpp>
#include <sophiatx/protocol/protocol_config.hpp>
#include <sophiatx/protocol/asset.hpp>
#include <sophiatx/protocol/types.hpp>
#include <sophiatx/protocol/version.hpp>
#include <sophiatx/chain/genesis_state.hpp>

namespace sophiatx { namespace chain {

class sophiatx_config {

public:
    inline static void init(const fc::variant &config)
    {
        FC_ASSERT(config.get_object().size(), "can not init sophiatx_config with empty object!");
        instance().config_ = config.get_object();
        instance().config_loaded_ = true;
        protocol::protocol_config::init(instance().config_);
    }

    inline static void init(const chain::genesis_state_type &genesis)
    {
        instance().config_loaded_ = true;
#ifdef IS_TEST_NET
        instance().config_["IS_TEST_NET"] = true;
        instance().config_["SOPHIATX_MIN_FEEDS"] = 1;
#else
        instance().config_["IS_TEST_NET"] = false;
        instance().config_["SOPHIATX_MIN_FEEDS"] = genesis.max_witnesses/10; /// protects the network from conversions before price has been established
#endif

        instance().config_["SOPHIATX_BLOCKCHAIN_VERSION"] = SOPHIATX_BLOCKCHAIN_VERSION;
        instance().config_["SOPHIATX_CHAIN_ID"] = genesis.compute_chain_id();
        instance().config_["IS_PRIVATE_NET"] = genesis.is_private_net;
        instance().config_["SOPHIATX_INIT_PUBLIC_KEY"] = genesis.initial_public_key;
        instance().config_["SOPHIATX_INIT_PUBLIC_MINING_KEY"] = genesis.initial_public_mining_key;
        instance().config_["SOPHIATX_MIN_ACCOUNT_CREATION_FEE"] = genesis.min_acc_creation_fee;
        instance().config_["SOPHIATX_OWNER_AUTH_RECOVERY_PERIOD"] = SOPHIATX_OWNER_AUTH_RECOVERY_PERIOD;
        instance().config_["SOPHIATX_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD"] = SOPHIATX_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
        instance().config_["SOPHIATX_OWNER_UPDATE_LIMIT"] = SOPHIATX_OWNER_UPDATE_LIMIT;
        instance().config_["SOPHIATX_INIT_SUPPLY"] = genesis.initial_supply;
        instance().config_["SOPHIATX_TOTAL_SUPPLY"] = genesis.total_supply;

        instance().config_["SOPHIATX_GENESIS_TIME"] = genesis.genesis_time;
        instance().config_["SOPHIATX_BLOCK_INTERVAL"] = genesis.block_interval;
        instance().config_["SOPHIATX_BLOCKS_PER_YEAR"] = 365*24*60*60/genesis.block_interval;
        instance().config_["SOPHIATX_BLOCKS_PER_DAY"] = 24*60*60/genesis.block_interval;
        instance().config_["SOPHIATX_START_MINER_VOTING_BLOCK"] = 24*60*60/genesis.block_interval * 7; ///SOPHIATX_BLOCKS_PER_DAY
        instance().config_["SOPHIATX_DECIMALS"] = SOPHIATX_DECIMALS;
        instance().config_["SOPHIATX_SATOSHIS"] = SOPHIATX_SATOSHIS;
        instance().config_["SOPHIATX_PROMOTION_POOL_PERCENTAGE"] = genesis.promotion_pool_percentage;
        instance().config_["SOPHIATX_MINING_POOL_PERCENTAGE"] = genesis.mining_pool_percentage;
        instance().config_["SOPHIATX_INTEREST_POOL_PERCENTAGE"] = genesis.interest_pool_percentage;
        instance().config_["SOPHIATX_BURN_FEE_PERCENTAGE"] = genesis.burn_fee_percentage;
        instance().config_["SOPHIATX_INTEREST_BLOCKS"] = SOPHIATX_INTEREST_BLOCKS;
        instance().config_["SOPHIATX_INTEREST_FEES_TIME"] = 24*60*60/genesis.block_interval; ///SOPHIATX_BLOCKS_PER_DAY
        instance().config_["SOPHIATX_INTEREST_DELAY"] = 24*60*60/genesis.block_interval; ///SOPHIATX_BLOCKS_PER_DAY
        instance().config_["SOPHIATX_COINBASE_YEARS"] = genesis.coinbase_years;
        instance().config_["SOPHIATX_COINBASE_BLOCKS"] = 365*24*60*60/genesis.block_interval * genesis.coinbase_years;  ///SOPHIATX_BLOCKS_PER_YEAR

        instance().config_["SOPHIATX_FINAL_WITNESS_REQUIRED_VESTING_BALANCE"] = genesis.final_witness_req_vesting_balance;
        instance().config_["SOPHIATX_WITNESS_VESTING_INCREASE_DAYS_HF"] = genesis.witness_vesting_increase_days;
        instance().config_["SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE"] = genesis.initial_witness_req_vesting_balance;

        instance().config_["VESTS_SYMBOL"] = VESTS_SYMBOL;
        instance().config_["SOPHIATX_SYMBOL"] = genesis.symbol;
        instance().config_["SBD1_SYMBOL"] = SBD1_SYMBOL;
        instance().config_["SBD2_SYMBOL"] = SBD2_SYMBOL;
        instance().config_["SBD3_SYMBOL"] = SBD3_SYMBOL;
        instance().config_["SBD4_SYMBOL"] = SBD4_SYMBOL;
        instance().config_["SBD5_SYMBOL"] = SBD5_SYMBOL;
        instance().config_["BASE_FEE"] = asset(genesis.base_fee, genesis.symbol);
        instance().config_["BASE_FEE_SBD1"] = asset(genesis.base_fee_usd, SBD1_SYMBOL);
        instance().config_["BASE_FEE_SBD2"] = asset(genesis.base_fee_eur, SBD2_SYMBOL);
        instance().config_["BASE_FEE_SBD3"] = asset(genesis.base_fee_chf, SBD3_SYMBOL);
        instance().config_["BASE_FEE_SBD4"] = asset(genesis.base_fee_cny, SBD4_SYMBOL);
        instance().config_["BASE_FEE_SBD5"] = asset(genesis.base_fee_gbp, SBD5_SYMBOL);
        instance().config_["SIZE_COVERED_IN_BASE_FEE"] = SIZE_COVERED_IN_BASE_FEE;
        instance().config_["SIZE_INCREASE_PER_FEE"] = SIZE_INCREASE_PER_FEE;
        instance().config_["SOPHIATX_BLOCKCHAIN_HARDFORK_VERSION"] = SOPHIATX_BLOCKCHAIN_HARDFORK_VERSION;
        instance().config_["SOPHIATX_INIT_MINER_NAME"] = SOPHIATX_INIT_MINER_NAME;
        instance().config_["SOPHIATX_NUM_INIT_MINERS"] = SOPHIATX_NUM_INIT_MINERS;
        instance().config_["SOPHIATX_MAX_WITNESSES"] = genesis.max_witnesses;
        instance().config_["SOPHIATX_MAX_VOTED_WITNESSES_HF0"] = genesis.max_voted_witnesses;
        instance().config_["SOPHIATX_MAX_RUNNER_WITNESSES_HF0"] = genesis.max_runner_witnesses;
        instance().config_["SOPHIATX_HARDFORK_REQUIRED_WITNESSES"] = genesis.hardfork_req_witnesses;
        instance().config_["SOPHIATX_MAX_TIME_UNTIL_EXPIRATION"] = SOPHIATX_MAX_TIME_UNTIL_EXPIRATION;
        instance().config_["SOPHIATX_MAX_MEMO_SIZE"] = SOPHIATX_MAX_MEMO_SIZE;
        instance().config_["SOPHIATX_MAX_NAME_SEED_SIZE"] = SOPHIATX_MAX_NAME_SEED_SIZE;
        instance().config_["SOPHIATX_MAX_PROXY_RECURSION_DEPTH"] = SOPHIATX_MAX_PROXY_RECURSION_DEPTH;
        instance().config_["SOPHIATX_VESTING_WITHDRAW_INTERVALS"] = SOPHIATX_VESTING_WITHDRAW_INTERVALS;
        instance().config_["SOPHIATX_VESTING_WITHDRAW_INTERVAL_SECONDS"] = SOPHIATX_VESTING_WITHDRAW_INTERVAL_SECONDS;
        instance().config_["SOPHIATX_MAX_ACCOUNT_WITNESS_VOTES"] = genesis.max_account_witness_votes;
        instance().config_["SOPHIATX_100_PERCENT"] = SOPHIATX_100_PERCENT;
        instance().config_["SOPHIATX_1_PERCENT"] = SOPHIATX_1_PERCENT;
        instance().config_["SOPHIATX_LIMIT_BANDWIDTH_BLOCKS"] = genesis.limit_bandwidth_blocks;
        instance().config_["SOPHIATX_MAX_ALLOWED_BANDWIDTH"] = genesis.max_allowed_bandwidth;
        instance().config_["SOPHIATX_MAX_ALLOWED_OPS_COUNT"] = genesis.max_allowed_ops;
        instance().config_["SOPHIATX_MIN_ACCOUNT_NAME_LENGTH"] = SOPHIATX_MIN_ACCOUNT_NAME_LENGTH;
        instance().config_["SOPHIATX_MAX_ACCOUNT_NAME_LENGTH"] = SOPHIATX_MAX_ACCOUNT_NAME_LENGTH;
        instance().config_["SOPHIATX_MAX_PERMLINK_LENGTH"] = SOPHIATX_MAX_PERMLINK_LENGTH;
        instance().config_["SOPHIATX_MAX_WITNESS_URL_LENGTH"] = SOPHIATX_MAX_WITNESS_URL_LENGTH;
        instance().config_["SOPHIATX_MAX_SHARE_SUPPLY"] = SOPHIATX_MAX_SHARE_SUPPLY;
        instance().config_["SOPHIATX_MAX_SATOSHIS"] = SOPHIATX_MAX_SATOSHIS;
        instance().config_["SOPHIATX_MAX_SIG_CHECK_DEPTH"] = SOPHIATX_MAX_SIG_CHECK_DEPTH;
        instance().config_["SOPHIATX_MIN_TRANSACTION_SIZE_LIMIT"] = SOPHIATX_MIN_TRANSACTION_SIZE_LIMIT;
        instance().config_["SOPHIATX_MAX_TRANSACTION_SIZE"] = genesis.max_transaction_size;
        instance().config_["SOPHIATX_MIN_BLOCK_SIZE_LIMIT"] = genesis.max_transaction_size * 16;
        instance().config_["SOPHIATX_MAX_BLOCK_SIZE"] = genesis.max_transaction_size * genesis.block_interval * genesis.max_block_size;
        instance().config_["SOPHIATX_MIN_BLOCK_SIZE"] = SOPHIATX_MIN_BLOCK_SIZE;
        instance().config_["SOPHIATX_BLOCKS_PER_HOUR"] = 60*60/genesis.block_interval;
        instance().config_["SOPHIATX_FEED_HISTORY_WINDOW"] = SOPHIATX_FEED_HISTORY_WINDOW;
        instance().config_["SOPHIATX_MAX_FEED_AGE_SECONDS"] = SOPHIATX_MAX_FEED_AGE_SECONDS;
        instance().config_["SOPHIATX_MAX_UNDO_HISTORY"] = SOPHIATX_MAX_UNDO_HISTORY;
        instance().config_["SOPHIATX_IRREVERSIBLE_THRESHOLD"] = SOPHIATX_IRREVERSIBLE_THRESHOLD;
        instance().config_["SOPHIATX_VIRTUAL_SCHEDULE_LAP_LENGTH"] = SOPHIATX_VIRTUAL_SCHEDULE_LAP_LENGTH;
        instance().config_["SOPHIATX_VIRTUAL_SCHEDULE_LAP_LENGTH2"] = SOPHIATX_VIRTUAL_SCHEDULE_LAP_LENGTH2;
        instance().config_["SOPHIATX_MINER_ACCOUNT"] = SOPHIATX_MINER_ACCOUNT;
        instance().config_["SOPHIATX_NULL_ACCOUNT"] = SOPHIATX_NULL_ACCOUNT;
        instance().config_["SOPHIATX_TEMP_ACCOUNT"] = SOPHIATX_TEMP_ACCOUNT;
        instance().config_["SOPHIATX_PROXY_TO_SELF_ACCOUNT"] = SOPHIATX_PROXY_TO_SELF_ACCOUNT;

        protocol::protocol_config::init(instance().config_);
    }

    inline static const fc::mutable_variant_object& get_config() {
        FC_ASSERT(instance().config_loaded_, "sophiatx_config is not initialized!");
        return instance().config_;
    }

    template<typename T>
    inline static T get( std::string_view index ) {
        FC_ASSERT(instance().config_loaded_, "sophiatx_config is not initialized!");
        T type;
        fc::from_variant(instance().config_[index], type);
        return type;
    }

private:
    sophiatx_config() : config_loaded_(false) {}
    ~sophiatx_config() {}

    inline static sophiatx_config &instance() {
        static sophiatx_config instance;
        return instance;
    }

    bool config_loaded_;
    fc::mutable_variant_object config_;
};

} } // sophiatx::protocol
