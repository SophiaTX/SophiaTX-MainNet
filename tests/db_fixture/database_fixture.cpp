#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <sophiatx/utilities/tempdir.hpp>

#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/history_object.hpp>
#include <sophiatx/plugins/account_history/account_history_plugin.hpp>
#include <sophiatx/plugins/witness/witness_plugin.hpp>
#include <sophiatx/plugins/chain/chain_plugin_full.hpp>
#include <sophiatx/plugins/webserver/webserver_plugin.hpp>
#include <sophiatx/plugins/witness_api/witness_api_plugin.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_plugin.hpp>

#include <fc/crypto/digest.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

uint32_t SOPHIATX_TESTING_GENESIS_TIMESTAMP = 1431700000;

using namespace sophiatx::plugins::webserver;
using namespace sophiatx::plugins::database_api;
using namespace sophiatx::plugins::block_api;

#define DUMP( x ) {fc::variant vo; fc::to_variant( x , vo); std::cout<< fc::json::to_string(vo) <<"\n";}

namespace sophiatx { namespace chain {

using std::cout;
using std::cerr;

clean_database_fixture::clean_database_fixture()
{
   try {
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

   appbase::app().register_plugin<sophiatx::plugins::chain::chain_plugin_full>();
   appbase::app().register_plugin< sophiatx::plugins::account_history::account_history_plugin >();
   db_plugin = &appbase::app().register_plugin< sophiatx::plugins::debug_node::debug_node_plugin >();
   appbase::app().register_plugin< sophiatx::plugins::witness::witness_plugin >();

   db_plugin->logging = false;
   appbase::app().initialize<
      sophiatx::plugins::chain::chain_plugin_full,
      sophiatx::plugins::account_history::account_history_plugin,
      sophiatx::plugins::debug_node::debug_node_plugin,
      sophiatx::plugins::witness::witness_plugin
      >( argc, argv );

   db = std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db());
   BOOST_REQUIRE( db );

   init_account_pub_key = init_account_priv_key.get_public_key();

   open_database();
   db->modify( db->get_witness( "initminer" ), [&]( witness_object& a )
   {
        a.signing_key = init_account_pub_key;
   });
   db->modify( db->get< account_authority_object, by_account >( "initminer" ), [&]( account_authority_object& a )
   {
        a.active.add_authority(init_account_pub_key, 1);
        a.owner.add_authority(init_account_pub_key, 1);
   });

   generate_block();
   db->set_hardfork( SOPHIATX_BLOCKCHAIN_VERSION.get_minor() );
   generate_block();


   vest( "initminer", SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );

   validate_database();

    // Fill up the rest of the required miners
   for( int i = SOPHIATX_NUM_INIT_MINERS; i < SOPHIATX_MAX_WITNESSES; i++ )
   {
      account_create( SOPHIATX_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
      fund( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
      vest( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
      witness_create( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), init_account_priv_key, "foo.bar", init_account_pub_key, 0 );
   }

   validate_database();

   } catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
      throw;
   }

   return;
}

clean_database_fixture::~clean_database_fixture()
{ try {
   // If we're unwinding due to an exception, don't do any more checks.
   // This way, boost test's last checkpoint tells us approximately where the error was.
   if( !std::uncaught_exception() )
   {
      BOOST_CHECK( db->node_properties().skip_flags == database_interface::skip_nothing );
   }

   if( data_dir )
      db->wipe( data_dir->path(), true );
   return;
} FC_CAPTURE_AND_LOG( (data_dir->path()) )
   exit(1);
}

void clean_database_fixture::resize_shared_mem( uint64_t size )
{
   db->wipe( data_dir->path(), true );
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
   init_account_pub_key = init_account_priv_key.get_public_key();

   {
      genesis_state_type gen;
      gen.genesis_time = fc::time_point::now();
      database_interface::open_args args;
      args.shared_mem_dir = data_dir->path();
      args.shared_file_size = size;
      db->open( args, gen, public_key_type(SOPHIATX_INIT_PUBLIC_KEY_STR) );
   }
   db->modify( db->get_witness( "initminer" ), [&]( witness_object& a )
   {
        a.signing_key = init_account_pub_key;
   });
   db->modify( db->get< account_authority_object, by_account >( "initminer" ), [&]( account_authority_object& a )
   {
        a.active.add_authority(init_account_pub_key, 1);
        a.owner.add_authority(init_account_pub_key, 1);
   });

   boost::program_options::variables_map options;


   generate_block();
   db->set_hardfork( SOPHIATX_BLOCKCHAIN_VERSION.get_minor() );
   generate_block();

   vest( "initminer", SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );

   // Fill up the rest of the required miners
   for( int i = SOPHIATX_NUM_INIT_MINERS; i < SOPHIATX_MAX_WITNESSES; i++ )
   {
      account_create( SOPHIATX_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
      fund( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
      vest( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
      witness_create( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), init_account_priv_key, "foo.bar", init_account_pub_key, 0 );
   }

   validate_database();
}

private_database_fixture::private_database_fixture()
{
   try {
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

      appbase::app().register_plugin<sophiatx::plugins::chain::chain_plugin_full>();
      appbase::app().register_plugin< sophiatx::plugins::account_history::account_history_plugin >();
      db_plugin = &appbase::app().register_plugin< sophiatx::plugins::debug_node::debug_node_plugin >();
      appbase::app().register_plugin< sophiatx::plugins::witness::witness_plugin >();

      db_plugin->logging = false;
      appbase::app().initialize<
            sophiatx::plugins::chain::chain_plugin_full,
            sophiatx::plugins::account_history::account_history_plugin,
            sophiatx::plugins::debug_node::debug_node_plugin,
            sophiatx::plugins::witness::witness_plugin
      >( argc, argv );

      db = std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db());
      BOOST_REQUIRE( db );

      init_account_pub_key = init_account_priv_key.get_public_key();

      open_database_private();
      db->modify( db->get_witness( "initminer" ), [&]( witness_object& a )
      {
           a.signing_key = init_account_pub_key;
      });
      db->modify( db->get< account_authority_object, by_account >( "initminer" ), [&]( account_authority_object& a )
      {
           a.active.add_authority(init_account_pub_key, 1);
           a.owner.add_authority(init_account_pub_key, 1);
      });

      db->modify( db->get_witness_schedule_object(), [&]( witness_schedule_object& wso )
      {
           wso.median_props.account_creation_fee = ASSET( "0.000000 SPHTX" );
      });

      generate_block();
      db->set_hardfork( SOPHIATX_BLOCKCHAIN_VERSION.get_minor() );
      generate_block();

      validate_database();

//      // Fill up the rest of the required miners
//      for( int i = SOPHIATX_NUM_INIT_MINERS; i < SOPHIATX_MAX_WITNESSES; i++ )
//      {
//         account_create( SOPHIATX_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
//         fund( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
//         vest( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
//         witness_create( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), init_account_priv_key, "foo.bar", init_account_pub_key, 0 );
//      }

//      validate_database();

   } catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
      throw;
   }

   return;
}

private_database_fixture::~private_database_fixture()
{
   try {
      // If we're unwinding due to an exception, don't do any more checks.
      // This way, boost test's last checkpoint tells us approximately where the error was.
      if( !std::uncaught_exception() )
      {
         BOOST_CHECK( db->node_properties().skip_flags == database_interface::skip_nothing );
      }

      if( data_dir )
         db->wipe( data_dir->path(), true );
      return;
   } FC_CAPTURE_AND_LOG( (data_dir->path()) )
   exit(1);
}

live_database_fixture::live_database_fixture()
{
   try
   {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;

      ilog( "Loading saved chain" );
      _chain_dir = fc::current_path() / "test_blockchain";
      FC_ASSERT( fc::exists( _chain_dir ), "Requires blockchain to test on in ./test_blockchain" );

      appbase::app().register_plugin<sophiatx::plugins::chain::chain_plugin_full>();
      appbase::app().register_plugin< sophiatx::plugins::account_history::account_history_plugin >();
      db_plugin = &appbase::app().register_plugin< sophiatx::plugins::debug_node::debug_node_plugin >();

      appbase::app().initialize<
         sophiatx::plugins::chain::chain_plugin_full,
         sophiatx::plugins::account_history::account_history_plugin, sophiatx::plugins::debug_node::debug_node_plugin
         >( argc, argv );

      db = std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db());
      BOOST_REQUIRE( db );

      {
         genesis_state_type gen;
         gen.genesis_time = fc::time_point::now();
         database_interface::open_args args;
         args.shared_mem_dir = _chain_dir;
         db->open( args, gen, public_key_type(SOPHIATX_INIT_PUBLIC_KEY_STR) );
      }

      validate_database();
      generate_block();

      ilog( "Done loading saved chain" );
   }
      FC_LOG_AND_RETHROW()
}

live_database_fixture::~live_database_fixture()
{
   try
   {
      // If we're unwinding due to an exception, don't do any more checks.
      // This way, boost test's last checkpoint tells us approximately where the error was.
      if( !std::uncaught_exception() )
      {
         BOOST_CHECK( db->node_properties().skip_flags == database_interface::skip_nothing );
      }

      db->pop_block();
      db->close();
      return;
   }
   FC_CAPTURE_AND_LOG( (data_dir->path()) )
   exit(1);
}

fc::ecc::private_key database_fixture::generate_private_key(string seed)
{
   static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
   if( seed == "init_key" )
      return committee;
   return fc::ecc::private_key::regenerate( fc::sha256::hash( seed ) );
}

asset_symbol_type database_fixture::name_to_asset_symbol( const std::string& name, uint8_t decimal_places )
{
   // Deterministically turn a name into an asset symbol
   // Example:
   // alice -> sha256(alice) -> 2bd806c9... -> 2bd806c9 -> low 27 bits is 64489161 -> add check digit -> @@644891612

   return asset_symbol_type::from_string(name);

}

void database_fixture::open_database()
{
   if( !data_dir )
   {
      data_dir = fc::temp_directory( sophiatx::utilities::temp_directory_path() );
      db->_log_hardforks = false;

      genesis_state_type gen;
      gen.genesis_time = fc::time_point::now();

      database_interface::open_args args;
      args.shared_mem_dir = data_dir->path();
      args.shared_file_size = 1024 * 1024 * 256;     // 8MB file for testing
      db->open(args, gen, public_key_type(SOPHIATX_INIT_PUBLIC_KEY_STR));
   }
}

void database_fixture::open_database_private()
{
   if( !data_dir )
   {
      data_dir = fc::temp_directory( sophiatx::utilities::temp_directory_path() );
      db->_log_hardforks = false;

      genesis_state_type gen;
      gen.genesis_time = fc::time_point::now();
      gen.initial_balace = 0;
      gen.initial_public_key = public_key_type(SOPHIATX_INIT_PUBLIC_KEY_STR);
      gen.is_private_net = true;

      database_interface::open_args args;
      args.shared_mem_dir = data_dir->path();
      args.shared_file_size = 1024 * 1024 * 256;     // 8MB file for testing
      db->open(args, gen, public_key_type(SOPHIATX_INIT_PUBLIC_KEY_STR));
   }
}

void database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
   skip |= default_skip;
   db_plugin->debug_generate_blocks( sophiatx::utilities::key_to_wif( key ), 1, skip, miss_blocks );
}

