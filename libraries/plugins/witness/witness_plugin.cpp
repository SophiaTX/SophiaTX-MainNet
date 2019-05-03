#include <sophiatx/plugins/witness/witness_plugin.hpp>
#include <sophiatx/plugins/witness/witness_objects.hpp>

#include <sophiatx/chain/database/database_exceptions.hpp>
#include <sophiatx/chain/database/database.hpp>
#include <sophiatx/chain/account_object.hpp>
#include <sophiatx/chain/witness_objects.hpp>
#include <sophiatx/chain/index.hpp>

#include <sophiatx/utilities/key_conversion.hpp>
#include <sophiatx/utilities/plugin_utilities.hpp>

#include <fc/io/json.hpp>
#include <fc/macros.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>


#define DISTANCE_CALC_PRECISION (10000)
#define BLOCK_PRODUCING_LAG_TIME (750)
#define BLOCK_PRODUCTION_LOOP_SLEEP_TIME (200000)


namespace sophiatx { namespace plugins { namespace witness {

using chain::plugin_exception;
using std::string;
using std::vector;

namespace bpo = boost::program_options;


void new_chain_banner()
{
   std::cerr << "\n"
      "********************************\n"
      "*                              *\n"
      "*   ------- NEW CHAIN ------   *\n"
      "*   - Welcome to SophiaTX! -   *\n"
      "*   ------------------------   *\n"
      "*                              *\n"
      "********************************\n"
      "\n";
   return;
}

namespace detail {

   class witness_plugin_impl {
   public:
      witness_plugin_impl( boost::asio::io_service& io ) :
         _timer(io),
         _db( std::static_pointer_cast<chain::database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db()) ),
         _chain_plugin( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >()) {}

      void pre_transaction( const sophiatx::protocol::signed_transaction& trx );
      void pre_operation( const chain::operation_notification& note );
      void on_block( const signed_block& b );

      void schedule_production_loop();
      block_production_condition::block_production_condition_enum block_production_loop();
      block_production_condition::block_production_condition_enum maybe_produce_block(fc::mutable_variant_object& capture);

      bool _production_enabled = false;
      uint32_t _required_witness_participation = 33 * SOPHIATX_1_PERCENT;
      uint32_t _production_skip_flags = chain::database_interface::skip_nothing;

      std::map< sophiatx::protocol::public_key_type, fc::ecc::private_key > _private_keys;
      std::set< sophiatx::protocol::account_name_type >                     _witnesses;
      boost::asio::deadline_timer                                          _timer;

      std::shared_ptr<chain::database>  _db;
      plugins::chain::chain_plugin& _chain_plugin;
      boost::signals2::connection   pre_apply_connection;
      boost::signals2::connection   applied_block_connection;
      boost::signals2::connection   on_pre_apply_transaction_connection;
   };


   void check_memo( const string& memo, const chain::account_object& account, const account_authority_object& auth )
   {
      vector< public_key_type > keys;

      try
      {
         // Check if memo is a private key
         keys.push_back( fc::ecc::extended_private_key::from_base58( memo ).get_public_key() );
      }
      catch( fc::parse_error_exception& ) {}
      catch( fc::assert_exception& ) {}

      // Get possible keys if memo was an account password
      string owner_seed = account.name + "owner" + memo;
      auto owner_secret = fc::sha256::hash( owner_seed.c_str(), owner_seed.size() );
      keys.push_back( fc::ecc::private_key::regenerate( owner_secret ).get_public_key() );

      string active_seed = account.name + "active" + memo;
      auto active_secret = fc::sha256::hash( active_seed.c_str(), active_seed.size() );
      keys.push_back( fc::ecc::private_key::regenerate( active_secret ).get_public_key() );

      string posting_seed = account.name + "posting" + memo;
      auto posting_secret = fc::sha256::hash( posting_seed.c_str(), posting_seed.size() );
      keys.push_back( fc::ecc::private_key::regenerate( posting_secret ).get_public_key() );

      // Check keys against public keys in authorites
      for( auto& key_weight_pair : auth.owner.key_auths )
      {
         for( auto& key : keys )
            SOPHIATX_ASSERT( key_weight_pair.first != key,  plugin_exception,
               "Detected private owner key in memo field. You should change your owner keys." );
      }

      for( auto& key_weight_pair : auth.active.key_auths )
      {
         for( auto& key : keys )
            SOPHIATX_ASSERT( key_weight_pair.first != key,  plugin_exception,
               "Detected private active key in memo field. You should change your active keys." );
      }

      const auto& memo_key = account.memo_key;
      for( auto& key : keys )
         SOPHIATX_ASSERT( memo_key != key,  plugin_exception,
            "Detected private memo key in memo field. You should change your memo key." );
   }

