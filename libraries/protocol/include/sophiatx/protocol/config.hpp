/*
 * Copyright (c) 2016 Steemit, Inc., Equidato Technologies and contributors.
 */
#pragma once
#include <sophiatx/protocol/hardfork.hpp>

#include <fc/variant_object.hpp>
#include <fc/exception/exception.hpp>

// WARNING!
// Every symbol defined here needs to be handled appropriately in get_config.hpp
// This is checked by get_config_check.sh called from Dockerfile

#define SOPHIATX_BLOCKCHAIN_VERSION              ( version(1, 2, 0) )


#ifdef IS_TEST_NET

#define SOPHIATX_INIT_PRIVATE_KEY                (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
#define SOPHIATX_INIT_PUBLIC_KEY_STR             (std::string( sophiatx::protocol::public_key_type(SOPHIATX_INIT_PRIVATE_KEY.get_public_key()) ))
#define SOPHIATX_MIN_ACCOUNT_CREATION_FEE          0

#define SOPHIATX_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define SOPHIATX_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define SOPHIATX_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define SOPHIATX_HARDFORK_REQUIRED_WITNESSES     1

#define SOPHIATX_INIT_SUPPLY                     (int64_t( 350 ) * int64_t( 1000000 ) * int64_t( 1000000 ))
#define SOPHIATX_TOTAL_SUPPLY                    (int64_t( 500 ) * int64_t( 1000000 ) * int64_t( 1000000 ))

#else // IS LIVE SOPHIATX NETWORK

#define SOPHIATX_INIT_PUBLIC_KEY_STR             "SPH78w3H1TUaKCysbF8p2ZQ12Mutrq3NJzr41zMPVQLETyP94cVbX" //used for mining
#define SOPHIATX_MIN_ACCOUNT_CREATION_FEE           50000

#define SOPHIATX_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define SOPHIATX_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define SOPHIATX_OWNER_UPDATE_LIMIT                          fc::minutes(60)

#define SOPHIATX_INIT_SUPPLY                     int64_t(350000000000000)
#define SOPHIATX_TOTAL_SUPPLY                    int64_t(500000000000000)

#define SOPHIATX_HARDFORK_REQUIRED_WITNESSES     31 // 31 of the 51 dpos witnesses required for hardfork. This guarantees 75% participation on all subsequent rounds.

#endif

#define SOPHIATX_ADDRESS_PREFIX                  "SPH"

#define SOPHIATX_GENESIS_TIME                    (fc::time_point_sec(1531512800))
#define SOPHIATX_BLOCK_INTERVAL                  3

#define SOPHIATX_DECIMALS (6)
#define SOPHIATX_SATOSHIS uint64_t(1000000)
#define SOPHIATX_PROMOTION_POOL_PERCENTAGE (1000)
#define SOPHIATX_MINING_POOL_PERCENTAGE (2500)
#define SOPHIATX_INTEREST_POOL_PERCENTAGE (6500)
#define SOPHIATX_BURN_FEE_PERCENTAGE (1000)
#define SOPHIATX_INTEREST_BLOCKS (2400)
#define SOPHIATX_COINBASE_YEARS (25)

#define SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE uint64_t( SOPHIATX_SATOSHIS * 250000 )
#define SOPHIATX_FINAL_WITNESS_REQUIRED_VESTING_BALANCE uint64_t( SOPHIATX_SATOSHIS * 300000 )
#define SOPHIATX_WITNESS_VESTING_INCREASE_DAYS 96 //January 1
#define SOPHIATX_WITNESS_VESTING_INCREASE_DAYS_HF_1_1 159 //January 1 - take into account missing blocks

