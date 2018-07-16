/*
 * Copyright (c) 2016 Steemit, Inc., Equidato Technologies and contributors.
 */
#pragma once
#include <sophiatx/protocol/hardfork.hpp>

// WARNING!
// Every symbol defined here needs to be handled appropriately in get_config.cpp
// This is checked by get_config_check.sh called from Dockerfile

#ifdef IS_TEST_NET
#define SOPHIATX_BLOCKCHAIN_VERSION              ( version(0, 0, 0) )

#define SOPHIATX_INIT_PRIVATE_KEY                (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
#define SOPHIATX_INIT_PUBLIC_KEY_STR             (std::string( sophiatx::protocol::public_key_type(SOPHIATX_INIT_PRIVATE_KEY.get_public_key()) ))
#define SOPHIATX_CHAIN_ID_NAME "testnet"
#define SOPHIATX_CHAIN_ID (fc::sha256::hash(SOPHIATX_CHAIN_ID_NAME))
#define SOPHIATX_ADDRESS_PREFIX                  "TST"

#define SOPHIATX_GENESIS_TIME                    (fc::time_point_sec(1451606400))
#define SOPHIATX_MINING_TIME                     (fc::time_point_sec(1451606400))
#define SOPHIATX_CASHOUT_WINDOW_SECONDS          (60*60) /// 1 hr
#define SOPHIATX_SECOND_CASHOUT_WINDOW           (60*60*24*3) /// 3 days
#define SOPHIATX_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24) /// 1 day
#define SOPHIATX_UPVOTE_LOCKOUT_HF7              (fc::minutes(1))
#define SOPHIATX_UPVOTE_LOCKOUT_HF17             (fc::minutes(5))


#define SOPHIATX_MIN_ACCOUNT_CREATION_FEE          0

#define SOPHIATX_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define SOPHIATX_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define SOPHIATX_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define SOPHIATX_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1

#define SOPHIATX_INIT_SUPPLY                     (int64_t( 250 ) * int64_t( 1000000 ) * int64_t( 1000 ))

/// Allows to limit number of total produced blocks.
#define TESTNET_BLOCK_LIMIT                   (3000000)

#else // IS LIVE SOPHIATX NETWORK

#define SOPHIATX_BLOCKCHAIN_VERSION              ( version(0, 0, 0) )

#define SOPHIATX_INIT_PUBLIC_KEY_STR             "SPH8Xg6cEbqPCY8jrWFccgbCq5Fjw1okivwwmLDDgqQCQeAk7jedu" //5JPwY3bwFgfsGtxMeLkLqXzUrQDMAsqSyAZDnMBkg7PDDRhQgaV
#define SOPHIATX_ADDRESS_PREFIX                  "SPH"

#define SOPHIATX_GENESIS_TIME                    (fc::time_point_sec(1524563000))
#//define SOPHIATX_MINING_TIME                     (fc::time_point_sec(1524563200))
#define SOPHIATX_CASHOUT_WINDOW_SECONDS          (60*60*24*7)  /// 7 days
#define SOPHIATX_SECOND_CASHOUT_WINDOW           (60*60*24*30) /// 30 days
#define SOPHIATX_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24*14) /// 2 weeks
#define SOPHIATX_UPVOTE_LOCKOUT_HF7              (fc::minutes(1))
#define SOPHIATX_UPVOTE_LOCKOUT_HF17             (fc::hours(12))

#define SOPHIATX_MIN_ACCOUNT_CREATION_FEE           100000

#define SOPHIATX_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define SOPHIATX_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define SOPHIATX_OWNER_UPDATE_LIMIT                          fc::minutes(60)

#define SOPHIATX_INIT_SUPPLY                     int64_t(350000000000000)
#define SOPHIATX_TOTAL_SUPPLY                    int64_t(500000000000000)

#endif