   struct operation_visitor
   {
      operation_visitor( const std::shared_ptr<chain::database_interface>& db ) : _db( db ) {}

      const std::shared_ptr<database_interface> _db;

      typedef void result_type;

      template< typename T >
      void operator()( const T& )const {}

      void operator()( const transfer_operation& o )const
      {
         if( o.memo.length() > 0 )
            check_memo( o.memo,
                        _db->get< chain::account_object, chain::by_name >( o.from ),
                        _db->get< account_authority_object, chain::by_account >( o.from ) );
      }

   };

   void witness_plugin_impl::pre_transaction( const sophiatx::protocol::signed_transaction& trx )
   {
      flat_set< account_name_type > required; vector<authority> other;
      trx.get_required_authorities( required, required, other );

      //auto trx_size = fc::raw::pack_size(trx);

      //TODO_SOPHIA - rework to evaluate if the fee is corresponding to the requirements.
   }

   void witness_plugin_impl::pre_operation( const chain::operation_notification& note )
   {
      if( _db->is_producing() )
      {
         note.op.visit( operation_visitor( _db ) );
      }
   }

   void witness_plugin_impl::on_block( const signed_block& b )
   { try {
      int64_t max_block_size = _db->get_dynamic_global_properties().maximum_block_size;

      auto reserve_ratio_ptr = _db->find( reserve_ratio_id_type() );

      if( BOOST_UNLIKELY( reserve_ratio_ptr == nullptr ) )
      {
         _db->create< reserve_ratio_object >( [&]( reserve_ratio_object& r )
         {
            r.average_block_size = 0;
            r.current_reserve_ratio = SOPHIATX_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION;
            r.max_virtual_bandwidth = ( static_cast<uint128_t>( SOPHIATX_MAX_BLOCK_SIZE) * SOPHIATX_MAX_RESERVE_RATIO
                                       * SOPHIATX_BANDWIDTH_PRECISION * SOPHIATX_BANDWIDTH_AVERAGE_WINDOW_SECONDS )
                                       / SOPHIATX_BLOCK_INTERVAL;
         });
      }
      else
      {
         _db->modify( *reserve_ratio_ptr, [&]( reserve_ratio_object& r )
         {
            r.average_block_size = ( 99 * r.average_block_size + fc::raw::pack_size( b ) ) / 100;

            /**
            * About once per minute the average network use is consulted and used to
            * adjust the reserve ratio. Anything above 25% usage reduces the reserve
            * ratio proportional to the distance from 25%. If usage is at 50% then
            * the reserve ratio will half. Likewise, if it is at 12% it will increase by 50%.
            *
            * If the reserve ratio is consistently low, then it is probably time to increase
            * the capcacity of the network.
            *
            * This algorithm is designed to react quickly to observations significantly
            * different from past observed behavior and make small adjustments when
            * behavior is within expected norms.
            */
            if( _db->head_block_num() % 20 == 0 )
            {
               int64_t distance = ( ( r.average_block_size - ( max_block_size / 4 ) ) * DISTANCE_CALC_PRECISION )
                  / ( max_block_size / 4 );
               auto old_reserve_ratio = r.current_reserve_ratio;

               if( distance > 0 )
               {
                  r.current_reserve_ratio -= ( r.current_reserve_ratio * distance ) / ( distance + DISTANCE_CALC_PRECISION );

                  // We do not want the reserve ratio to drop below 1
                  if( r.current_reserve_ratio < RESERVE_RATIO_PRECISION )
                     r.current_reserve_ratio = RESERVE_RATIO_PRECISION;
               }
               else
               {
                  // By default, we should always slowly increase the reserve ratio.
                  r.current_reserve_ratio += std::max( RESERVE_RATIO_MIN_INCREMENT, ( r.current_reserve_ratio * distance ) / ( distance - DISTANCE_CALC_PRECISION ) );

                  if( r.current_reserve_ratio > SOPHIATX_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION )
                     r.current_reserve_ratio = SOPHIATX_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION;
               }

               if( old_reserve_ratio != r.current_reserve_ratio )
               {
                  ilog( "Reserve ratio updated from ${old} to ${new}. Block: ${blocknum}",
                     ("old", old_reserve_ratio)
                     ("new", r.current_reserve_ratio)
                     ("blocknum", _db->head_block_num()) );
               }

               r.max_virtual_bandwidth = ( uint128_t( max_block_size ) * uint128_t( r.current_reserve_ratio )
                                          * uint128_t( SOPHIATX_BANDWIDTH_PRECISION * SOPHIATX_BANDWIDTH_AVERAGE_WINDOW_SECONDS ) )
                                          / ( SOPHIATX_BLOCK_INTERVAL * RESERVE_RATIO_PRECISION );
            }
         });
      }
   } FC_LOG_AND_RETHROW() }
   #pragma message( "Remove FC_LOG_AND_RETHROW here before appbase release. It exists to help debug a rare lock exception" )


