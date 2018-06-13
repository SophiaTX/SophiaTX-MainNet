#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/steem_objects.hpp>


#include <steem/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
#define DUMP( x ) {fc::variant vo; fc::to_variant( x , vo); std::cout<< fc::json::to_string(vo) <<"\n";}

BOOST_FIXTURE_TEST_SUITE( operation_time_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( vesting_withdrawals )
{
   try
   {
      ACTORS( (alice) )
      fund( "alice", 100000000 );
      vest( "alice", 100000000 );

      const auto& new_alice = db->get_account( "alice" );

      BOOST_TEST_MESSAGE( "Setting up withdrawal" );

      signed_transaction tx;
      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( new_alice.vesting_shares.amount / 2, VESTS_SYMBOL );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto next_withdrawal = db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS;
      asset vesting_shares = new_alice.vesting_shares;
      asset to_withdraw = op.vesting_shares;
      asset original_vesting = vesting_shares;
      asset withdraw_rate = new_alice.vesting_withdraw_rate;

      BOOST_TEST_MESSAGE( "Generating block up to first withdrawal" );
      generate_blocks( next_withdrawal - ( STEEM_BLOCK_INTERVAL / 2 ), true);

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == vesting_shares.amount.value );

      BOOST_TEST_MESSAGE( "Generating block to cause withdrawal" );
      generate_block();

      auto fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
      auto gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
      BOOST_REQUIRE( ( withdraw_rate ).amount.value - db->get_account( "alice" ).balance.amount.value <= 1 ); // Check a range due to differences in the share price
      BOOST_REQUIRE( fill_op.from_account == "alice" );
      BOOST_REQUIRE( fill_op.to_account == "alice" );
      BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
      BOOST_REQUIRE( std::abs( fill_op.deposited.amount.value - fill_op.withdrawn.amount.value ) <= 1 );
      validate_database();

      BOOST_TEST_MESSAGE( "Generating the rest of the blocks in the withdrawal" );

      vesting_shares = db->get_account( "alice" ).vesting_shares;
      auto balance = db->get_account( "alice" ).balance;
      auto old_next_vesting = db->get_account( "alice" ).next_vesting_withdrawal;

      for( int i = 1; i < STEEM_VESTING_WITHDRAW_INTERVALS - 1; i++ )
      {
         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );

         const auto& alice = db->get_account( "alice" );

         gpo = db->get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE( alice.vesting_shares.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
         BOOST_REQUIRE( balance.amount.value + ( withdraw_rate ).amount.value - alice.balance.amount.value <= 1 );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( fill_op.deposited.amount.value - fill_op.withdrawn.amount.value ) <= 1 );

         if ( i == STEEM_VESTING_WITHDRAW_INTERVALS - 1 )
            BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );
         else
            BOOST_REQUIRE( alice.next_vesting_withdrawal.sec_since_epoch() == ( old_next_vesting + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );

         validate_database();

         vesting_shares = alice.vesting_shares;
         balance = alice.balance;
         old_next_vesting = alice.next_vesting_withdrawal;
      }

      if (  to_withdraw.amount.value % withdraw_rate.amount.value != 0 )
      {
         BOOST_TEST_MESSAGE( "Generating one more block to take care of remainder" );
         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         gpo = db->get_dynamic_global_properties();

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == ( old_next_vesting + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( fill_op.deposited.amount.value - fill_op.withdrawn.amount.value ) <= 1 );

         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         gpo = db->get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == fc::time_point_sec::maximum().sec_since_epoch() );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == to_withdraw.amount.value % withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( fill_op.deposited.amount.value - fill_op.withdrawn.amount.value ) <= 1 );

         validate_database();
      }
      else
      {
         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == fc::time_point_sec::maximum().sec_since_epoch() );

         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( fill_op.deposited.amount.value - fill_op.withdrawn.amount.value ) <= 1 );
      }

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == ( original_vesting - op.vesting_shares ).amount.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( feed_publish_mean )
{
   try
   {
      resize_shared_mem( 1024 * 1024 * 128 );

      ACTORS( (alice0)(alice1)(alice2)(alice3)(alice4)(alice5)(alice6) )

      BOOST_TEST_MESSAGE( "Setup" );

      generate_blocks( 30 / STEEM_BLOCK_INTERVAL );

      vector< string > accounts;
      accounts.push_back( "alice0" );
      accounts.push_back( "alice1" );
      accounts.push_back( "alice2" );
      accounts.push_back( "alice3" );
      accounts.push_back( "alice4" );
      accounts.push_back( "alice5" );
      accounts.push_back( "alice6" );

      vector< private_key_type > keys;
      keys.push_back( alice0_private_key );
      keys.push_back( alice1_private_key );
      keys.push_back( alice2_private_key );
      keys.push_back( alice3_private_key );
      keys.push_back( alice4_private_key );
      keys.push_back( alice5_private_key );
      keys.push_back( alice6_private_key );

      vector< feed_publish_operation > ops;
      vector< signed_transaction > txs;

      // Upgrade accounts to witnesses
      for( int i = 0; i < 7; i++ )
      {
         transfer( STEEM_INIT_MINER_NAME, accounts[i], asset( SOPHIATX_WITNESS_REQUIRED_VESTING_BALANCE, STEEM_SYMBOL ) );
         vest( accounts[i], SOPHIATX_WITNESS_REQUIRED_VESTING_BALANCE);
         witness_create( accounts[i], keys[i], "foo.bar", keys[i].get_public_key(), 0 );


         ops.push_back( feed_publish_operation() );
         ops[i].publisher = accounts[i];

         txs.push_back( signed_transaction() );
      }

      ops[0].exchange_rate = price( asset( 1000, SBD1_SYMBOL ), asset( 100000, STEEM_SYMBOL ) );
      ops[1].exchange_rate = price( asset( 1000, SBD1_SYMBOL ), asset( 105000, STEEM_SYMBOL ) );
      ops[2].exchange_rate = price( asset( 1000, SBD1_SYMBOL ), asset(  98000, STEEM_SYMBOL ) );
      ops[3].exchange_rate = price( asset( 1000, SBD1_SYMBOL ), asset(  97000, STEEM_SYMBOL ) );
      ops[4].exchange_rate = price( asset( 1000, SBD1_SYMBOL ), asset(  99000, STEEM_SYMBOL ) );
      ops[5].exchange_rate = price( asset( 1000, SBD1_SYMBOL ), asset(  97500, STEEM_SYMBOL ) );
      ops[6].exchange_rate = price( asset( 1000, SBD1_SYMBOL ), asset( 102000, STEEM_SYMBOL ) );

      for( int i = 0; i < 7; i++ )
      {
         txs[i].set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         txs[i].operations.push_back( ops[i] );
         txs[i].sign( keys[i], db->get_chain_id() );
         db->push_transaction( txs[i], 0 );
      }

      BOOST_TEST_MESSAGE( "Jump forward an hour" );

      generate_blocks( STEEM_BLOCKS_PER_HOUR ); // Jump forward 1 hour
      BOOST_TEST_MESSAGE( "Get feed history object" );
      feed_history_object feed_history = db->get_feed_history(SBD1_SYMBOL);
      BOOST_TEST_MESSAGE( "Check state" );
      BOOST_REQUIRE( feed_history.current_median_history == price( asset( 1000, SBD1_SYMBOL ), asset( 99000, STEEM_SYMBOL) ) );
      BOOST_REQUIRE( feed_history.price_history[ 0 ] == price( asset( 1000, SBD1_SYMBOL ), asset( 99000, STEEM_SYMBOL) ) );
      validate_database();

      for ( int i = 0; i < 23; i++ )
      {
         BOOST_TEST_MESSAGE( "Updating ops" );

         for( int j = 0; j < 7; j++ )
         {
            txs[j].operations.clear();
            txs[j].signatures.clear();
            ops[j].exchange_rate = price( ops[j].exchange_rate.base, asset( ops[j].exchange_rate.quote.amount + 10, STEEM_SYMBOL ) );
            txs[j].set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
            txs[j].operations.push_back( ops[j] );
            txs[j].sign( keys[j], db->get_chain_id() );
            db->push_transaction( txs[j], 0 );
         }

         BOOST_TEST_MESSAGE( "Generate Blocks" );

         generate_blocks( STEEM_BLOCKS_PER_HOUR  ); // Jump forward 1 hour

         BOOST_TEST_MESSAGE( "Check feed_history" );

         feed_history = db->get_feed_history(SBD1_SYMBOL);
         BOOST_REQUIRE( feed_history.current_median_history == ops[4].exchange_rate );
         //BOOST_REQUIRE( feed_history.price_history[ i + 1 ] == ops[4].exchange_rate );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( interests )
{
   try{
      ACTORS( (alice)(bob) )
      generate_block();
      fund("alice", 100000000);
      fund("bob",  1000000000);

      generate_blocks( SOPHIATX_INTEREST_BLOCKS );
      generate_blocks( SOPHIATX_INTEREST_BLOCKS );
      uint64_t expected_interest = 3 * SOPHIATX_INTEREST_BLOCKS * 1000 * 65 / (SOPHIATX_COINBASE_BLOCKS / 10000) / 7;



      auto interest_op = get_last_operations( 1, "bob" )[0].get< interest_operation >();
      BOOST_REQUIRE( interest_op.owner == "bob" );
      BOOST_REQUIRE( interest_op.interest.amount.value == expected_interest );
      //DUMP(db->get_account( "alice" ).balance.amount);
      BOOST_REQUIRE( db->get_account( "alice" ).balance.amount.value >= 100000000 + expected_interest/10  && db->get_account( "alice" ).balance.amount.value <= 100000000 + 2*expected_interest/10);
      validate_database();

   }FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( sbd_interest )
{
   //TODO_SOPHIA rework
   /*try
   {
      ACTORS( (alice)(bob) )
      generate_block();
      vest( "alice", ASSET( "10.000 TESTS" ) );
      vest( "bob", ASSET( "10.000 TESTS" ) );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      BOOST_TEST_MESSAGE( "Testing interest over smallest interest period" );

      convert_operation op;
      signed_transaction tx;

      fund( "alice", ASSET( "31.903 TBD" ) );

      auto start_time = db->get_account( "alice" ).sbd_seconds_last_update;
      auto alice_sbd = db->get_account( "alice" ).sbd_balance;

      generate_blocks( db->head_block_time() + fc::seconds( STEEM_SBD_INTEREST_COMPOUND_INTERVAL_SEC ), true );

      transfer_operation transfer;
      transfer.to = "bob";
      transfer.from = "alice";
      transfer.amount = ASSET( "1.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( transfer );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto gpo = db->get_dynamic_global_properties();
      auto interest_op = get_last_operations( 1 )[0].get< interest_operation >();

      BOOST_REQUIRE( gpo.sbd_interest_rate > 0 );
      BOOST_REQUIRE( static_cast<uint64_t>(db->get_account( "alice" ).sbd_balance.amount.value) == alice_sbd.amount.value - ASSET( "1.000 TBD" ).amount.value + ( ( ( ( uint128_t( alice_sbd.amount.value ) * ( db->head_block_time() - start_time ).to_seconds() ) / STEEM_SECONDS_PER_YEAR ) * gpo.sbd_interest_rate ) / STEEM_100_PERCENT ).to_uint64() );
      BOOST_REQUIRE( interest_op.owner == "alice" );
      BOOST_REQUIRE( interest_op.interest.amount.value == db->get_account( "alice" ).sbd_balance.amount.value - ( alice_sbd.amount.value - ASSET( "1.000 TBD" ).amount.value ) );
      validate_database();

      BOOST_TEST_MESSAGE( "Testing interest under interest period" );

      start_time = db->get_account( "alice" ).sbd_seconds_last_update;
      alice_sbd = db->get_account( "alice" ).sbd_balance;

      generate_blocks( db->head_block_time() + fc::seconds( STEEM_SBD_INTEREST_COMPOUND_INTERVAL_SEC / 2 ), true );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).sbd_balance.amount.value == alice_sbd.amount.value - ASSET( "1.000 TBD" ).amount.value );
      validate_database();

      auto alice_coindays = uint128_t( alice_sbd.amount.value ) * ( db->head_block_time() - start_time ).to_seconds();
      alice_sbd = db->get_account( "alice" ).sbd_balance;
      start_time = db->get_account( "alice" ).sbd_seconds_last_update;

      BOOST_TEST_MESSAGE( "Testing longer interest period" );

      generate_blocks( db->head_block_time() + fc::seconds( ( STEEM_SBD_INTEREST_COMPOUND_INTERVAL_SEC * 7 ) / 3 ), true );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( static_cast<uint64_t>(db->get_account( "alice" ).sbd_balance.amount.value) == alice_sbd.amount.value - ASSET( "1.000 TBD" ).amount.value + ( ( ( ( uint128_t( alice_sbd.amount.value ) * ( db->head_block_time() - start_time ).to_seconds() + alice_coindays ) / STEEM_SECONDS_PER_YEAR ) * gpo.sbd_interest_rate ) / STEEM_100_PERCENT ).to_uint64() );
      validate_database();
   }
   FC_LOG_AND_RETHROW();*/
}


BOOST_AUTO_TEST_CASE( clear_null_account )
{
   /*try
   {
      BOOST_TEST_MESSAGE( "Testing clearing the null account's balances on block" );

      ACTORS( (alice) );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      fund( "alice", ASSET( "10.000 TESTS" ) );
      fund( "alice", ASSET( "10.000 TBD" ) );

      transfer_operation transfer1;
      transfer1.from = "alice";
      transfer1.to = STEEM_NULL_ACCOUNT;
      transfer1.amount = ASSET( "1.000 TESTS" );

      transfer_operation transfer2;
      transfer2.from = "alice";
      transfer2.to = STEEM_NULL_ACCOUNT;
      transfer2.amount = ASSET( "2.000 TBD" );

      transfer_to_vesting_operation vest;
      vest.from = "alice";
      vest.to = STEEM_NULL_ACCOUNT;
      vest.amount = ASSET( "3.000 TESTS" );

      transfer_to_savings_operation save1;
      save1.from = "alice";
      save1.to = STEEM_NULL_ACCOUNT;
      save1.amount = ASSET( "4.000 TESTS" );

      transfer_to_savings_operation save2;
      save2.from = "alice";
      save2.to = STEEM_NULL_ACCOUNT;
      save2.amount = ASSET( "5.000 TBD" );

      BOOST_TEST_MESSAGE( "--- Transferring to NULL Account" );

      signed_transaction tx;
      tx.operations.push_back( transfer1 );
      tx.operations.push_back( transfer2 );
      tx.operations.push_back( vest );
      tx.operations.push_back( save1);
      tx.operations.push_back( save2 );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( STEEM_NULL_ACCOUNT ), [&]( account_object& a )
         {
            a.reward_steem_balance = ASSET( "1.000 TESTS" );
            a.reward_sbd_balance = ASSET( "1.000 TBD" );
            a.reward_vesting_balance = ASSET( "1.000000 VESTS" );
            a.reward_vesting_steem = ASSET( "1.000 TESTS" );
         });

         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_supply += ASSET( "2.000 TESTS" );
            gpo.current_sbd_supply += ASSET( "1.000 TBD" );
            gpo.pending_rewarded_vesting_shares += ASSET( "1.000000 VESTS" );
            gpo.pending_rewarded_vesting_steem += ASSET( "1.000 TESTS" );
         });
      });

      validate_database();

      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).balance == ASSET( "1.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).sbd_balance == ASSET( "2.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).vesting_shares > ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_balance == ASSET( "4.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_sbd_balance == ASSET( "5.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_sbd_balance == ASSET( "1.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_steem_balance == ASSET( "1.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_balance == ASSET( "1.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_steem == ASSET( "1.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "2.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).sbd_balance == ASSET( "3.000 TBD" ) );

      BOOST_TEST_MESSAGE( "--- Generating block to clear balances" );
      generate_block();
      validate_database();

      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).vesting_shares == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_steem_balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_balance == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_steem == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "2.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).sbd_balance == ASSET( "3.000 TBD" ) );
   }
   FC_LOG_AND_RETHROW()*/
}

BOOST_AUTO_TEST_SUITE_END()