void database_fixture::generate_blocks( uint32_t block_count )
{
   auto produced = db_plugin->debug_generate_blocks( debug_key, block_count, default_skip, 0 );
   BOOST_REQUIRE( produced == block_count );
}

void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks)
{
   db_plugin->debug_generate_blocks_until( debug_key, timestamp, miss_intermediate_blocks, default_skip );
   BOOST_REQUIRE( ( db->head_block_time() - timestamp ).to_seconds() < SOPHIATX_BLOCK_INTERVAL );
}

const account_object& database_fixture::account_create(
   const string& name,
   const string& creator,
   const private_key_type& creator_key,
   const share_type& fee,
   const public_key_type& key,
   const string& json_metadata
   )
{
   try
   {
      account_create_operation op;
      op.name_seed = name;
      op.creator = creator;
      op.fee = asset( fee, SOPHIATX_SYMBOL );
      op.owner = authority( 1, key, 1 );
      op.active = authority( 1, key, 1 );
      op.memo_key = key;
      op.json_metadata = json_metadata;

      trx.operations.push_back( op );

      trx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      sign( trx, creator_key );
      trx.validate();
      db->push_transaction( trx, 0 );
      trx.operations.clear();
      trx.signatures.clear();

      const account_object& acct = db->get_account( AN(name) );

      return acct;
   }
   FC_CAPTURE_AND_RETHROW( (name)(creator) )
}