#define SOPHIATX_BLOCK_INTERVAL                  3
#define SOPHIATX_BLOCKS_PER_YEAR                 (365*24*60*60/SOPHIATX_BLOCK_INTERVAL)
#define SOPHIATX_BLOCKS_PER_DAY                  (24*60*60/SOPHIATX_BLOCK_INTERVAL)
#define SOPHIATX_START_VESTING_BLOCK             ( 0 )
#define SOPHIATX_START_MINER_VOTING_BLOCK        (SOPHIATX_BLOCKS_PER_DAY * 7)

#define SOPHIATX_DECIMALS (6)
#define SOPHIATX_SATOSHIS uint64_t(1000000)
#define SOPHIATX_PROMOTION_POOL_PERCENTAGE (1000)
#define SOPHIATX_MINING_POOL_PERCENTAGE (2500)
#define SOPHIATX_INTEREST_POOL_PERCENTAGE (6500)
#define SOPHIATX_INTEREST_BLOCKS (2400)
#define SOPHIATX_INTEREST_FEES_TIME ( SOPHIATX_BLOCKS_PER_DAY )
#define SOPHIATX_INTEREST_DELAY (SOPHIATX_BLOCKS_PER_DAY)
#define SOPHIATX_COINBASE_YEARS (25)
#define SOPHIATX_COINBASE_BLOCKS ( SOPHIATX_BLOCKS_PER_YEAR * SOPHIATX_COINBASE_YEARS )
#ifdef PRIVATE_NET
#define SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE uint64_t( 0 )
#else
#define SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE uint64_t( SOPHIATX_SATOSHIS * 250000 )
#define SOPHIATX_FINAL_WITNESS_REQUIRED_VESTING_BALANCE uint64_t( SOPHIATX_SATOSHIS * 300000 )
#define SOPHIATX_WITNESS_VESTING_INCREASE_DAYS 96 //January 1
#endif //PRIVATE_NET

#define VESTS_SYMBOL  ( sophiatx::protocol::asset_symbol_type( VESTS_SYMBOL_SER ) )
#define SOPHIATX_SYMBOL  ( sophiatx::protocol::asset_symbol_type( SOPHIATX_SYMBOL_SER ) )
#define SBD1_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD1_SYMBOL_SER ) ) //USD
#define SBD2_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD2_SYMBOL_SER ) ) //EUR
#define SBD3_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD3_SYMBOL_SER ) ) //CHF
#define SBD4_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD4_SYMBOL_SER ) ) //CNY
#define SBD5_SYMBOL    ( sophiatx::protocol::asset_symbol_type( SBD5_SYMBOL_SER ) ) //GBP

#define BASE_FEE       (asset(100000, SOPHIATX_SYMBOL))
#define BASE_FEE_SBD1  (asset(100000, SBD1_SYMBOL))
#define BASE_FEE_SBD2  (asset(80000, SBD2_SYMBOL))
#define BASE_FEE_SBD3  (asset(100000, SBD3_SYMBOL))
#define BASE_FEE_SBD4  (asset(640000, SBD4_SYMBOL))
#define BASE_FEE_SBD5  (asset(75000, SBD5_SYMBOL))

#define SIZE_COVERED_IN_BASE_FEE 1024
#define SIZE_INCREASE_PER_FEE 2048

#define SOPHIATX_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( SOPHIATX_BLOCKCHAIN_VERSION ) )



#define SOPHIATX_INIT_MINER_NAME                 "initminer"
#define SOPHIATX_NUM_INIT_MINERS                 1
#define SOPHIATX_INIT_TIME                       (fc::time_point_sec());

#define SOPHIATX_MAX_WITNESSES                   51

#define SOPHIATX_MAX_VOTED_WITNESSES_HF0         47
#define SOPHIATX_MAX_MINER_WITNESSES_HF0         0
#define SOPHIATX_MAX_RUNNER_WITNESSES_HF0        4



#define SOPHIATX_HARDFORK_REQUIRED_WITNESSES     31 // 31 of the 51 dpos witnesses required for hardfork. This guarantees 75% participation on all subsequent rounds.
#define SOPHIATX_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define SOPHIATX_MAX_MEMO_SIZE                   2048
#define SOPHIATX_MAX_PROXY_RECURSION_DEPTH       4
#define SOPHIATX_VESTING_WITHDRAW_INTERVALS      27
#define SOPHIATX_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24) /// 1 day per interval

