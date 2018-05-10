//#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>


#include <steem/plugins/witness/witness_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

#define DUMP( x ) {fc::variant vo; fc::to_variant( x , vo); std::cout<< fc::json::to_string(vo) <<"\n";}
BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( account_create_validate )
{
   try
   {

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_create_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_authorities" );

      account_create_operation op;
      op.creator = "alice";
      op.new_account_name = "bob";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      BOOST_TEST_MESSAGE( "--- Testing owner authority" );
      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      BOOST_TEST_MESSAGE( "--- Testing active authority" );
      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_create_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_apply" );

      set_price_feed( price( ASSET( "1.000 SBD" ), ASSET( "1.000 SPHTX" ) ) );

      signed_transaction tx;
      private_key_type priv_key = generate_private_key( "alice" );

      const account_object& init = db->get_account( STEEM_INIT_MINER_NAME );
      asset init_starting_balance = init.balance;

      const auto& gpo = db->get_dynamic_global_properties();

      account_create_operation op;

      op.fee = ASSET( "0.100000 SPHTX" );
      op.new_account_name = "alice";
      op.creator = STEEM_INIT_MINER_NAME;
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 2, priv_key.get_public_key(), 2 );
      op.memo_key = priv_key.get_public_key();
      op.json_metadata = "{\"foo\":\"bar\"}";

      BOOST_TEST_MESSAGE( "--- Test normal account creation" );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      tx.validate();
      db->push_transaction( tx, 0 );


      const account_object& acct = db->get_account( "alice" );
      const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
      BOOST_REQUIRE( acct.proxy == "" );
      BOOST_REQUIRE( acct.created == db->head_block_time() );
      BOOST_REQUIRE( acct.balance.amount.value == 0 );
      BOOST_REQUIRE( acct.id._id == acct_auth.id._id );

      /// because init_witness has created vesting shares and blocks have been produced, 100 STEEM is worth less than 100 vesting shares due to rounding
      BOOST_REQUIRE( acct.vesting_shares.amount.value == 0 );
      BOOST_REQUIRE( acct.vesting_withdraw_rate.amount.value == 0 );
      BOOST_REQUIRE( acct.proxied_vsf_votes_total().value == 0 );
      //This doeas not hold due to interests...
      // BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100000 SPHTX" ) ).amount.value == init.balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure of duplicate account creation" );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
      BOOST_REQUIRE( acct.proxy == "" );
      BOOST_REQUIRE( acct.created == db->head_block_time() );
      BOOST_REQUIRE( acct.balance.amount.value == ASSET( "0.000000 SPHTX " ).amount.value );
      BOOST_REQUIRE( acct.vesting_shares.amount.value == 0 );
      BOOST_REQUIRE( acct.vesting_withdraw_rate.amount.value == ASSET( "0.000000 VESTS" ).amount.value );
      BOOST_REQUIRE( acct.proxied_vsf_votes_total().value == 0 );
      //This doeas not hold due to interests...
      //BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100000 SPHTX" ) ).amount.value == init.balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when creator cannot cover fee" );
      tx.signatures.clear();
      tx.operations.clear();
      op.fee = asset( db->get_account( STEEM_INIT_MINER_NAME ).balance.amount + 1, STEEM_SYMBOL );
      op.new_account_name = "bob";
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure covering witness fee" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "10.000000 SPHTX" );
         });
      });
      generate_block();

      tx.clear();
      op.fee = ASSET( "1.000000 SPHTX" );
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_validate" );

      ACTORS( (alice) )

      account_update_operation op;
      op.account = "alice";
      op.active = authority();
      op.active->weight_threshold = 1;
      op.active->add_authorities( "abcdefghijklmnopq", 1 );

      try
      {
         op.validate();

         signed_transaction tx;
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         BOOST_FAIL( "An exception was not thrown for an invalid account name" );
      }
      catch( fc::exception& ) {}

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_authorities" );

      ACTORS( (alice)(bob) )
      private_key_type active_key = generate_private_key( "new_key" );

      db->modify( db->get< account_authority_object, by_account >( "alice" ), [&]( account_authority_object& a )
      {
         a.active = authority( 1, active_key.get_public_key(), 1 );
      });

      account_update_operation op;
      op.account = "alice";
      op.json_metadata = "{\"success\":true}";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "  Tests when owner authority is not updated ---" );
      BOOST_TEST_MESSAGE( "--- Test failure when no signature" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when wrong signature" );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when containing additional incorrect signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when containing duplicate signatures" );
      tx.signatures.clear();
      tx.sign( active_key, db->get_chain_id() );
      tx.sign( active_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test success on active key" );
      tx.signatures.clear();
      tx.sign( active_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test success on owner key alone" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "  Tests when owner authority is updated ---" );
      BOOST_TEST_MESSAGE( "--- Test failure when updating the owner authority with an active key" );
      tx.signatures.clear();
      tx.operations.clear();
      op.owner = authority( 1, active_key.get_public_key(), 1 );
      tx.operations.push_back( op );
      tx.sign( active_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_owner_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when owner key and active key are present" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when incorrect signature" );
      tx.signatures.clear();
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0), tx_missing_owner_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate owner keys are present" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test success when updating the owner authority with an owner key" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_apply" );

      ACTORS( (alice) )
      private_key_type new_private_key = generate_private_key( "new_key" );

      BOOST_TEST_MESSAGE( "--- Test normal update" );

      account_update_operation op;
      op.account = "alice";
      op.owner = authority( 1, new_private_key.get_public_key(), 1 );
      op.active = authority( 2, new_private_key.get_public_key(), 2 );
      op.memo_key = new_private_key.get_public_key();
      op.json_metadata = "{\"bar\":\"foo\"}";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const account_object& acct = db->get_account( "alice" );
      const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, new_private_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, new_private_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == new_private_key.get_public_key() );

      /* This is being moved out of consensus
      #ifndef IS_LOW_MEM
         BOOST_REQUIRE( acct.json_metadata == "{\"bar\":\"foo\"}" );
      #else
         BOOST_REQUIRE( acct.json_metadata == "" );
      #endif
      */

      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when updating a non-existent account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( new_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception )
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test failure when account authority does not exist" );
      tx.clear();
      op = account_update_operation();
      op.account = "alice";
      op.active = authority();
      op.active->weight_threshold = 1;
      op.active->add_authorities( "dave", 1 );
      tx.operations.push_back( op );
      tx.sign( new_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE( transfer_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_authorities )
{
   try
   {
      ACTORS( (alice)(bob) )
      fund( "alice", 10000000 );

      BOOST_TEST_MESSAGE( "Testing: transfer_authorities" );

      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "2.500 SPHTX" );

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( signature_stripping )
{
   try
   {
      // Alice, Bob and Sam all have 2-of-3 multisig on corp.
      // Legitimate tx signed by (Alice, Bob) goes through.
      // Sam shouldn't be able to add or remove signatures to get the transaction to process multiple times.

      ACTORS( (alice)(bob)(sam)(corp) )
      fund( "corp", 10000000 );

      account_update_operation update_op;
      update_op.account = "corp";
      update_op.active = authority( 2, "alice", 1, "bob", 1, "sam", 1 );

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( update_op );

      tx.sign( corp_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      transfer_operation transfer_op;
      transfer_op.from = "corp";
      transfer_op.to = "sam";
      transfer_op.amount = ASSET( "1.000 SPHTX" );

      tx.operations.push_back( transfer_op );

      tx.sign( alice_private_key, db->get_chain_id() );
      signature_type alice_sig = tx.signatures.back();
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.sign( bob_private_key, db->get_chain_id() );
      signature_type bob_sig = tx.signatures.back();
      tx.sign( sam_private_key, db->get_chain_id() );
      signature_type sam_sig = tx.signatures.back();
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      tx.signatures.clear();
      tx.signatures.push_back( alice_sig );
      tx.signatures.push_back( bob_sig );
      db->push_transaction( tx, 0 );

      tx.signatures.clear();
      tx.signatures.push_back( alice_sig );
      tx.signatures.push_back( sam_sig );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_apply" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000000 );

      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "10.000000 SPHTX" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET(" 0.000000 SPHTX" ).amount.value );

      signed_transaction tx;
      transfer_operation op;

      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "5.000000 SPHTX" );

      BOOST_TEST_MESSAGE( "--- Test normal transaction" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.balance.amount.value >= ASSET( "5.000000 SPHTX" ).amount.value && alice.balance.amount.value < ASSET( "5.010000 SPHTX" ).amount.value);
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "5.000000 SPHTX" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Generating a block" );
      generate_block();

      const auto& new_alice = db->get_account( "alice" );
      const auto& new_bob = db->get_account( "bob" );

      BOOST_REQUIRE( new_alice.balance.amount.value >= ASSET( "5.000000 SPHTX" ).amount.value && new_alice.balance.amount.value < ASSET( "5.010000 SPHTX" ).amount.value );
      BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "5.000000 SPHTX" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test emptying an account" );
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_REQUIRE( new_alice.balance.amount.value >= ASSET( "0.000000 SPHTX" ).amount.value && new_alice.balance.amount.value < ASSET( "0.010000 SPHTX" ).amount.value);
      BOOST_REQUIRE( new_bob.balance.amount.value >= ASSET( "10.000000 SPHTX" ).amount.value && new_bob.balance.amount.value < ASSET( "10.010000 SPHTX" ).amount.value);
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test transferring non-existent funds" );
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( new_alice.balance.amount.value >= ASSET( "0.000000 SPHTX" ).amount.value && new_alice.balance.amount.value < ASSET( "0.010000 SPHTX" ).amount.value);
      BOOST_REQUIRE( new_bob.balance.amount.value >= ASSET( "10.000000 SPHTX" ).amount.value && new_bob.balance.amount.value < ASSET( "10.010000 SPHTX" ).amount.value);
      validate_database();

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_vesting_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_vesting_authorities )
{
   try
   {
      ACTORS( (alice)(bob) )
      fund( "alice", 10000000 );

      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_authorities" );

      transfer_to_vesting_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "2.500 SPHTX" );

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with from signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_vesting_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_apply" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000000 );

      const auto& gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( alice.balance == ASSET( "10.000000 SPHTX" ) );

      auto alice_shares = alice.vesting_shares;
      auto bob_shares = bob.vesting_shares;

      transfer_to_vesting_operation op;
      op.from = "alice";
      op.to = "";
      op.amount = ASSET( "7.500000 SPHTX" );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "2.500000 SPHTX" ).amount.value );
      BOOST_REQUIRE( alice.vesting_shares.amount.value == ASSET( "7.500000 VESTS" ).amount.value );
      validate_database();

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_vesting_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_vesting_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000000 );
      vest( "alice", 10000000 );

      const auto& new_alice = db->get_account("alice");
      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = ASSET( "1.000000 VESTS" );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_vesting_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_apply" );

      ACTORS( (alice) )
      generate_block();
      fund( "alice", 10000000 );
      vest( "alice", ASSET( "10.000000 SPHTX" ) );
      generate_block();


      BOOST_TEST_MESSAGE( "--- Test withdraw of existing VESTS" );

      {
      const auto& alice = db->get_account( "alice" );

      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( alice.vesting_shares.amount / 2, VESTS_SYMBOL );

      auto old_vesting_shares = alice.vesting_shares;

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 2 ) ).value );
      BOOST_REQUIRE( alice.to_withdraw.value == op.vesting_shares.amount.value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test changing vesting withdrawal" );
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( alice.vesting_shares.amount / 3, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 3 ) ).value );
      BOOST_REQUIRE( alice.to_withdraw.value == op.vesting_shares.amount.value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing more vests than available" );
      //auto old_withdraw_amount = alice.to_withdraw;
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( alice.vesting_shares.amount * 2, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 3 ) ).value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing 0 to reset vesting withdraw" );
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( 0, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == 0 );
      BOOST_REQUIRE( alice.to_withdraw.value == 0 );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );


      BOOST_TEST_MESSAGE( "--- Test cancelling a withdraw when below the account creation fee" );
      op.vesting_shares = alice.vesting_shares;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();
      }

      db_plugin->debug_update( [=]( database& db )
      {
         auto& wso = db.get_witness_schedule_object();

         db.modify( wso, [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "10.000000 SPHTX" );
         });


         db.update_virtual_supply();
      }, database::skip_witness_signature );

      withdraw_vesting_operation op;
      signed_transaction tx;
      op.account = "alice";
      op.vesting_shares = ASSET( "0.000000 VESTS" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_withdraw_rate == ASSET( "0.000000 VESTS" ) );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( witness_update_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withness_update_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( witness_update_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_update_authorities" );

      ACTORS( (alice)(bob) );
      fund( "alice", 10000000 );

      private_key_type signing_key = generate_private_key( "new_key" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.fee = ASSET( "1.000000 SPHTX" );
      op.block_signing_key = signing_key.get_public_key();

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.signatures.clear();
      tx.sign( signing_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( witness_update_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_update_apply" );

      ACTORS( (alice) )
      fund( "alice", 10000000 );

      private_key_type signing_key = generate_private_key( "new_key" );

      BOOST_TEST_MESSAGE( "--- Test upgrading an account to a witness" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.block_signing_key = signing_key.get_public_key();
      op.props.account_creation_fee = ASSET("1.000000 SPHTX");
      op.props.maximum_block_size = STEEM_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      const witness_object& alice_witness = db->get_witness( "alice" );

      BOOST_REQUIRE( alice_witness.owner == "alice" );
      BOOST_REQUIRE( alice_witness.created == db->head_block_time() );
      BOOST_REQUIRE( to_string( alice_witness.url ) == op.url );
      BOOST_REQUIRE( alice_witness.signing_key == op.block_signing_key );
      BOOST_REQUIRE( alice_witness.props.account_creation_fee.amount.value == 1000000 );
      BOOST_REQUIRE( alice_witness.props.maximum_block_size == op.props.maximum_block_size );
      BOOST_REQUIRE( alice_witness.total_missed == 0 );
      BOOST_REQUIRE( alice_witness.last_aslot == 0 );
      BOOST_REQUIRE( alice_witness.last_confirmed_block_num == 0 );
      BOOST_REQUIRE( alice_witness.votes.value == 0 );
      BOOST_REQUIRE( alice_witness.virtual_last_update == 0 );
      BOOST_REQUIRE( alice_witness.virtual_position == 0 );
      BOOST_REQUIRE( alice_witness.virtual_scheduled_time == fc::uint128_t::max_value() );
      BOOST_REQUIRE( alice.balance.amount.value >= ASSET( "10.000000 SPHTX" ).amount.value && alice.balance.amount.value < ASSET( "10.010000 SPHTX" ).amount.value); // No fee
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test updating a witness" );

      tx.signatures.clear();
      tx.operations.clear();
      op.url = "bar.foo";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice_witness.owner == "alice" );
      BOOST_REQUIRE( alice_witness.created == db->head_block_time() );
      BOOST_REQUIRE( to_string( alice_witness.url ) == "bar.foo" );
      BOOST_REQUIRE( alice_witness.signing_key == op.block_signing_key );
      BOOST_REQUIRE( alice_witness.props.account_creation_fee.amount.value == 1000000 );
      BOOST_REQUIRE( alice_witness.props.maximum_block_size == op.props.maximum_block_size );
      BOOST_REQUIRE( alice_witness.total_missed == 0 );
      BOOST_REQUIRE( alice_witness.last_aslot == 0 );
      BOOST_REQUIRE( alice_witness.last_confirmed_block_num == 0 );
      BOOST_REQUIRE( alice_witness.votes.value == 0 );
      BOOST_REQUIRE( alice_witness.virtual_last_update == 0 );
      BOOST_REQUIRE( alice_witness.virtual_position == 0 );
      BOOST_REQUIRE( alice_witness.virtual_scheduled_time == fc::uint128_t::max_value() );
      BOOST_REQUIRE( alice.balance.amount.value >= ASSET( "10.000000 SPHTX" ).amount.value && alice.balance.amount.value < ASSET( "10.010000 SPHTX" ).amount.value); // No fee
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when upgrading a non-existent account" );

      tx.signatures.clear();
      tx.operations.clear();
      op.owner = "bob";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_vote_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_vote_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_authorities" );

      ACTORS( (alice)(bob)(sam) )

      fund( "alice", 1000000 );
      private_key_type alice_witness_key = generate_private_key( "alice_witness" );
      witness_create( "alice", alice_private_key, "foo.bar", alice_witness_key.get_public_key(), 1000000 );

      account_witness_vote_operation op;
      op.account = "bob";
      op.witness = "alice";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
      proxy( "bob", "sam" );
      tx.signatures.clear();
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_vote_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_apply" );

      ACTORS( (alice)(bob)(sam) )
      fund( "alice" , 5000000 );
      vest( "alice", 5000000 );
      fund( "sam", 1000000 );

      private_key_type sam_witness_key = generate_private_key( "sam_key" );
      witness_create( "sam", sam_private_key, "foo.bar", sam_witness_key.get_public_key(), 1000 );
      const witness_object& sam_witness = db->get_witness( "sam" );

      const auto& witness_vote_idx = db->get_index< witness_vote_index >().indices().get< by_witness_account >();

      BOOST_TEST_MESSAGE( "--- Test normal vote" );
      account_witness_vote_operation op;
      op.account = "alice";
      op.witness = "sam";
      op.approve = true;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes == alice.vesting_shares.amount );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) != witness_vote_idx.end() );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test revoke vote" );
      op.approve = false;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test failure when attempting to revoke a non-existent vote" );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test proxied vote" );
      proxy( "alice", "bob" );
      tx.operations.clear();
      tx.signatures.clear();
      op.approve = true;
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes == ( bob.proxied_vsf_votes_total() + bob.vesting_shares.amount ) );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) != witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test vote from a proxied account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( sam_witness.votes == ( bob.proxied_vsf_votes_total() + bob.vesting_shares.amount ) );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) != witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test revoke proxied vote" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "bob";
      op.approve = false;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) == witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test failure when voting for a non-existent account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.witness = "dave";
      op.approve = true;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when voting for an account that is not a witness" );
      tx.operations.clear();
      tx.signatures.clear();
      op.witness = "alice";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_proxy_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_proxy_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_authorities" );

      ACTORS( (alice)(bob) )

      account_witness_proxy_operation op;
      op.account = "bob";
      op.proxy = "alice";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_proxy_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 1000000 );
      vest( "alice", 1000000 );
      fund( "bob", 3000000 );
      vest( "bob", 3000000 );
      fund( "sam", 5000000 );
      vest( "sam", 5000000 );
      fund( "dave", 7000000 );
      vest( "dave", 7000000 );

      BOOST_TEST_MESSAGE( "--- Test setting proxy to another account from self." );
      // bob -> alice

      account_witness_proxy_operation op;
      op.account = "bob";
      op.proxy = "alice";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      const auto& new_bob = db->get_account("bob");
      const auto& new_alice = db->get_account("alice");
      BOOST_REQUIRE( new_bob.proxy == "alice" );
      BOOST_REQUIRE( new_bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_alice.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( new_alice.proxied_vsf_votes_total() == new_bob.witness_vote_weight() );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test changing proxy" );
      // bob->sam

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "sam";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );
      const auto& new_sam = db->get_account("sam");

      BOOST_REQUIRE( new_bob.proxy == "sam" );
      BOOST_REQUIRE( new_bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_sam.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( new_sam.proxied_vsf_votes_total().value == new_bob.witness_vote_weight() );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when changing proxy to existing proxy" );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( new_bob.proxy == "sam" );
      BOOST_REQUIRE( new_bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_sam.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( new_sam.proxied_vsf_votes_total() == new_bob.witness_vote_weight());
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test adding a grandparent proxy" );
      // bob->sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "dave";
      op.account = "sam";
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );
      const auto& new_dave = db->get_account("dave");

      BOOST_REQUIRE( new_bob.proxy == "sam" );
      BOOST_REQUIRE( new_bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_sam.proxy == "dave" );
      BOOST_REQUIRE( new_sam.proxied_vsf_votes_total() == new_bob.witness_vote_weight() );
      BOOST_REQUIRE( new_dave.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( new_dave.proxied_vsf_votes_total() == ( new_sam.witness_vote_weight() + new_bob.witness_vote_weight() ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test adding a grandchild proxy" );
      //       alice
      //         |
      // bob->  sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "sam";
      op.account = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( new_alice.proxy == "sam" );
      BOOST_REQUIRE( new_alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_bob.proxy == "sam" );
      BOOST_REQUIRE( new_bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_sam.proxy == "dave" );
      BOOST_REQUIRE( new_sam.proxied_vsf_votes_total() == ( new_bob.witness_vote_weight() + new_alice.witness_vote_weight() ) );
      BOOST_REQUIRE( new_dave.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      DUMP(new_dave)
      DUMP(new_sam)
      DUMP(new_bob)
      DUMP(new_alice)
      BOOST_REQUIRE( new_dave.proxied_vsf_votes_total() == ( new_sam.witness_vote_weight() + new_bob.witness_vote_weight() + new_alice.witness_vote_weight() ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test removing a grandchild proxy" );
      // alice->sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = STEEM_PROXY_TO_SELF_ACCOUNT;
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( new_alice.proxy == "sam" );
      BOOST_REQUIRE( new_alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_bob.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( new_bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( new_sam.proxy == "dave" );
      BOOST_REQUIRE( new_sam.proxied_vsf_votes_total() == new_alice.witness_vote_weight() );
      BOOST_REQUIRE( new_dave.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( new_dave.proxied_vsf_votes_total() == ( new_sam.witness_vote_weight() + new_alice.witness_vote_weight() ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test votes are transferred when a proxy is added" );
      account_witness_vote_operation vote;
      vote.account= "bob";
      vote.witness = STEEM_INIT_MINER_NAME;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.account = "alice";
      op.proxy = "bob";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_witness( STEEM_INIT_MINER_NAME ).votes == ( new_alice.witness_vote_weight() + new_bob.witness_vote_weight() ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test votes are removed when a proxy is removed" );
      op.proxy = STEEM_PROXY_TO_SELF_ACCOUNT;
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_witness( STEEM_INIT_MINER_NAME ).votes == new_bob.witness_vote_weight() );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

/*BOOST_AUTO_TEST_CASE( custom_authorities )
{
   custom_operation op;
   op.required_auths.insert( "alice" );
   op.required_auths.insert( "bob" );

   flat_set< account_name_type > auths;
   flat_set< account_name_type > expected;

   op.get_required_owner_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   op.get_required_posting_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   expected.insert( "alice" );
   expected.insert( "bob" );
   op.get_required_active_authorities( auths );
   BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_CASE( custom_json_authorities )
{
   custom_json_operation op;
   op.required_auths.insert( "alice" );

   flat_set< account_name_type > auths;
   flat_set< account_name_type > expected;

   op.get_required_owner_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   expected.insert( "alice" );
   op.get_required_active_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   auths.clear();
   expected.clear();
   expected.insert( "bob" );
   op.get_required_posting_authorities( auths );
   BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_CASE( custom_binary_authorities )
{
   ACTORS( (alice) )

   custom_binary_operation op;
   op.required_owner_auths.insert( "alice" );
   op.required_active_auths.insert( "bob" );
   op.required_posting_auths.insert( "sam" );
   op.required_auths.push_back( db->get< account_authority_object, by_account >( "alice" ).posting );

   flat_set< account_name_type > acc_auths;
   flat_set< account_name_type > acc_expected;
   vector< authority > auths;
   vector< authority > expected;

   acc_expected.insert( "alice" );
   op.get_required_owner_authorities( acc_auths );
   BOOST_REQUIRE( acc_auths == acc_expected );

   acc_auths.clear();
   acc_expected.clear();
   acc_expected.insert( "bob" );
   op.get_required_active_authorities( acc_auths );
   BOOST_REQUIRE( acc_auths == acc_expected );

   acc_auths.clear();
   acc_expected.clear();
   acc_expected.insert( "sam" );
   op.get_required_posting_authorities( acc_auths );
   BOOST_REQUIRE( acc_auths == acc_expected );

   expected.push_back( db->get< account_authority_object, by_account >( "alice" ).posting );
   op.get_required_authorities( auths );
   BOOST_REQUIRE( auths == expected );
}*/

BOOST_AUTO_TEST_CASE( feed_publish_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: feed_publish_validate" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( feed_publish_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: feed_publish_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000000 );
      witness_create( "alice", alice_private_key, "foo.bar", alice_private_key.get_public_key(), 1000000 );

      feed_publish_operation op;
      op.publisher = "alice";
      op.exchange_rate = price( ASSET( "1.000000 SBD" ), ASSET( "1.000000 SPHTX" ) );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness account signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( feed_publish_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: feed_publish_apply" );

      ACTORS( (alice) )
      fund( "alice", 10000000 );
      witness_create( "alice", alice_private_key, "foo.bar", alice_private_key.get_public_key(), 1000000 );

      BOOST_TEST_MESSAGE( "--- Test publishing price feed" );
      feed_publish_operation op;
      op.publisher = "alice";
      op.exchange_rate = price( ASSET( "1.000000 SBD" ), ASSET( "1000.000000 SPHTX" ) ); // 1000 STEEM : 1 SBD

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      witness_object& alice_witness = const_cast< witness_object& >( db->get_witness( "alice" ) );

      BOOST_REQUIRE( alice_witness.sbd_exchange_rate == op.exchange_rate );
      BOOST_REQUIRE( alice_witness.last_sbd_exchange_update == db->head_block_time() );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure publishing to non-existent witness" );

      tx.operations.clear();
      tx.signatures.clear();
      op.publisher = "bob";
      tx.sign( alice_private_key, db->get_chain_id() );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure publishing with SBD base symbol" );

      tx.operations.clear();
      tx.signatures.clear();
      op.exchange_rate = price( ASSET( "1.000000 SBD" ), ASSET( "1.000000 SPHTX" ) );
      tx.sign( alice_private_key, db->get_chain_id() );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test updating price feed" );

      tx.operations.clear();
      tx.signatures.clear();
      op.exchange_rate = price( ASSET(" 1.000000 SBD" ), ASSET( "1500000.000 SPHTX" ) );
      op.publisher = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      alice_witness = const_cast< witness_object& >( db->get_witness( "alice" ) );
      // BOOST_REQUIRE( std::abs( alice_witness.sbd_exchange_rate.to_real() - op.exchange_rate.to_real() ) < 0.0000005 );
      BOOST_REQUIRE( alice_witness.last_sbd_exchange_update == db->head_block_time() );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE( account_recovery )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account recovery" );

      ACTORS( (alice) );
      fund( "alice", 10000000 );

      BOOST_TEST_MESSAGE( "Creating account bob with alice" );

      account_create_operation acc_create;
      acc_create.fee = ASSET( "10.000000 SPHTX" );
      acc_create.creator = "alice";
      acc_create.new_account_name = "bob";
      acc_create.owner = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
      acc_create.active = authority( 1, generate_private_key( "bob_active" ).get_public_key(), 1 );
      acc_create.memo_key = generate_private_key( "bob_memo" ).get_public_key();
      acc_create.json_metadata = "";


      signed_transaction tx;
      tx.operations.push_back( acc_create );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& bob_auth = db->get< account_authority_object, by_account >( "bob" );
      BOOST_REQUIRE( bob_auth.owner == acc_create.owner );


      BOOST_TEST_MESSAGE( "Changing bob's owner authority" );

      account_update_operation acc_update;
      acc_update.account = "bob";
      acc_update.owner = authority( 1, generate_private_key( "bad_key" ).get_public_key(), 1 );
      acc_update.memo_key = acc_create.memo_key;
      acc_update.json_metadata = "";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( acc_update );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );


      BOOST_TEST_MESSAGE( "Creating recover request for bob with alice" );

      request_account_recovery_operation request;
      request.recovery_account = "alice";
      request.account_to_recover = "bob";
      request.new_owner_authority = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );


      BOOST_TEST_MESSAGE( "Recovering bob's account with original owner auth and new secret" );

      generate_blocks( db->head_block_time() + STEEM_OWNER_UPDATE_LIMIT );

      recover_account_operation recover;
      recover.account_to_recover = "bob";
      recover.new_owner_authority = request.new_owner_authority;
      recover.recent_owner_authority = acc_create.owner;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "new_key" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );
      const auto& owner1 = db->get< account_authority_object, by_account >("bob").owner;

      BOOST_REQUIRE( owner1 == recover.new_owner_authority );


      BOOST_TEST_MESSAGE( "Creating new recover request for a bogus key" );

      request.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "Testing failure when bob does not have new authority" );

      generate_blocks( db->head_block_time() + STEEM_OWNER_UPDATE_LIMIT + fc::seconds( STEEM_BLOCK_INTERVAL ) );

      recover.new_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "idontknow" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner2 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner2 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );


      BOOST_TEST_MESSAGE( "Testing failure when bob does not have old authority" );

      recover.recent_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );
      recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      tx.sign( generate_private_key( "idontknow" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner3 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner3 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );


      BOOST_TEST_MESSAGE( "Testing using the same old owner auth again for recovery" );

      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
      recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& owner4 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner4 == recover.new_owner_authority );

      BOOST_TEST_MESSAGE( "Creating a recovery request that will expire" );

      request.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& request_idx = db->get_index< account_recovery_request_index >().indices();
      auto req_itr = request_idx.begin();

      BOOST_REQUIRE( req_itr->account_to_recover == "bob" );
      BOOST_REQUIRE( req_itr->new_owner_authority == authority( 1, generate_private_key( "expire" ).get_public_key(), 1 ) );
      BOOST_REQUIRE( req_itr->expires == db->head_block_time() + STEEM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD );
      auto expires = req_itr->expires;
      ++req_itr;
      BOOST_REQUIRE( req_itr == request_idx.end() );

      generate_blocks( time_point_sec( expires - STEEM_BLOCK_INTERVAL ), true );

      const auto& new_request_idx = db->get_index< account_recovery_request_index >().indices();
      BOOST_REQUIRE( new_request_idx.begin() != new_request_idx.end() );

      generate_block();

      BOOST_REQUIRE( new_request_idx.begin() == new_request_idx.end() );

      recover.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );
      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() );
      tx.sign( generate_private_key( "expire" ), db->get_chain_id() );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner5 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner5 == authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 ) );

      BOOST_TEST_MESSAGE( "Expiring owner authority history" );

      acc_update.owner = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( acc_update );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ( STEEM_OWNER_AUTH_RECOVERY_PERIOD - STEEM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD ) );
      generate_block();

      request.new_owner_authority = authority( 1, generate_private_key( "last key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      recover.new_owner_authority = request.new_owner_authority;
      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "last key" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner6 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner6 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );

      recover.recent_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      tx.sign( generate_private_key( "last key" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );
      const auto& owner7 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner7 == authority( 1, generate_private_key( "last key" ).get_public_key(), 1 ) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( change_recovery_account )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing change_recovery_account_operation" );

      ACTORS( (alice)(bob)(sam)(tyler) )

      auto change_recovery_account = [&]( const std::string& account_to_recover, const std::string& new_recovery_account )
      {
         change_recovery_account_operation op;
         op.account_to_recover = account_to_recover;
         op.new_recovery_account = new_recovery_account;

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      auto recover_account = [&]( const std::string& account_to_recover, const fc::ecc::private_key& new_owner_key, const fc::ecc::private_key& recent_owner_key )
      {
         recover_account_operation op;
         op.account_to_recover = account_to_recover;
         op.new_owner_authority = authority( 1, public_key_type( new_owner_key.get_public_key() ), 1 );
         op.recent_owner_authority = authority( 1, public_key_type( recent_owner_key.get_public_key() ), 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( recent_owner_key, db->get_chain_id() );
         // only Alice -> throw
         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         tx.signatures.clear();
         tx.sign( new_owner_key, db->get_chain_id() );
         // only Sam -> throw
         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         tx.sign( recent_owner_key, db->get_chain_id() );
         // Alice+Sam -> OK
         db->push_transaction( tx, 0 );
      };

      auto request_account_recovery = [&]( const std::string& recovery_account, const fc::ecc::private_key& recovery_account_key, const std::string& account_to_recover, const public_key_type& new_owner_key )
      {
         request_account_recovery_operation op;
         op.recovery_account    = recovery_account;
         op.account_to_recover  = account_to_recover;
         op.new_owner_authority = authority( 1, new_owner_key, 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( recovery_account_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      auto change_owner = [&]( const std::string& account, const fc::ecc::private_key& old_private_key, const public_key_type& new_public_key )
      {
         account_update_operation op;
         op.account = account;
         op.owner = authority( 1, new_public_key, 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( old_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      // if either/both users do not exist, we shouldn't allow it
      STEEM_REQUIRE_THROW( change_recovery_account("alice", "nobody"), fc::exception );
      STEEM_REQUIRE_THROW( change_recovery_account("haxer", "sam"   ), fc::exception );
      STEEM_REQUIRE_THROW( change_recovery_account("haxer", "nobody"), fc::exception );
      change_recovery_account("alice", "sam");

      fc::ecc::private_key alice_priv1 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k1" ) );
      fc::ecc::private_key alice_priv2 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k2" ) );
      public_key_type alice_pub1 = public_key_type( alice_priv1.get_public_key() );

      generate_blocks( db->head_block_time() + STEEM_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds( STEEM_BLOCK_INTERVAL ), true );
      // cannot request account recovery until recovery account is approved
      STEEM_REQUIRE_THROW( request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 ), fc::exception );
      generate_blocks(1);
      // cannot finish account recovery until requested
      STEEM_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
      // do the request
      request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 );
      // can't recover with the current owner key
      STEEM_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
      // unless we change it!
      change_owner( "alice", alice_private_key, public_key_type( alice_priv2.get_public_key() ) );
      recover_account( "alice", alice_priv1, alice_private_key );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_transfer_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_transfer_validate" );

      escrow_transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.steem_amount = ASSET( "1.000000 SPHTX" );
      op.escrow_id = 0;
      op.agent = "sam";
      op.fee = ASSET( "0.100 SPHTX" );
      op.json_meta = "";
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;

      BOOST_TEST_MESSAGE( "--- failure when steem symbol != STEEM" );
      op.steem_amount.symbol = SBD_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when fee symbol != SBD and fee symbol != STEEM" );
      op.steem_amount.symbol = STEEM_SYMBOL;
      op.fee.symbol = VESTS_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when sbd == 0 " );
      op.fee.symbol = STEEM_SYMBOL;
      op.steem_amount.amount = 0;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when steem < 0" );
      op.steem_amount.amount = -100;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when fee < 0" );
      op.steem_amount.amount = 1000;
      op.fee.amount = -100;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ratification deadline == escrow expiration" );
      op.fee.amount = 100;
      op.ratification_deadline = op.escrow_expiration;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ratification deadline > escrow expiration" );
      op.ratification_deadline = op.escrow_expiration + 100;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- success" );
      op.ratification_deadline = op.escrow_expiration - 100;
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_transfer_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_transfer_authorities" );

      escrow_transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.steem_amount = ASSET( "1.000000 SPHTX" );
      op.escrow_id = 0;
      op.agent = "sam";
      op.fee = ASSET( "0.100000 SPHTX" );
      op.json_meta = "";
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );


      op.get_required_active_authorities( auths );
      expected.insert( "alice" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_transfer_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_transfer_apply" );

      ACTORS( (alice)(bob)(sam) )

      fund( "alice", 10000000 );

      escrow_transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.steem_amount = ASSET( "1.000000 SPHTX" );
      op.escrow_id = 0;
      op.agent = "sam";
      op.fee = ASSET( "0.100000 SPHTX" );
      op.json_meta = "";
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );


      BOOST_TEST_MESSAGE( "--- falure when from cannot cover amount + fee" );
      op.steem_amount.amount = 10000000;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ratification deadline is in the past" );
      op.steem_amount.amount = 1000000;
      op.ratification_deadline = db->head_block_time() - 200;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when expiration is in the past" );
      op.escrow_expiration = db->head_block_time() - 100;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- success" );
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      auto alice_steem_balance = alice.balance - op.steem_amount - op.fee;
      auto bob_steem_balance = bob.balance;
      auto sam_steem_balance = sam.balance;

      db->push_transaction( tx, 0 );

      const auto& escrow = db->get_escrow( op.from, op.escrow_id );

      BOOST_REQUIRE( escrow.escrow_id == op.escrow_id );
      BOOST_REQUIRE( escrow.from == op.from );
      BOOST_REQUIRE( escrow.to == op.to );
      BOOST_REQUIRE( escrow.agent == op.agent );
      BOOST_REQUIRE( escrow.ratification_deadline == op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == op.escrow_expiration );
      BOOST_REQUIRE( escrow.steem_balance == op.steem_amount );
      BOOST_REQUIRE( escrow.pending_fee == op.fee );
      BOOST_REQUIRE( !escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );
      BOOST_REQUIRE( alice.balance == alice_steem_balance );
      BOOST_REQUIRE( bob.balance == bob_steem_balance );
      BOOST_REQUIRE( sam.balance == sam_steem_balance );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_approve_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_approve_validate" );

      escrow_approve_operation op;

      op.from = "alice";
      op.to = "bob";
      op.agent = "sam";
      op.who = "bob";
      op.escrow_id = 0;
      op.approve = true;

      BOOST_TEST_MESSAGE( "--- failure when who is not to or agent" );
      op.who = "dave";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- success when who is to" );
      op.who = op.to;
      op.validate();

      BOOST_TEST_MESSAGE( "--- success when who is agent" );
      op.who = op.agent;
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_approve_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_approve_authorities" );

      escrow_approve_operation op;

      op.from = "alice";
      op.to = "bob";
      op.agent = "sam";
      op.who = "bob";
      op.escrow_id = 0;
      op.approve = true;

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "bob" );
      BOOST_REQUIRE( auths == expected );

      expected.clear();
      auths.clear();

      op.who = "sam";
      op.get_required_active_authorities( auths );
      expected.insert( "sam" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_approve_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_approve_apply" );
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000000 );

      escrow_transfer_operation et_op;
      et_op.from = "alice";
      et_op.to = "bob";
      et_op.agent = "sam";
      et_op.steem_amount = ASSET( "1.000000 SPHTX" );
      et_op.fee = ASSET( "0.100000 SPHTX" );
      et_op.json_meta = "";
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;

      signed_transaction tx;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();


      BOOST_TEST_MESSAGE( "---failure when to does not match escrow" );
      escrow_approve_operation op;
      op.from = "alice";
      op.to = "dave";
      op.agent = "sam";
      op.who = "dave";
      op.approve = true;

      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when agent does not match escrow" );
      op.to = "bob";
      op.agent = "dave";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success approving to" );
      op.agent = "sam";
      op.who = "bob";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto& escrow = db->get_escrow( op.from, op.escrow_id );
      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.steem_balance == ASSET( "1.000000 SPHTX" ) );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.100000 SPHTX" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure on repeat approval" );
      tx.signatures.clear();

      tx.set_expiration( db->head_block_time() + STEEM_BLOCK_INTERVAL );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.steem_balance == ASSET( "1.000000 SPHTX" ) );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.100000 SPHTX" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure trying to repeal after approval" );
      tx.signatures.clear();
      tx.operations.clear();

      op.approve = false;

      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.steem_balance == ASSET( "1.000000 SPHTX" ) );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.100000 SPHTX" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- success refunding from because of repeal" );
      tx.signatures.clear();
      tx.operations.clear();

      op.who = op.agent;

      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      STEEM_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( alice.balance == ASSET( "10.000000 SPHTX" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- test automatic refund when escrow is not ratified before deadline" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( et_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( et_op.ratification_deadline + STEEM_BLOCK_INTERVAL, true );

      STEEM_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "10.000000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "10.010000 SPHTX" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- test ratification expiration when escrow is only approved by to" );
      tx.operations.clear();
      tx.signatures.clear();
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.to;
      op.approve = true;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( et_op.ratification_deadline + STEEM_BLOCK_INTERVAL, true );

      STEEM_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "10.000000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "10.010000 SPHTX" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- test ratification expiration when escrow is only approved by agent" );
      tx.operations.clear();
      tx.signatures.clear();
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( et_op.ratification_deadline + STEEM_BLOCK_INTERVAL, true );

      STEEM_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "10.000000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "10.010000 SPHTX" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success approving escrow" );
      tx.operations.clear();
      tx.signatures.clear();
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.to;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      {
         const auto& escrow = db->get_escrow( op.from, op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.steem_balance == ASSET( "1.000000 SPHTX" ) );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000000 SPHTX" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( !escrow.disputed );
      }

      BOOST_REQUIRE( db->get_account( "sam" ).balance == et_op.fee );
      validate_database();


      BOOST_TEST_MESSAGE( "--- ratification expiration does not remove an approved escrow" );

      generate_blocks( et_op.ratification_deadline + STEEM_BLOCK_INTERVAL, true );
      {
         const auto& escrow = db->get_escrow( op.from, op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.steem_balance == ASSET( "1.000000 SPHTX" ) );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000000 SPHTX" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( !escrow.disputed );
      }

      BOOST_REQUIRE( db->get_account( "sam" ).balance == et_op.fee );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_dispute_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_dispute_validate" );
      escrow_dispute_operation op;
      op.from = "alice";
      op.to = "bob";
      op.agent = "alice";
      op.who = "alice";

      BOOST_TEST_MESSAGE( "failure when who is not from or to" );
      op.who = "sam";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "success" );
      op.who = "alice";
      op.validate();

      op.who = "bob";
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_dispute_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_dispute_authorities" );
      escrow_dispute_operation op;
      op.from = "alice";
      op.to = "bob";
      op.who = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "alice" );
      BOOST_REQUIRE( auths == expected );

      auths.clear();
      expected.clear();
      op.who = "bob";
      op.get_required_active_authorities( auths );
      expected.insert( "bob" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_dispute_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_dispute_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000000 );

      escrow_transfer_operation et_op;
      et_op.from = "alice";
      et_op.to = "bob";
      et_op.agent = "sam";
      et_op.steem_amount = ASSET( "1.000000 SPHTX" );
      et_op.fee = ASSET( "0.100000 SPHTX" );
      et_op.ratification_deadline = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * STEEM_BLOCK_INTERVAL;

      escrow_approve_operation ea_b_op;
      ea_b_op.from = "alice";
      ea_b_op.to = "bob";
      ea_b_op.agent = "sam";
      ea_b_op.who = "bob";
      ea_b_op.approve = true;

      signed_transaction tx;
      tx.operations.push_back( et_op );
      tx.operations.push_back( ea_b_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- failure when escrow has not been approved" );
      escrow_dispute_operation op;
      op.from = "alice";
      op.to = "bob";
      op.agent = "sam";
      op.who = "bob";

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.steem_balance == et_op.steem_amount );
      BOOST_REQUIRE( escrow.pending_fee == et_op.fee );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure when to does not match escrow" );
      escrow_approve_operation ea_s_op;
      ea_s_op.from = "alice";
      ea_s_op.to = "bob";
      ea_s_op.agent = "sam";
      ea_s_op.who = "sam";
      ea_s_op.approve = true;

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( ea_s_op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.to = "dave";
      op.who = "alice";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.steem_balance == et_op.steem_amount );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000000 SPHTX" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure when agent does not match escrow" );
      op.to = "bob";
      op.who = "alice";
      op.agent = "dave";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.steem_balance == et_op.steem_amount );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000000 SPHTX" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure when escrow is expired" );
      generate_blocks( 2 );

      tx.operations.clear();
      tx.signatures.clear();
      op.agent = "sam";
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      {
         const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.steem_balance == et_op.steem_amount );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000000 SPHTX" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( !escrow.disputed );
      }


      BOOST_TEST_MESSAGE( "--- success disputing escrow" );
      et_op.escrow_id = 1;
      et_op.ratification_deadline = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * STEEM_BLOCK_INTERVAL;
      ea_b_op.escrow_id = et_op.escrow_id;
      ea_s_op.escrow_id = et_op.escrow_id;

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( et_op );
      tx.operations.push_back( ea_b_op );
      tx.operations.push_back( ea_s_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.escrow_id = et_op.escrow_id;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      {
         const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.steem_balance == et_op.steem_amount );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000000 SPHTX" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( escrow.disputed );
      }


      BOOST_TEST_MESSAGE( "--- failure when escrow is already under dispute" );
      tx.operations.clear();
      tx.signatures.clear();
      op.who = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      {
         const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.steem_balance == et_op.steem_amount );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000000 SPHTX" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( escrow.disputed );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_release_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow release validate" );
      escrow_release_operation op;
      op.from = "alice";
      op.to = "bob";
      op.who = "alice";
      op.agent = "sam";
      op.receiver = "bob";


      BOOST_TEST_MESSAGE( "--- failure when steem < 0" );
      op.steem_amount.amount = -1;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when steem == 0 and sbd == 0" );
      op.steem_amount.amount = 0;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when steem is not steem symbol" );
      op.steem_amount = ASSET( "1.000 SBD" );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "--- success" );
      op.steem_amount.symbol = STEEM_SYMBOL;
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_release_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_release_authorities" );
      escrow_release_operation op;
      op.from = "alice";
      op.to = "bob";
      op.who = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.who = "bob";
      auths.clear();
      expected.clear();
      expected.insert( "bob" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.who = "sam";
      auths.clear();
      expected.clear();
      expected.insert( "sam" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( escrow_release_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_release_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000000 );

      escrow_transfer_operation et_op;
      et_op.from = "alice";
      et_op.to = "bob";
      et_op.agent = "sam";
      et_op.steem_amount = ASSET( "1.000000 SPHTX" );
      et_op.fee = ASSET( "0.100000 SPHTX" );
      et_op.ratification_deadline = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * STEEM_BLOCK_INTERVAL;

      signed_transaction tx;
      tx.operations.push_back( et_op );

      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- failure releasing funds prior to approval" );
      escrow_release_operation op;
      op.from = et_op.from;
      op.to = et_op.to;
      op.agent = et_op.agent;
      op.who = et_op.from;
      op.receiver = et_op.to;
      op.steem_amount = ASSET( "0.100000 SPHTX" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      escrow_approve_operation ea_b_op;
      ea_b_op.from = "alice";
      ea_b_op.to = "bob";
      ea_b_op.agent = "sam";
      ea_b_op.who = "bob";

      escrow_approve_operation ea_s_op;
      ea_s_op.from = "alice";
      ea_s_op.to = "bob";
      ea_s_op.agent = "sam";
      ea_s_op.who = "sam";

      tx.clear();
      tx.operations.push_back( ea_b_op );
      tx.operations.push_back( ea_s_op );
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempts to release non-disputed escrow to 'to'" );
      op.who = et_op.agent;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when 'agent' attempts to release non-disputed escrow to 'from' " );
      op.receiver = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempt to release non-disputed escrow to not 'to' or 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when other attempts to release non-disputed escrow to 'to'" );
      op.receiver = et_op.to;
      op.who = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when other attempts to release non-disputed escrow to 'from' " );
      op.receiver = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when other attempt to release non-disputed escrow to not 'to' or 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attemtps to release non-disputed escrow to 'to'" );
      op.receiver = et_op.to;
      op.who = et_op.to;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when 'to' attempts to release non-dispured escrow to 'agent' " );
      op.receiver = et_op.agent;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release non-disputed escrow to not 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed escrow to 'to' from 'from'" );
      op.receiver = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_escrow( op.from, op.escrow_id ).steem_balance == ASSET( "0.900000 SPHTX" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.000000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed escrow to 'from'" );
      op.receiver = et_op.from;
      op.who = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release non-disputed escrow to 'agent'" );
      op.receiver = et_op.agent;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed escrow to not 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed escrow to 'from' from 'to'" );
      op.receiver = et_op.to;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_escrow( op.from, op.escrow_id ).steem_balance == ASSET( "0.800000 SPHTX" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.100000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- failure when releasing more sbd than available" );
      op.steem_amount = ASSET( "1.000000 SPHTX" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when releasing less steem than available" );
      op.steem_amount = ASSET( "0.000000 SPHTX" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release disputed escrow" );
      escrow_dispute_operation ed_op;
      ed_op.from = "alice";
      ed_op.to = "bob";
      ed_op.agent = "sam";
      ed_op.who = "alice";

      tx.clear();
      tx.operations.push_back( ed_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.clear();
      op.from = et_op.from;
      op.receiver = et_op.from;
      op.who = et_op.to;
      op.steem_amount = ASSET( "0.100000 SPHTX" );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release disputed escrow" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.from;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when releasing disputed escrow to an account not 'to' or 'from'" );
      tx.clear();
      op.who = et_op.agent;
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when agent does not match escrow" );
      tx.clear();
      op.who = "dave";
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success releasing disputed escrow with agent to 'to'" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.200000 SPHTX" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).steem_balance == ASSET( "0.700000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- success releasing disputed escrow with agent to 'from'" );
      tx.clear();
      op.receiver = et_op.from;
      op.who = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.100000 SPHTX" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).steem_balance == ASSET( "0.600000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release disputed expired escrow" );
      generate_blocks( 2 );

      tx.clear();
      op.receiver = et_op.from;
      op.who = et_op.to;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release disputed expired escrow" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.from;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success releasing disputed expired escrow with agent" );
      tx.clear();
      op.receiver = et_op.from;
      op.who = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "9.200000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "9.210000 SPHTX" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).steem_balance == ASSET( "0.500000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- success deleting escrow when balances are both zero" );
      tx.clear();
      op.steem_amount = ASSET( "0.500000 SPHTX" );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "9.700000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "9.710000 SPHTX" ) );
      STEEM_REQUIRE_THROW( db->get_escrow( et_op.from, et_op.escrow_id ), fc::exception );


      tx.clear();
      et_op.ratification_deadline = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * STEEM_BLOCK_INTERVAL;
      tx.operations.push_back( et_op );
      tx.operations.push_back( ea_b_op );
      tx.operations.push_back( ea_s_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_blocks( 2 );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempts to release non-disputed expired escrow to 'to'" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.agent;
      op.steem_amount = ASSET( "0.100000 SPHTX" );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempts to release non-disputed expired escrow to 'from'" );
      tx.clear();
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempt to release non-disputed expired escrow to not 'to' or 'from'" );
      tx.clear();
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release non-dispured expired escrow to 'agent'" );
      tx.clear();
      op.who = et_op.to;
      op.receiver = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release non-disputed expired escrow to not 'from' or 'to'" );
      tx.clear();
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'to' from 'to'" );
      tx.clear();
      op.receiver = et_op.to;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).balance >= ASSET( "0.300000 SPHTX" ) && db->get_account( "bob" ).balance < ASSET( "0.310000 SPHTX" ));
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).steem_balance == ASSET( "0.900000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'from' from 'to'" );
      tx.clear();
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "8.700000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "8.710000 SPHTX" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).steem_balance == ASSET( "0.800000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed expired escrow to 'agent'" );
      tx.clear();
      op.who = et_op.from;
      op.receiver = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed expired escrow to not 'from' or 'to'" );
      tx.clear();
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'to' from 'from'" );
      tx.clear();
      op.receiver = et_op.to;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).balance >= ASSET( "0.400000 SPHTX" ) && db->get_account( "bob" ).balance < ASSET( "0.410000 SPHTX" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).steem_balance == ASSET( "0.700000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'from' from 'from'" );
      tx.clear();
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "8.800000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "8.810000 SPHTX" ));
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).steem_balance == ASSET( "0.600000 SPHTX" ) );


      BOOST_TEST_MESSAGE( "--- success deleting escrow when balances are zero on non-disputed escrow" );
      tx.clear();
      op.steem_amount = ASSET( "0.600000 SPHTX" );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance >= ASSET( "9.400000 SPHTX" ) && db->get_account( "alice" ).balance < ASSET( "9.410000 SPHTX" ));
      STEEM_REQUIRE_THROW( db->get_escrow( et_op.from, et_op.escrow_id ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE( witness_set_properties_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_set_properties_validate" );

      ACTORS( (alice) )
      fund( "alice", 10000000 );
      private_key_type signing_key = generate_private_key( "old_key" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.fee = ASSET( "1.000000 SPHTX" );
      op.block_signing_key = signing_key.get_public_key();
      op.props.account_creation_fee = asset(STEEM_MIN_ACCOUNT_CREATION_FEE + 10, STEEM_SYMBOL) ;
      op.props.maximum_block_size = STEEM_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();

      BOOST_TEST_MESSAGE( "--- failure when signing key is not present" );
      witness_set_properties_operation prop_op;
      prop_op.owner = "alice";
      STEEM_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting account_creation_fee with incorrect symbol" );
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
      prop_op.props[ "account_creation_fee" ] = fc::raw::pack_to_vector( ASSET( "2.000 SBD" ) );
      STEEM_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting maximum_block_size below STEEM_MIN_BLOCK_SIZE_LIMIT" );
      prop_op.props.erase( "account_creation_fee" );
      prop_op.props[ "maximum_block_size" ] = fc::raw::pack_to_vector( STEEM_MIN_BLOCK_SIZE_LIMIT - 1 );
      STEEM_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting new sbd_exchange_rate with SBD / STEEM" );
      prop_op.props.erase( "sbd_interest_rate" );
      prop_op.props[ "sbd_exchange_rate" ] = fc::raw::pack_to_vector( price( ASSET( "1.000 SPHTX" ), ASSET( "10.000 SBD" ) ) );
      STEEM_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting new url with length of zero" );
      prop_op.props.erase( "sbd_exchange_rate" );
      prop_op.props[ "url" ] = fc::raw::pack_to_vector( "" );
      STEEM_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting new url with non UTF-8 character" );
      prop_op.props[ "url" ].clear();
      prop_op.props[ "url" ] = fc::raw::pack_to_vector( "\xE0\x80\x80" );
      STEEM_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( witness_set_properties_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_set_properties_authorities" );

      witness_set_properties_operation op;
      op.owner = "alice";
      op.props[ "key" ] = fc::raw::pack_to_vector( generate_private_key( "key" ).get_public_key() );

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      vector< authority > key_auths;
      vector< authority > expected_keys;
      expected_keys.push_back( authority( 1, generate_private_key( "key" ).get_public_key(), 1 ) );
      op.get_required_authorities( key_auths );
      BOOST_REQUIRE( key_auths == expected_keys );

      op.props.erase( "key" );
      key_auths.clear();
      expected_keys.clear();

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected_keys.push_back( authority( 1, STEEM_NULL_ACCOUNT, 1 ) );
      op.get_required_authorities( key_auths );
      BOOST_REQUIRE( key_auths == expected_keys );

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( witness_set_properties_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_set_properties_apply" );

      ACTORS( (alice) )
      fund( "alice", 10000000 );
      private_key_type signing_key = generate_private_key( "old_key" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.fee = ASSET( "1.000 SPHTX" );
      op.block_signing_key = signing_key.get_public_key();
      op.props.account_creation_fee = asset(STEEM_MIN_ACCOUNT_CREATION_FEE + 10, STEEM_SYMBOL) ;
      op.props.maximum_block_size = STEEM_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test setting runtime parameters" );

      // Setting account_creation_fee
      const witness_object& alice_witness = db->get_witness( "alice" );
      witness_set_properties_operation prop_op;
      prop_op.owner = "alice";
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
      prop_op.props[ "account_creation_fee" ] = fc::raw::pack_to_vector( ASSET( "2.000 SPHTX" ) );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.props.account_creation_fee == ASSET( "2.000 SPHTX" ) );

      // Setting maximum_block_size
      prop_op.props.erase( "account_creation_fee" );
      prop_op.props[ "maximum_block_size" ] = fc::raw::pack_to_vector( STEEM_MIN_BLOCK_SIZE_LIMIT + 1 );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.props.maximum_block_size == STEEM_MIN_BLOCK_SIZE_LIMIT + 1 );

      // Setting new sbd_exchange_rate
      prop_op.props.erase( "new_signing_key" );
      prop_op.props[ "key" ].clear();
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
      prop_op.props[ "sbd_exchange_rate" ] = fc::raw::pack_to_vector( price( ASSET(" 1.000000 SBD" ), ASSET( "100.000000 SPHTX" ) ) );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.sbd_exchange_rate == price( ASSET( "1.000000 SBD" ), ASSET( "100.000000 SPHTX" ) ) );
      BOOST_REQUIRE( alice_witness.last_sbd_exchange_update == db->head_block_time() );

      // Setting new url
      prop_op.props.erase( "sbd_exchange_rate" );
      prop_op.props[ "url" ] = fc::raw::pack_to_vector( "foo.bar" );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.url == "foo.bar" );

      // Setting new extranious_property
      prop_op.props.erase( "sbd_exchange_rate" );
      prop_op.props[ "extraneous_property" ] = fc::raw::pack_to_vector( "foo" );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()
//#endif