const account_object& database_fixture::account_create(
   const string& name,
   const public_key_type& key
)
{
   try
   {
      return account_create(
         name,
         SOPHIATX_INIT_MINER_NAME,
         init_account_priv_key,
         std::max( db->get_witness_schedule_object().median_props.account_creation_fee.amount, share_type( 0 ) ),
         key,
         "" );
   }
   FC_CAPTURE_AND_RETHROW( (name) );
}


const witness_object& database_fixture::witness_create(
   const string& owner,
   const private_key_type& owner_key,
   const string& url,
   const public_key_type& signing_key,
   const share_type& fee )
{
   try
   {
      witness_update_operation op;
      op.owner = owner;
      op.url = url;
      op.block_signing_key = signing_key;
      op.fee = asset( fee, SOPHIATX_SYMBOL );

      trx.operations.push_back( op );
      trx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      sign( trx, owner_key );
      trx.validate();
      db->push_transaction( trx, 0 );
      trx.operations.clear();
      trx.signatures.clear();

      return db->get_witness( owner );
   }
   FC_CAPTURE_AND_RETHROW( (owner)(url) )
}

void database_fixture::fund(
   const string& account_name,
   const share_type& amount
   )
{
   try
   {
      transfer( SOPHIATX_INIT_MINER_NAME, account_name, asset( amount, SOPHIATX_SYMBOL ) );

   } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::fund(
   const string& account_name,
   const asset& amount
   )
{
   try
   {
      db_plugin->debug_update( [=]( std::shared_ptr<database>& db)
      {
         db->modify( db->get_account( account_name ), [&]( account_object& a )
         {
            if( amount.symbol == SOPHIATX_SYMBOL )
               a.balance += amount;
         });

         db->modify( db->get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            if( amount.symbol == SOPHIATX_SYMBOL )
               gpo.current_supply += amount;

         });


      }, default_skip );
   }
   FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::transfer(
   const string& from,
   const string& to,
   const asset& amount )
{
   try
   {
      transfer_operation op;
      op.from = from;
      op.to = to;
      op.amount = amount;
      op.fee = asset(100000, SOPHIATX_SYMBOL);

      trx.operations.push_back( op );
      trx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      trx.validate();
      db->push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount) )
}