   void witness_plugin_impl::schedule_production_loop() {
      // Sleep for 200ms, before checking the block production
      fc::time_point now = fc::time_point::now();
      int64_t time_to_sleep = BLOCK_PRODUCTION_LOOP_SLEEP_TIME - (now.time_since_epoch().count() % BLOCK_PRODUCTION_LOOP_SLEEP_TIME);
      if (time_to_sleep < 50000) // we must sleep for at least 50ms
         time_to_sleep += BLOCK_PRODUCTION_LOOP_SLEEP_TIME;


      _timer.expires_from_now( boost::posix_time::microseconds( time_to_sleep ) );
      _timer.async_wait( boost::bind( &witness_plugin_impl::block_production_loop, this ) );
   }

   block_production_condition::block_production_condition_enum witness_plugin_impl::block_production_loop()
   {
      auto db = std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db());
      if( fc::time_point::now() < fc::time_point(db->get_genesis_time()) )
      {
         wlog( "waiting until genesis time to produce block: ${t}, now is: ${n}", ("t", db->get_genesis_time())("n", fc::time_point::now()) );
         schedule_production_loop();
         return block_production_condition::wait_for_genesis;
      }

      block_production_condition::block_production_condition_enum result;
      fc::mutable_variant_object capture;
      try
      {
         result = maybe_produce_block(capture);
      }
      catch( const fc::canceled_exception& )
      {
         //We're trying to exit. Go ahead and let this one out.
         throw;
      }
      catch( const chain::unknown_hardfork_exception& e )
      {
         // Hit a hardfork that the current node know nothing about, stop production and inform user
         elog( "${e}\nNode may be out of date...", ("e", e.to_detail_string()) );
         throw;
      }
      catch( const fc::exception& e )
      {
         elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
         result = block_production_condition::exception_producing_block;
      }