#define VESTS_SYMBOL_U64  (uint64_t('V') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define SOPHIATX_SYMBOL_U64  (uint64_t('S') | (uint64_t('P') << 8) | (uint64_t('H') << 16) | (uint64_t('T') << 24) | (uint64_t('X') << 32))
#define SBD1_SYMBOL_U64    (uint64_t('U') | (uint64_t('S') << 8) | (uint64_t('D') << 16) )
#define SBD2_SYMBOL_U64    (uint64_t('E') | (uint64_t('U') << 8) | (uint64_t('R') << 16) )
#define SBD3_SYMBOL_U64    (uint64_t('C') | (uint64_t('H') << 8) | (uint64_t('F') << 16) )
#define SBD4_SYMBOL_U64    (uint64_t('C') | (uint64_t('N') << 8) | (uint64_t('Y') << 16) )
#define SBD5_SYMBOL_U64    (uint64_t('G') | (uint64_t('B') << 8) | (uint64_t('P') << 16) )


#define VESTS_SYMBOL_SER  (VESTS_SYMBOL_U64) ///< VESTS|VESTS with 6 digits of precision
#define SOPHIATX_SYMBOL_SER  (SOPHIATX_SYMBOL_U64) ///< SPHTX|TESTS with 6 digits of precision
#define SBD1_SYMBOL_SER    (SBD1_SYMBOL_U64)
#define SBD2_SYMBOL_SER    (SBD2_SYMBOL_U64)
#define SBD3_SYMBOL_SER    (SBD3_SYMBOL_U64)
#define SBD4_SYMBOL_SER    (SBD4_SYMBOL_U64)
#define SBD5_SYMBOL_SER    (SBD5_SYMBOL_U64)

#define VESTS_SYMBOL  ( sophiatx::protocol::asset_symbol_type( VESTS_SYMBOL_SER ) )
#define SOPHIATX_SYMBOL  ( sophiatx::protocol::asset_symbol_type( SOPHIATX_SYMBOL_SER ) )
#define SBD1_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD1_SYMBOL_SER ) ) //USD
#define SBD2_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD2_SYMBOL_SER ) ) //EUR
#define SBD3_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD3_SYMBOL_SER ) ) //CHF
#define SBD4_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD4_SYMBOL_SER ) ) //CNY
#define SBD5_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD5_SYMBOL_SER ) ) //GBP

#define BASE_FEE       (asset(10000, SOPHIATX_SYMBOL))
#define BASE_FEE_SBD1  (asset(10000, SBD1_SYMBOL)) //USD
#define BASE_FEE_SBD2  (asset(8000,  SBD2_SYMBOL)) //EUR
#define BASE_FEE_SBD3  (asset(10000, SBD3_SYMBOL)) //CHF
#define BASE_FEE_SBD4  (asset(64000, SBD4_SYMBOL)) //CNY
#define BASE_FEE_SBD5  (asset(7500,  SBD5_SYMBOL)) //GBP

#define SIZE_COVERED_IN_BASE_FEE 4096
#define SIZE_INCREASE_PER_FEE 4096

#define SOPHIATX_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( SOPHIATX_BLOCKCHAIN_VERSION ) )

#define SOPHIATX_INIT_MINER_NAME                 "initminer"
#define SOPHIATX_NUM_INIT_MINERS                 1

#define SOPHIATX_MAX_WITNESSES                   51

#define SOPHIATX_MAX_VOTED_WITNESSES_HF0         47
#define SOPHIATX_MAX_RUNNER_WITNESSES_HF0        4

#define SOPHIATX_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define SOPHIATX_MAX_MEMO_SIZE                   2048
#define SOPHIATX_MAX_NAME_SEED_SIZE              32
#define SOPHIATX_MAX_PROXY_RECURSION_DEPTH       4
#define SOPHIATX_VESTING_WITHDRAW_INTERVALS      27
#define SOPHIATX_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24) /// 1 day per interval

#define SOPHIATX_MAX_ACCOUNT_WITNESS_VOTES       60

#define SOPHIATX_100_PERCENT                     10000
#define SOPHIATX_1_PERCENT                       (SOPHIATX_100_PERCENT/100)

// bandwidth and total operations num counters are reset to zero every SOPHIATX_LIMIT_BANDWIDTH_BLOCKS
#define SOPHIATX_LIMIT_BANDWIDTH_BLOCKS          51      // one round (153 seconds)
// max allowed fee-free operations bandwidth [Bytes] per account
#define SOPHIATX_MAX_ALLOWED_BANDWIDTH           51000   // [Bytes]
// max allowed fee-free operations count per account
#define SOPHIATX_MAX_ALLOWED_OPS_COUNT           102     // [count]