void database_fixture::vest( const string& account_name, const share_type& amount )
{

   vest(account_name, asset(amount, SOPHIATX_SYMBOL));
}

void database_fixture::vest( const string& account_name, const asset& amount )
{
   if( amount.symbol != SOPHIATX_SYMBOL )
      return;

   try
   {
      transfer_to_vesting_operation op;
      op.from = account_name;
      op.to = account_name;
      op.amount = amount;
      trx.operations.push_back( op );
      trx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      trx.validate();

      db->push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::proxy( const string& account, const string& proxy )
{
   try
   {
      account_witness_proxy_operation op;
      op.account = account;
      op.proxy = proxy;
      trx.operations.push_back( op );
      trx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );

      db->push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (account)(proxy) )
}

void database_fixture::set_price_feed( const price& new_price )
{
   flat_map< string, vector< char > > props;
   vector<price> new_prices;
   new_prices.push_back(new_price);
   props[ "exchange_rates" ] = fc::raw::pack_to_vector( new_prices );

   set_witness_props( props );

   BOOST_REQUIRE(
      db->get(feed_history_id_type()).current_median_history == new_price
   );
}

void database_fixture::set_witness_props( const flat_map< string, vector< char > >& props )
{
   for( size_t i = 1; i < 8; i++ )
   {
      witness_set_properties_operation op;
      op.owner = AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i ));
      op.props = props;

      if( op.props.find( "key" ) == op.props.end() )
      {
         op.props[ "key" ] = fc::raw::pack_to_vector( init_account_pub_key );
      }

      trx.operations.push_back( op );
      trx.set_expiration( db->head_block_time() + SOPHIATX_MAX_TIME_UNTIL_EXPIRATION );
      db->push_transaction( trx, ~0 );
      trx.operations.clear();
   }

   generate_blocks( SOPHIATX_BLOCKS_PER_HOUR );
}

const asset& database_fixture::get_balance( const string& account_name )const
{
  return db->get_account( AN(account_name )).balance;
}

void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
   trx.sign( key, db->get_chain_id(), default_sig_canon );
}