      switch(result)
      {
         case block_production_condition::produced:
            ilog("Generated block #${n} with timestamp ${t} and transactions count ${tc} by ${w} at time ${c}", (capture));
            break;
         case block_production_condition::not_synced:
            elog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
            break;
         case block_production_condition::not_my_turn:
   //         ilog("Not producing block because it isn't my turn");
            break;
         case block_production_condition::not_time_yet:
   //         ilog("Not producing block because slot has not yet arrived");
            break;
         case block_production_condition::no_private_key:
            ilog("Not producing block because I don't have the private key for ${scheduled_key}", (capture) );
            break;
         case block_production_condition::low_participation:
            elog("Not producing block because node appears to be on a minority fork with only ${pct}% witness participation", (capture) );
            break;
         case block_production_condition::lag:
            elog("Not producing block because node didn't wake up within ${t}ms of the slot time.", ("t", BLOCK_PRODUCING_LAG_TIME));
            break;
         case block_production_condition::consecutive:
            elog("Not producing block because the last block was generated by the same witness.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
            break;
         case block_production_condition::exception_producing_block:
            elog( "exception producing block" );
            break;
         case block_production_condition::wait_for_genesis:
            break;
      }

      schedule_production_loop();
      return result;
   }

   block_production_condition::block_production_condition_enum witness_plugin_impl::maybe_produce_block(fc::mutable_variant_object& capture)
   {
      auto db = std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db());
      fc::time_point now_fine = fc::time_point::now();
      fc::time_point_sec now = now_fine + fc::microseconds( 500000 );

      // If the next block production opportunity is in the present or future, we're synced.
      if( !_production_enabled )
      {
         if( db->get_slot_time(1) >= now )
            _production_enabled = true;
         else
            return block_production_condition::not_synced;
      }

      // is anyone scheduled to produce now or one second in the future?
      uint32_t slot = db->get_slot_at_time( now );
      if( slot == 0 )
      {
         capture("next_time", db->get_slot_time(1));
         return block_production_condition::not_time_yet;
      }

      //
      // this assert should not fail, because now <= db->head_block_time()
      // should have resulted in slot == 0.
      //
      // if this assert triggers, there is a serious bug in get_slot_at_time()
      // which would result in allowing a later block to have a timestamp
      // less than or equal to the previous block
      //
      assert( now > db->head_block_time() );

      chain::account_name_type scheduled_witness = db->get_scheduled_witness( slot );
      // we must control the witness scheduled to produce the next block.
      if( _witnesses.find( scheduled_witness ) == _witnesses.end() )
      {
         capture("scheduled_witness", scheduled_witness);
         return block_production_condition::not_my_turn;
      }

      fc::time_point_sec scheduled_time = db->get_slot_time( slot );
      chain::public_key_type scheduled_key = db->get< chain::witness_object, chain::by_name >(scheduled_witness).signing_key;
      auto private_key_itr = _private_keys.find( scheduled_key );

      if( private_key_itr == _private_keys.end() )
      {
         capture("scheduled_witness", scheduled_witness);
         capture("scheduled_key", scheduled_key);
         return block_production_condition::no_private_key;
      }

      uint32_t prate = db->witness_participation_rate();
      if( prate < _required_witness_participation )
      {
         capture("pct", uint32_t(100*uint64_t(prate) / SOPHIATX_1_PERCENT));
         return block_production_condition::low_participation;
      }

      if( llabs((scheduled_time - now).count()) > fc::milliseconds( BLOCK_PRODUCING_LAG_TIME ).count() )
      {
         capture("scheduled_time", scheduled_time)("now", now);
         return block_production_condition::lag;
      }

      auto block = _chain_plugin.generate_block(
         scheduled_time,
         scheduled_witness,
         private_key_itr->second,
         _production_skip_flags
         );
      capture("n", block.block_num())("t", block.timestamp)("c", now)("w", scheduled_witness)("tc", block.transactions.size());