#define SOPHIATX_VOTE_REGENERATION_SECONDS       (5*60*60*24) // 5 day
#define SOPHIATX_MAX_VOTE_CHANGES                5
#define SOPHIATX_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
#define SOPHIATX_MIN_VOTE_INTERVAL_SEC           3
#define SOPHIATX_VOTE_DUST_THRESHOLD             (50000000)

#define SOPHIATX_MIN_REPLY_INTERVAL              (fc::seconds(20)) // 20 seconds
#define SOPHIATX_POST_AVERAGE_WINDOW             (60*60*24u) // 1 day
#define SOPHIATX_POST_WEIGHT_CONSTANT            (uint64_t(4*SOPHIATX_100_PERCENT) * (4*SOPHIATX_100_PERCENT))// (4*SOPHIATX_100_PERCENT) -> 2 posts per 1 days, average 1 every 12 hours

#define SOPHIATX_MAX_ACCOUNT_WITNESS_VOTES       60

#define SOPHIATX_100_PERCENT                     10000
#define SOPHIATX_1_PERCENT                       (SOPHIATX_100_PERCENT/100)

#define SOPHIATX_MINER_PAY_PERCENT               (SOPHIATX_1_PERCENT) // 1%
#define SOPHIATX_MAX_RATION_DECAY_RATE           (1000000)

#define SOPHIATX_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) ///< 1 week
#define SOPHIATX_BANDWIDTH_PRECISION             (uint64_t(1000000)) ///< 1 million

#define SOPHIATX_MAX_RESERVE_RATIO               (20000)

#define SOPHIATX_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define SOPHIATX_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)

#define SOPHIATX_MINING_REWARD                   asset( 1000, SOPHIATX_SYMBOL )


#define SOPHIATX_ACTIVE_CHALLENGE_FEE            asset( 2000, SOPHIATX_SYMBOL )
#define SOPHIATX_OWNER_CHALLENGE_FEE             asset( 30000, SOPHIATX_SYMBOL )
#define SOPHIATX_ACTIVE_CHALLENGE_COOLDOWN       fc::days(1)
#define SOPHIATX_OWNER_CHALLENGE_COOLDOWN        fc::days(1)

// note, if redefining these constants make sure calculate_claims doesn't overflow

// 5ccc e802 de5f
// int(expm1( log1p( 1 ) / BLOCKS_PER_YEAR ) * 2**SOPHIATX_APR_PERCENT_SHIFT_PER_BLOCK / 100000 + 0.5)
// we use 100000 here instead of 10000 because we end up creating an additional 9x for vesting
#define SOPHIATX_APR_PERCENT_MULTIPLY_PER_BLOCK          ( (uint64_t( 0x5ccc ) << 0x20) \
                                                        | (uint64_t( 0xe802 ) << 0x10) \
                                                        | (uint64_t( 0xde5f )        ) \
                                                        )
// chosen to be the maximal value such that SOPHIATX_APR_PERCENT_MULTIPLY_PER_BLOCK * 2**64 * 100000 < 2**128
#define SOPHIATX_APR_PERCENT_SHIFT_PER_BLOCK             87

#define SOPHIATX_APR_PERCENT_MULTIPLY_PER_ROUND          ( (uint64_t( 0x79cc ) << 0x20 ) \
                                                        | (uint64_t( 0xf5c7 ) << 0x10 ) \
                                                        | (uint64_t( 0x3480 )         ) \
                                                        )

#define SOPHIATX_APR_PERCENT_SHIFT_PER_ROUND             83