vector< operation > database_fixture::get_last_operations( uint32_t num_ops )
{
   vector< operation > ops;
   const auto& acc_hist_idx = db->get_index< account_history_index >().indices().get< by_id >();
   auto itr = acc_hist_idx.end();

   while( itr != acc_hist_idx.begin() && ops.size() < num_ops )
   {
      itr--;
      const buffer_type& bip_serialized_op = db->get(itr->op).serialized_op;
      std::vector<char> serialized_op;
      serialized_op.reserve( bip_serialized_op.size() );
      std::copy( bip_serialized_op.begin(), bip_serialized_op.end(), std::back_inserter( serialized_op ) );
      ops.push_back( fc::raw::unpack_from_vector< sophiatx::chain::operation >( serialized_op, 0 ) );
   }

   return ops;
}

vector< operation > database_fixture::get_last_operations( uint32_t num_ops, string account_name )
{
   vector< operation > ops;
   const auto& acc_hist_idx = db->get_index< account_history_index >().indices().get< by_account >();


   auto itr = acc_hist_idx.lower_bound(boost::make_tuple(account_name, 100000000 ));
   auto end = acc_hist_idx.upper_bound(boost::make_tuple(account_name, 0 ));

   while( itr != end && ops.size() < num_ops && itr->account == account_name )
   {
      const buffer_type& bip_serialized_op = db->get(itr->op).serialized_op;
      std::vector<char> serialized_op;
      serialized_op.reserve( bip_serialized_op.size() );
      std::copy( bip_serialized_op.begin(), bip_serialized_op.end(), std::back_inserter( serialized_op ) );
      ops.push_back( fc::raw::unpack_from_vector< sophiatx::chain::operation >( serialized_op, 0 ) );
      itr++;
   }

   return ops;
}

void database_fixture::validate_database( void )
{
   try
   {
      db->validate_invariants();
   }
   FC_LOG_AND_RETHROW();
}

json_rpc_database_fixture::json_rpc_database_fixture()
{
   try {
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

   appbase::app().register_plugin<sophiatx::plugins::chain::chain_plugin_full>();
   appbase::app().register_plugin< sophiatx::plugins::account_history::account_history_plugin >();
   db_plugin = &appbase::app().register_plugin< sophiatx::plugins::debug_node::debug_node_plugin >();
   appbase::app().register_plugin< sophiatx::plugins::witness::witness_plugin >();
   rpc_plugin = &appbase::app().register_plugin< sophiatx::plugins::json_rpc::json_rpc_plugin >();
   appbase::app().register_plugin< sophiatx::plugins::block_api::block_api_plugin >();
   appbase::app().register_plugin< sophiatx::plugins::database_api::database_api_plugin >();
   appbase::app().register_plugin< sophiatx::plugins::witness::witness_api_plugin >();
   appbase::app().register_plugin< sophiatx::plugins::alexandria_api::alexandria_api_plugin >();

   db_plugin->logging = false;
   appbase::app().initialize<
      sophiatx::plugins::chain::chain_plugin_full,
      sophiatx::plugins::account_history::account_history_plugin,
      sophiatx::plugins::debug_node::debug_node_plugin,
      sophiatx::plugins::witness::witness_plugin,
      sophiatx::plugins::json_rpc::json_rpc_plugin,
      sophiatx::plugins::block_api::block_api_plugin,
      sophiatx::plugins::database_api::database_api_plugin,
      sophiatx::plugins::witness::witness_api_plugin,
      sophiatx::plugins::alexandria_api::alexandria_api_plugin
      >( argc, argv );


   appbase::app().get_plugin< sophiatx::plugins::alexandria_api::alexandria_api_plugin >().plugin_startup();

   db = std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db());
   BOOST_REQUIRE( db );

   init_account_pub_key = init_account_priv_key.get_public_key();

   open_database();

   db->modify( db->get_witness( "initminer" ), [&]( witness_object& a )
   {
        a.signing_key = init_account_pub_key;
   });
   db->modify( db->get< account_authority_object, by_account >( "initminer" ), [&]( account_authority_object& a )
   {
        a.active.add_authority(init_account_pub_key, 1);
        a.owner.add_authority(init_account_pub_key, 1);
   });
   generate_block();
   db->set_hardfork( SOPHIATX_BLOCKCHAIN_VERSION.get_minor() );
   generate_block();

   vest( "initminer", SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );

   // Fill up the rest of the required miners
   for( int i = SOPHIATX_NUM_INIT_MINERS; i < SOPHIATX_MAX_WITNESSES; i++ )
   {
      account_create( SOPHIATX_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
      fund( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
      vest( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE );
      witness_create( AN(SOPHIATX_INIT_MINER_NAME + fc::to_string( i )), init_account_priv_key, "foo.bar", init_account_pub_key, 0  );
   }

   validate_database();
   } catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
      throw;
   }

   return;
}