      appbase::app().get_plugin< sophiatx::plugins::p2p::p2p_plugin >().broadcast_block( block );
      return block_production_condition::produced;
   }

} // detail


witness_plugin::witness_plugin() {}
witness_plugin::~witness_plugin() {}

void witness_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg)
{
   string witness_id_example = "initwitness";
   cfg.add_options()
         ("enable-stale-production", bpo::bool_switch()->default_value(false), "Enable block production, even if the chain is stale.")
         ("required-participation", bpo::value< uint32_t >()->default_value( 67 ), "Percent of witnesses (0-99) that must be participating in order to produce blocks")
         ("witness,w", bpo::value<vector<string>>()->composing()->multitoken(),
            ("name of witness controlled by this node (e.g. " + witness_id_example + " )" ).c_str() )
         ("private-key", bpo::value<vector<string>>()->composing()->multitoken(), "WIF PRIVATE KEY to be used by one or more witnesses or miners" )
         ;
}

void witness_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   my = std::make_unique< detail::witness_plugin_impl >( appbase::app().get_io_service() );

   SOPHIATX_LOAD_VALUE_SET( options, "witness", my->_witnesses, sophiatx::protocol::account_name_type )

   if( options.count("private-key") )
   {
      const std::vector<std::string> keys = options["private-key"].as<std::vector<std::string>>();
      for (const std::string& wif_key : keys )
      {
         fc::optional<fc::ecc::private_key> private_key = sophiatx::utilities::wif_to_key(wif_key);
         FC_ASSERT( private_key.valid(), "unable to parse private key" );
         my->_private_keys[private_key->get_public_key()] = *private_key;
      }
   }

   my->_production_enabled = options.at( "enable-stale-production" ).as< bool >();

   if( options.count( "required-participation" ) )
   {
      my->_required_witness_participation = SOPHIATX_1_PERCENT * options.at( "required-participation" ).as< uint32_t >();
   }

   my->on_pre_apply_transaction_connection = my->_db->on_pre_apply_transaction.connect( 0, [&]( const signed_transaction& tx ){ my->pre_transaction( tx ); } );
   my->pre_apply_connection = my->_db->pre_apply_operation.connect( 0, [&]( const operation_notification& note ){ my->pre_operation( note ); } );
   my->applied_block_connection = my->_db->applied_block.connect( 0, [&]( const signed_block& b ){ my->on_block( b ); } );

   add_plugin_index< account_bandwidth_index >( my->_db );
   add_plugin_index< reserve_ratio_index     >( my->_db );

   if( my->_witnesses.size() && my->_private_keys.size() )
      my->_chain_plugin.set_write_lock_hold_time( -1 );
} FC_LOG_AND_RETHROW() }

void witness_plugin::plugin_startup()
{ try {
   ilog("witness plugin:  plugin_startup() begin");
      auto d = std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db());

   if( !my->_witnesses.empty() )
   {
      ilog( "Launching block production for ${n} witnesses.", ("n", my->_witnesses.size()) );
      appbase::app().get_plugin< sophiatx::plugins::p2p::p2p_plugin >().set_block_production( true );
      if( my->_production_enabled )
      {
         if( d->head_block_num() == 0 )
            new_chain_banner();
         my->_production_skip_flags |= chain::database_interface::skip_undo_history_check;
      }
      my->schedule_production_loop();
   } else
      elog("No witnesses configured! Please add witness IDs and private keys to configuration.");
   ilog("witness plugin:  plugin_startup() end");
   } FC_CAPTURE_AND_RETHROW() }

void witness_plugin::plugin_shutdown()
{
   try
   {
      chain::util::disconnect_signal( my->pre_apply_connection );
      chain::util::disconnect_signal( my->applied_block_connection );
      chain::util::disconnect_signal( my->on_pre_apply_transaction_connection );

      my->_timer.cancel();
   }
   catch(fc::exception& e)
   {
      edump( (e.to_detail_string()) );
   }
}

} } } // sophiatx::plugins::witness