// We have different constants for hourly rewards
// i.e. hex(int(math.expm1( math.log1p( 1 ) / HOURS_PER_YEAR ) * 2**SOPHIATX_APR_PERCENT_SHIFT_PER_HOUR / 100000 + 0.5))
#define SOPHIATX_APR_PERCENT_MULTIPLY_PER_HOUR           ( (uint64_t( 0x6cc1 ) << 0x20) \
                                                        | (uint64_t( 0x39a1 ) << 0x10) \
                                                        | (uint64_t( 0x5cbd )        ) \
                                                        )

// chosen to be the maximal value such that SOPHIATX_APR_PERCENT_MULTIPLY_PER_HOUR * 2**64 * 100000 < 2**128
#define SOPHIATX_APR_PERCENT_SHIFT_PER_HOUR              77

#define SOPHIATX_MIN_ACCOUNT_NAME_LENGTH          3
#define SOPHIATX_MAX_ACCOUNT_NAME_LENGTH         16

#define SOPHIATX_MIN_PERMLINK_LENGTH             0
#define SOPHIATX_MAX_PERMLINK_LENGTH             256
#define SOPHIATX_MAX_WITNESS_URL_LENGTH          2048

#define SOPHIATX_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)
#define SOPHIATX_MAX_SATOSHIS                    int64_t(4611686018427387903ll)
#define SOPHIATX_MAX_SIG_CHECK_DEPTH             2

#define SOPHIATX_MIN_TRANSACTION_SIZE_LIMIT      1024
#define SOPHIATX_SECONDS_PER_YEAR                (uint64_t(60*60*24*365ll))

#define SOPHIATX_MAX_TRANSACTION_SIZE            (1024*64)
#define SOPHIATX_MIN_BLOCK_SIZE_LIMIT            (SOPHIATX_MAX_TRANSACTION_SIZE)
#define SOPHIATX_MAX_BLOCK_SIZE                  (SOPHIATX_MAX_TRANSACTION_SIZE * SOPHIATX_BLOCK_INTERVAL*2000)
#define SOPHIATX_SOFT_MAX_BLOCK_SIZE             (20*1024*1024)
#define SOPHIATX_MIN_BLOCK_SIZE                  115
#define SOPHIATX_BLOCKS_PER_HOUR                 (60*60/SOPHIATX_BLOCK_INTERVAL)
#define SOPHIATX_FEED_INTERVAL_BLOCKS            (SOPHIATX_BLOCKS_PER_HOUR)
#define SOPHIATX_FEED_HISTORY_WINDOW             (12*7) // 3.5 days
#define SOPHIATX_MAX_FEED_AGE_SECONDS            (60*60*24*7) // 7 days
#define SOPHIATX_MIN_FEEDS                       1 //(SOPHIATX_MAX_WITNESSES/10) /// protects the network from conversions before price has been established

#define SOPHIATX_MIN_UNDO_HISTORY                10
#define SOPHIATX_MAX_UNDO_HISTORY                10000

#define SOPHIATX_MIN_TRANSACTION_EXPIRATION_LIMIT (SOPHIATX_BLOCK_INTERVAL * 5) // 5 transactions per block
#define SOPHIATX_BLOCKCHAIN_PRECISION            uint64_t( 1000 )

#define SOPHIATX_BLOCKCHAIN_PRECISION_DIGITS     3
#define SOPHIATX_MAX_INSTANCE_ID                 (uint64_t(-1)>>16)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define SOPHIATX_MAX_AUTHORITY_MEMBERSHIP        10
#define SOPHIATX_MAX_ASSET_WHITELIST_AUTHORITIES 10
#define SOPHIATX_MAX_URL_LENGTH                  127

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
/// Represents the canonical root post parent account
#define SOPHIATX_ROOT_POST_PARENT                (account_name_type())
///@}

#ifdef SOPHIATX_ENABLE_SMT

#define SMT_MAX_VOTABLE_ASSETS 2
#define SMT_VESTING_WITHDRAW_INTERVAL_SECONDS   (60*60*24*7) /// 1 week per interval
#define SMT_UPVOTE_LOCKOUT                      (60*60*12)  /// 12 hours

#endif /// SOPHIATX_ENABLE_SMT