#define SOPHIATX_MIN_ACCOUNT_NAME_LENGTH          3
#define SOPHIATX_MAX_ACCOUNT_NAME_LENGTH         16

#define SOPHIATX_MAX_PERMLINK_LENGTH             256
#define SOPHIATX_MAX_WITNESS_URL_LENGTH          2048

#define SOPHIATX_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)
#define SOPHIATX_MAX_SATOSHIS                    int64_t(4611686018427387903ll)
#define SOPHIATX_MAX_SIG_CHECK_DEPTH             2

#define SOPHIATX_MIN_TRANSACTION_SIZE_LIMIT      1024

#define SOPHIATX_MAX_TRANSACTION_SIZE            (1024*8)
#define SOPHIATX_MIN_BLOCK_SIZE                  115
#define SOPHIATX_FEED_HISTORY_WINDOW             (12*7) // 3.5 days
#define SOPHIATX_MAX_FEED_AGE_SECONDS            (60*60*24*7) // 7 days

#define SOPHIATX_MAX_UNDO_HISTORY                10000

#define SOPHIATX_IRREVERSIBLE_THRESHOLD          (75 * SOPHIATX_1_PERCENT)

#define SOPHIATX_VIRTUAL_SCHEDULE_LAP_LENGTH  ( fc::uint128(uint64_t(-1)) )
#define SOPHIATX_VIRTUAL_SCHEDULE_LAP_LENGTH2 ( fc::uint128::max_value() )

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the current witnesses
#define SOPHIATX_MINER_ACCOUNT                   "miners"
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define SOPHIATX_NULL_ACCOUNT                    "none"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define SOPHIATX_TEMP_ACCOUNT                    "temp"
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define SOPHIATX_PROXY_TO_SELF_ACCOUNT           ""
///@}
#define SOPHIATX_API_SINGLE_QUERY_LIMIT           1000


/////////////////////////////////////////////////////////////////////////

namespace sophiatx { namespace protocol {

class protocol_config {
public:
    inline static void init(const fc::mutable_variant_object& conf)
    {
        instance().config_loaded_ = true;
        try {
            instance().config_["SOPHIATX_MIN_BLOCK_SIZE_LIMIT"] = conf["SOPHIATX_MAX_TRANSACTION_SIZE"] * 16;
        } catch (...) {
            instance().config_["SOPHIATX_MIN_BLOCK_SIZE_LIMIT"] = SOPHIATX_MAX_TRANSACTION_SIZE * 16;
        }

        try {
            instance().config_["SOPHIATX_BLOCK_INTERVAL"] = conf["SOPHIATX_BLOCK_INTERVAL"];
        } catch (...) {
            instance().config_["SOPHIATX_BLOCK_INTERVAL"] = SOPHIATX_BLOCK_INTERVAL;
        }

        try {
            instance().config_["SOPHIATX_MAX_BLOCK_SIZE"] = conf["SOPHIATX_MAX_BLOCK_SIZE"];
        } catch (...) {
            instance().config_["SOPHIATX_MAX_BLOCK_SIZE"] = SOPHIATX_MAX_TRANSACTION_SIZE * SOPHIATX_BLOCK_INTERVAL * 2048;
        }


    }

    template<typename T>
    inline static T get( std::string_view index ) {
        FC_ASSERT(instance().config_loaded_, "protocol_config is not initialized!");
        T type;
        fc::from_variant(instance().config_[index], type);
        return type;
    }

private:
    protocol_config() : config_loaded_(false) {}
    ~protocol_config() {}

    inline static protocol_config &instance() {
        static protocol_config instance;
        return instance;
    }

    bool config_loaded_;
    fc::mutable_variant_object config_;
};

} } // sophiatx::protocol

///////////////////////////////////////////////////////////////////////////////
