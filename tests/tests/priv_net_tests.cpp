#include <boost/test/unit_test.hpp>

#include <sophiatx/protocol/exceptions.hpp>
#include <sophiatx/protocol/hardfork.hpp>

#include <sophiatx/chain/database.hpp>
#include <sophiatx/chain/database_exceptions.hpp>
#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/application_object.hpp>

#include <sophiatx/plugins/witness/witness_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace sophiatx;
using namespace sophiatx::chain;
using namespace sophiatx::protocol;
using fc::string;

#define DUMP( x ) {fc::variant vo; fc::to_variant( x , vo); std::cout<< fc::json::to_string(vo) <<"\n";}
BOOST_FIXTURE_TEST_SUITE( operation_tests, private_database_fixture )

BOOST_AUTO_TEST_CASE( priv_witness_update_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withness_update_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( priv_witness_update_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_update_authorities" );

      ACTORS( (alice)(bob) );

      private_key_type signing_key = generate_private_key( "new_key" );

      witness_update_operation op;
      op.owner = AN("alice");
      op.url = "foo.bar";
      op.block_signing_key = signing_key.get_public_key();

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      sign(tx, alice_private_key );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      sign(tx, alice_private_key );
      sign(tx, bob_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( priv_witness_update_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_update_apply" );

      ACTORS( (alice) )
      private_key_type signing_key = generate_private_key( "new_key" );

      BOOST_TEST_MESSAGE( "--- Test upgrading an account to a witness" );

      witness_update_operation op;
      op.owner = AN("alice");
      op.url = "foo.bar";
      op.block_signing_key = signing_key.get_public_key();
      op.props.maximum_block_size = SOPHIATX_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign(tx, alice_private_key );

      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( priv_admin_witness_update_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: admin_witness_update_authorities" );

      ACTORS( (alice)(bob) );

      private_key_type signing_key = generate_private_key( "new_key" );

      admin_witness_update_operation op;
      op.owner = AN("alice");
      op.url = "foo.bar";
      op.block_signing_key = signing_key.get_public_key();

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when wrong signatures" );
      tx.signatures.clear();
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      sign(tx, init_account_priv_key );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( priv_admin_witness_update_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: admin_witness_update_apply" );

      ACTORS( (alice) )
      private_key_type signing_key = generate_private_key( "new_key" );

      BOOST_TEST_MESSAGE( "--- Test upgrading an account to a witness" );

      admin_witness_update_operation op;
      op.owner = AN("alice");
      op.url = "foo.bar";
      op.block_signing_key = signing_key.get_public_key();
      op.props.maximum_block_size = SOPHIATX_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign(tx, init_account_priv_key );

      db->push_transaction( tx, 0 );
      const witness_object& alice_witness = db->get_witness( AN("alice") );

      BOOST_REQUIRE( alice_witness.owner == AN("alice") );
      BOOST_REQUIRE( alice_witness.created == db->head_block_time() );
      BOOST_REQUIRE( to_string( alice_witness.url ) == op.url );

      BOOST_REQUIRE( alice_witness.total_missed == 0 );
      BOOST_REQUIRE( alice_witness.last_aslot == 0 );
      BOOST_REQUIRE( alice_witness.last_confirmed_block_num == 0 );
      BOOST_REQUIRE( alice_witness.votes.value == 0 );
      BOOST_REQUIRE( alice_witness.virtual_last_update == static_cast<fc::uint128_t>(0) );
      BOOST_REQUIRE( alice_witness.virtual_position == static_cast<fc::uint128_t>(0));
      BOOST_REQUIRE( alice_witness.virtual_scheduled_time == fc::uint128_t::max_value() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "0.000000 SPHTX" ).amount.value); // No fee
      validate_database();

      witness_update_operation op2;
      tx.signatures.clear();
      tx.operations.clear();
      op2.url = "bar.foo";
      op2.owner = AN("alice");
      op2.block_signing_key = signing_key.get_public_key();
      op2.props.maximum_block_size = SOPHIATX_MIN_BLOCK_SIZE_LIMIT + 100;
      tx.operations.push_back( op2 );
      sign(tx, alice_private_key );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice_witness.owner == AN("alice") );
      BOOST_REQUIRE( alice_witness.created == db->head_block_time() );
      BOOST_REQUIRE( to_string( alice_witness.url ) == "bar.foo" );
      BOOST_REQUIRE( alice_witness.signing_key == op.block_signing_key );
      BOOST_REQUIRE( alice_witness.props.maximum_block_size == op.props.maximum_block_size );
      BOOST_REQUIRE( alice_witness.total_missed == 0 );
      BOOST_REQUIRE( alice_witness.last_aslot == 0 );
      BOOST_REQUIRE( alice_witness.last_confirmed_block_num == 0 );
      BOOST_REQUIRE( alice_witness.votes.value == 0 );
      BOOST_REQUIRE( alice_witness.virtual_last_update == static_cast<fc::uint128_t>(0));
      BOOST_REQUIRE( alice_witness.virtual_position == static_cast<fc::uint128_t>(0));
      BOOST_REQUIRE( alice_witness.virtual_scheduled_time == fc::uint128_t::max_value() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "0.000000 SPHTX" ).amount.value ); // No fee
      validate_database();

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( priv_check_unsupported_ops )
{
   try{
      BOOST_TEST_MESSAGE( "Testing: unsupported operations" );

      ACTORS( (alice)(bob)(cecil) )

      escrow_transfer_operation et_op;
      et_op.from = AN("alice");
      et_op.to = AN("bob");
      et_op.agent = AN("cecil");
      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( et_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      escrow_approve_operation ea_op;
      ea_op.from = AN("alice");
      ea_op.to = AN("bob");
      ea_op.agent = AN("cecil");
      ea_op.who = AN("cecil");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( ea_op );
      sign(tx, cecil_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      escrow_dispute_operation ed_op;
      ed_op.from = AN("alice");
      ed_op.to = AN("bob");
      ed_op.agent = AN("cecil");
      ed_op.who = AN("alice");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( ed_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      escrow_release_operation er_op;
      er_op.from = AN("alice");
      er_op.to = AN("bob");
      er_op.agent = AN("cecil");
      er_op.who = AN("alice");
      er_op.receiver = AN("bob");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( er_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      transfer_operation t_op;
      t_op.from = AN("alice");
      t_op.to = AN("bob");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( t_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      transfer_to_vesting_operation tv_op;
      tv_op.from = AN("alice");
      tv_op.to = AN("bob");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( tv_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      withdraw_vesting_operation wv_op;
      wv_op.account = AN("alice");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( wv_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      account_witness_proxy_operation wp_op;
      wp_op.account = AN("alice");
      wp_op.proxy = AN("bob");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( wp_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      account_witness_vote_operation vw_op;
      vw_op.account = AN("alice");
      vw_op.witness = AN("bob");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( vw_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      feed_publish_operation fp_op;
      fp_op.publisher = AN("alice");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( fp_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      buy_application_operation ba_op;
      ba_op.buyer = AN("alice");
      ba_op.app_id = 0;
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( ba_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      cancel_application_buying_operation ca_op;
      ca_op.app_owner = AN("alice");
      ca_op.buyer = AN("bob");
      ca_op.app_id = 0;
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( ca_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      transfer_from_promotion_pool_operation tp_op;
      tp_op.transfer_to = AN("alice");
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( tp_op );
      sign(tx, init_account_priv_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      sponsor_fees_operation sf_op;
      sf_op.sponsor = AN("alice");
      sf_op.sponsored = AN("bob");
      sf_op.is_sponsoring = true;
      tx.operations.clear(); tx.signatures.clear();
      tx.operations.push_back( sf_op );
      sign(tx, alice_private_key );
      SOPHIATX_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()