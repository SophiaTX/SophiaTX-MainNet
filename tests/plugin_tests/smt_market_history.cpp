#if defined IS_TEST_NET && defined SOPHIATX_ENABLE_SMT
#include <boost/test/unit_test.hpp>

#include <sophiatx/chain/account_object.hpp>
#include <sophiatx/chain/comment_object.hpp>
#include <sophiatx/protocol/sophiatx_operations.hpp>

#include <sophiatx/plugins/market_history/market_history_plugin.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace sophiatx::chain;
using namespace sophiatx::protocol;

BOOST_FIXTURE_TEST_SUITE( smt_market_history, smt_database_fixture_for_plugin )

BOOST_AUTO_TEST_CASE( smt_mh_test )
{
   using namespace sophiatx::plugins::market_history;

   try
   {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;
      for( int i=1; i<argc; i++ )
      {
         const std::string arg = argv[i];
         if( arg == "--record-assert-trip" )
            fc::enable_record_assert_trip = true;
         if( arg == "--show-test-names" )
            std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
      }

      appbase::app().register_plugin< market_history_plugin >();
      db_plugin = &appbase::app().register_plugin< sophiatx::plugins::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      db_plugin->logging = false;
      appbase::app().initialize<
         sophiatx::plugins::market_history::market_history_plugin,
         sophiatx::plugins::debug_node::debug_node_plugin
      >( argc, argv );

      db = &appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db();
      BOOST_REQUIRE( db );

      open_database();

      generate_block();
      db->set_hardfork( SOPHIATX_NUM_HARDFORKS );
      generate_block();

      vest( "initminer", 10000 );

      // Fill up the rest of the required miners
      for( int i = SOPHIATX_NUM_INIT_MINERS; i < SOPHIATX_MAX_WITNESSES; i++ )
      {
         account_create( SOPHIATX_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
         fund( SOPHIATX_INIT_MINER_NAME + fc::to_string( i ), SOPHIATX_MIN_PRODUCER_REWARD.amount.value );
         witness_create( SOPHIATX_INIT_MINER_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key, SOPHIATX_MIN_PRODUCER_REWARD.amount );
      }

      validate_database();

      ACTORS( (alice)(bob)(sam)(smtcreator) );

      signed_transaction tx;
      asset_symbol_type any_smt_symbol = create_smt( "smtcreator", smtcreator_private_key, 3);
 
      fund( "alice", ASSET( "1000.000 TESTS" ) );
      fund( "bob", ASSET( "1000.000 TESTS" ) );
      fund( "sam", ASSET( "1000.000 TESTS" ) );
      fund( "alice", asset( 1000000, any_smt_symbol ) );

      tx.operations.clear();
      tx.signatures.clear();

      const auto& bucket_idx = db->get_index< bucket_index >().indices().get< by_bucket >();
      const auto& order_hist_idx = db->get_index< order_history_index >().indices().get< by_id >();

      BOOST_REQUIRE( bucket_idx.begin() == bucket_idx.end() );
      BOOST_REQUIRE( order_hist_idx.begin() == order_hist_idx.end() );
      validate_database();

      auto fill_order_a_time = db->head_block_time();
      auto time_a = fc::time_point_sec( ( fill_order_a_time.sec_since_epoch() / 15 ) * 15 );

      limit_order_create_operation op;
      op.owner = "alice";
      op.amount_to_sell = asset( 1000, any_smt_symbol );
      op.min_to_receive = ASSET( "2.000 TESTS" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "bob";
      op.amount_to_sell = ASSET( "1.500 TESTS" );
      op.min_to_receive = asset( 750, any_smt_symbol );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ( 60 * 90 ) );

      auto fill_order_b_time = db->head_block_time();

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "sam";
      op.amount_to_sell = ASSET( "1.000 TESTS" );
      op.min_to_receive = asset( 500, any_smt_symbol );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + 60 );

      auto fill_order_c_time = db->head_block_time();

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "alice";
      op.amount_to_sell = asset( 500, any_smt_symbol );
      op.min_to_receive = ASSET( "0.900 TESTS" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "bob";
      op.amount_to_sell = ASSET( "0.450 TESTS" );
      op.min_to_receive = asset( 250, any_smt_symbol );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      auto bucket = bucket_idx.begin();

      BOOST_REQUIRE( bucket->seconds == 15 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "1.500 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "1.500 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 750, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 15 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "0.500 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "0.500 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 250, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 15 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) + 60 );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "0.450 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "0.450 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "0.950 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 500, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 60 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "1.500 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "1.500 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 750, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 60 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "0.500 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "0.500 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 250, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 60 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) + 60 );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "0.450 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "0.450 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "0.950 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 500, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 300 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "1.500 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "1.500 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 750, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 300 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "0.450 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "0.450 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "1.450 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 750, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 3600 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "1.500 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "1.500 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 750, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 3600 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 60 ) );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "0.450 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "0.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "0.450 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "1.450 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 750, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 86400 );
      BOOST_REQUIRE( bucket->open == SOPHIATX_GENESIS_TIME );
      BOOST_REQUIRE( bucket->sophiatx.high == ASSET( "0.450 TESTS " ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.high == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.low == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.low == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.open == ASSET( "1.500 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.open == asset( 750, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.close == ASSET( "0.450 TESTS").amount );
      BOOST_REQUIRE( bucket->non_sophiatx.close == asset( 250, any_smt_symbol ).amount );
      BOOST_REQUIRE( bucket->sophiatx.volume == ASSET( "2.950 TESTS" ).amount );
      BOOST_REQUIRE( bucket->non_sophiatx.volume == asset( 1500, any_smt_symbol ).amount );
      bucket++;

      BOOST_REQUIRE( bucket == bucket_idx.end() );

      auto order = order_hist_idx.begin();

      BOOST_REQUIRE( order->time == fill_order_a_time );
      BOOST_REQUIRE( order->op.current_owner == "bob" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == ASSET( "1.500 TESTS" ) );
      BOOST_REQUIRE( order->op.open_owner == "alice" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == asset( 750, any_smt_symbol ) );
      order++;

      BOOST_REQUIRE( order->time == fill_order_b_time );
      BOOST_REQUIRE( order->op.current_owner == "sam" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == ASSET( "0.500 TESTS" ) );
      BOOST_REQUIRE( order->op.open_owner == "alice" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == asset( 250, any_smt_symbol ) );
      order++;

      BOOST_REQUIRE( order->time == fill_order_c_time );
      BOOST_REQUIRE( order->op.current_owner == "alice" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == asset( 250, any_smt_symbol ) );
      BOOST_REQUIRE( order->op.open_owner == "sam" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == ASSET( "0.500 TESTS" ) );
      order++;

      BOOST_REQUIRE( order->time == fill_order_c_time );
      BOOST_REQUIRE( order->op.current_owner == "bob" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == ASSET( "0.450 TESTS" ) );
      BOOST_REQUIRE( order->op.open_owner == "alice" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == asset( 250, any_smt_symbol ) );
      order++;

      BOOST_REQUIRE( order == order_hist_idx.end() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