json_rpc_database_fixture::~json_rpc_database_fixture()
{ try {
   // If we're unwinding due to an exception, don't do any more checks.
   // This way, boost test's last checkpoint tells us approximately where the error was.
   if( !std::uncaught_exception() )
   {
      BOOST_CHECK( db->node_properties().skip_flags == database_interface::skip_nothing );
   }

   if( data_dir )
      db->wipe( data_dir->path(), true );
   return;
} FC_CAPTURE_AND_LOG( (data_dir->path()) )
   exit(1);
}

fc::variant json_rpc_database_fixture::get_answer( std::string& request )
{
   bool is_error = false;
   return fc::json::from_string( rpc_plugin->call( request, is_error ) );
}

void check_id_equal( const fc::variant& id_a, const fc::variant& id_b )
{
   BOOST_REQUIRE( id_a.get_type() == id_b.get_type() );

   switch( id_a.get_type() )
   {
      case fc::variant::int64_type:
         BOOST_REQUIRE( id_a.as_int64() == id_b.as_int64() );
         break;
      case fc::variant::uint64_type:
         BOOST_REQUIRE( id_a.as_uint64() == id_b.as_uint64() );
         break;
      case fc::variant::string_type:
         BOOST_REQUIRE( id_a.as_string() == id_b.as_string() );
         break;
      case fc::variant::null_type:
         break;
      default:
         BOOST_REQUIRE( false );
   }
}

void json_rpc_database_fixture::review_answer( fc::variant& answer, int64_t code, bool is_warning, bool is_fail, fc::optional< fc::variant > id )
{
   fc::variant_object error;
   int64_t answer_code;

   if( is_fail )
   {
      if( id.valid() && code != JSON_RPC_INVALID_REQUEST )
      {
         BOOST_REQUIRE( answer.get_object().contains( "id" ) );
         check_id_equal( answer[ "id" ], *id );
      }

      BOOST_REQUIRE( answer.get_object().contains( "error" ) );
      BOOST_REQUIRE( answer["error"].is_object() );
      error = answer["error"].get_object();
      BOOST_REQUIRE( error.contains( "code" ) );
      BOOST_REQUIRE( error["code"].is_int64() );
      answer_code = error["code"].as_int64();
      BOOST_REQUIRE( answer_code == code );
      if( is_warning )
         BOOST_TEST_MESSAGE( error["message"].as_string() );
   }
   else
   {
      BOOST_REQUIRE( answer.get_object().contains( "result" ) );
      BOOST_REQUIRE( answer.get_object().contains( "id" ) );
      if( id.valid() )
         check_id_equal( answer[ "id" ], *id );
   }
}

void json_rpc_database_fixture::make_array_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
{
   fc::variant answer = get_answer( request );
   BOOST_REQUIRE( answer.is_array() );

   fc::variants request_array = fc::json::from_string( request ).get_array();
   fc::variants array = answer.get_array();

   BOOST_REQUIRE( array.size() == request_array.size() );
   for( size_t i = 0; i < array.size(); ++i )
   {
      fc::optional< fc::variant > id;

      try
      {
         id = request_array[i][ "id" ];
      }
      catch( ... ) {}

      review_answer( array[i], code, is_warning, is_fail, id );
   }
}

fc::variant json_rpc_database_fixture::make_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
{
   fc::variant answer = get_answer( request );
   BOOST_REQUIRE( answer.is_object() );
   fc::optional< fc::variant > id;

   try
   {
      id = fc::json::from_string( request ).get_object()[ "id" ];
   }
   catch( ... ) {}

   review_answer( answer, code, is_warning, is_fail, id );

   return answer;
}

void json_rpc_database_fixture::make_positive_request( std::string& request )
{
   make_request( request, 0/*code*/, false/*is_warning*/, false/*is_fail*/);
}

namespace test {

bool _push_block( const std::shared_ptr<database>& db, const signed_block& b, uint32_t skip_flags /* = 0 */ )
{
   return db->push_block( b, skip_flags);
}

void _push_transaction( const std::shared_ptr<database>& db, const signed_transaction& tx, uint32_t skip_flags /* = 0 */ )
{ try {
   db->push_transaction( tx, skip_flags );
} FC_CAPTURE_AND_RETHROW((tx)) }

} // sophiatx::chain::test

} } // sophiatx::chain
