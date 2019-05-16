#include <sophiatx/chain/database/database_exceptions.hpp>
#include <sophiatx/chain/genesis_state.hpp>

#include <sophiatx/plugins/chain/chain_plugin_full.hpp>
#include <sophiatx/chain/database/database.hpp>

#include <sophiatx/utilities/benchmark_dumper.hpp>

#include <sophiatx/egenesis/egenesis.hpp>

#include <fc/string.hpp>
#include <fc/io/fstream.hpp>

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/thread/future.hpp>

#include <thread>
#include <memory>
#include <iostream>

namespace sophiatx { namespace plugins { namespace chain {

using namespace sophiatx;
using fc::flat_map;
using sophiatx::chain::block_id_type;
namespace asio = boost::asio;


chain_plugin_full::chain_plugin_full() : write_queue( 64 ) {
   db_ = std::make_shared<database>();
}

chain_plugin_full::~chain_plugin_full() { stop_write_processing(); }

struct write_request_visitor
{
   write_request_visitor() {}

   std::shared_ptr<database> db;
   uint32_t  skip = 0;
   fc::optional< fc::exception >* except;

   typedef bool result_type;

   bool operator()( const signed_block* block )
   {
      bool result = false;

      try
      {
         result = db->push_block( *block, skip );
      }
      catch( fc::exception& e )
      {
         *except = e;
      }
      catch( ... )
      {
         *except = fc::unhandled_exception( FC_LOG_MESSAGE( warn, "Unexpected exception while pushing block." ),
                                            std::current_exception() );
      }

      return result;
   }

   bool operator()( const signed_transaction* trx )
   {
      bool result = false;

      try
      {
         db->push_transaction( *trx );
         result = true;
      }
      catch( fc::exception& e )
      {
         *except = e;
      }
      catch( ... )
      {
         *except = fc::unhandled_exception( FC_LOG_MESSAGE( warn, "Unexpected exception while pushing block." ),
                                            std::current_exception() );
      }

      return result;
   }


   bool operator()( generate_block_request* req )
   {
      bool result = false;
      try{
         req->block = db->generate_block(
               req->when,
               req->witness_owner,
               req->block_signing_private_key,
               req->skip
         );
         result = true;
      }catch( fc::exception& e ){
         *except = e;
      }
      catch( ... )
      {
         *except = fc::unhandled_exception( FC_LOG_MESSAGE( warn, "Unexpected exception while pushing block." ),
                                            std::current_exception() );
      }

      return result;
   }
};


struct request_promise_visitor
{
   request_promise_visitor(){}

   typedef void result_type;

   template< typename T >
   void operator()( T* t )
   {
      t->set_value();
   }
};

void chain_plugin_full::start_write_processing()
{
   write_processor_thread = std::make_shared< std::thread >( [&](){
        bool is_syncing = true;
        write_context* cxt;
        fc::time_point_sec start = fc::time_point::now();
        write_request_visitor req_visitor;
        req_visitor.db = std::static_pointer_cast<database>(db_);

        request_promise_visitor prom_visitor;

        /* This loop monitors the write request queue and performs writes to the database. These
         * can be blocks or pending transactions. Because the caller needs to know the success of
         * the write and any exceptions that are thrown, a write context is passed in the queue
         * to the processing thread which it will use to store the results of the write. It is the
         * caller's responsibility to ensure the pointer to the write context remains valid until
         * the contained promise is complete.
         *
         * The loop has two modes, sync mode and live mode. In sync mode we want to process writes
         * as quickly as possible with minimal overhead. The outer loop busy waits on the queue
         * and the inner loop drains the queue as quickly as possible. We exit sync mode when the
         * head block is within 1 minute of system time.
         *
         * Live mode needs to balance between processing pending writes and allowing readers access
         * to the database. It will batch writes together as much as possible to minimize lock
         * overhead but will willingly give up the write lock after 500ms. The thread then sleeps for
         * 10ms. This allows time for readers to access the database as well as more writes to come
         * in. When the node is live the rate at which writes come in is slower and busy waiting is
         * not an optimal use of system resources when we could give CPU time to read threads.
         */
        while( running )
        {
           if( !is_syncing )
              start = fc::time_point::now();

           if( write_queue.pop( cxt ) )
           {
              db_->with_write_lock( [&](){
                   while( true )
                   {
                      req_visitor.skip = cxt->skip;
                      req_visitor.except = &(cxt->except);
                      cxt->success = cxt->req_ptr.visit( req_visitor );
                      cxt->prom_ptr.visit( prom_visitor );

                      if( is_syncing && start - db_->head_block_time() < fc::minutes(1) )
                      {
                         start = fc::time_point::now();
                         is_syncing = false;
                      }

                      if( !is_syncing && write_lock_hold_time >= 0 && fc::time_point::now() - start > fc::milliseconds( write_lock_hold_time ) )
                      {
                         break;
                      }

                      if( !write_queue.pop( cxt ) )
                      {
                         break;
                      }
                   }
              });

           }

           if( !is_syncing ) {
              boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
           } else
              boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
        }
   });
}

void chain_plugin_full::stop_write_processing()
{
   running = false;

   if( write_processor_thread )
      write_processor_thread->join();

   write_processor_thread.reset();
}

void chain_plugin_full::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()      
         ("genesis-json", bpo::value<bfs::path>(), "the location of the genesis file in JSON format")
         ("shared-file-size", bpo::value<string>()->default_value("24G"), "Size of the shared memory file. Default: 24G. If running a full node, increase this value to 200G.")
         ("shared-file-full-threshold", bpo::value<uint16_t>()->default_value(0),
            "A 2 precision percentage (0-10000) that defines the threshold for when to autoscale the shared memory file. Setting this to 0 disables autoscaling. Recommended value for consensus node is 9500 (95%). Full node is 9900 (99%)" )
         ("shared-file-scale-rate", bpo::value<uint16_t>()->default_value(0),
            "A 2 precision percentage (0-10000) that defines how quickly to scale the shared memory file. When autoscaling occurs the file's size will be increased by this percent. Setting this to 0 disables autoscaling. Recommended value is between 1000-2000 (10-20%)" )
         ("checkpoint,c", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("flush-state-interval", bpo::value<uint32_t>(),
            "flush shared memory changes to disk every N blocks")
         ;
   cli.add_options()
         ("replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and replay all blocks" )
         ("resync-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and block log" )
         ("stop-replay-at-block", bpo::value<uint32_t>(), "Stop and exit after reaching given block number")
         ("set-benchmark-interval", bpo::value<uint32_t>(), "Print time and memory usage every given number of blocks")
         ("dump-memory-details", bpo::bool_switch()->default_value(false), "Dump database objects memory usage info. Use set-benchmark-interval to set dump interval.")
         ("check-locks", bpo::bool_switch()->default_value(false), "Check correctness of chainbase locking" )
         ("validate-database-invariants", bpo::bool_switch()->default_value(false), "Validate all supply invariants check out" )
         ("initminer-mining-pubkey", bpo::value<std::string>(), "initminer public key for mining. Used only for private nets.")
         ("initminer-account-pubkey", bpo::value<std::string>(), "initminer public key for account operations. Used only for private nets.")
         ;
}

void chain_plugin_full::plugin_initialize(const variables_map& options) {

   shared_memory_size = fc::parse_size( options.at( "shared-file-size" ).as< string >() );

   if( options.count( "shared-file-full-threshold" ) )
      shared_file_full_threshold = options.at( "shared-file-full-threshold" ).as< uint16_t >();

   if( options.count( "shared-file-scale-rate" ) )
      shared_file_scale_rate = options.at( "shared-file-scale-rate" ).as< uint16_t >();


   // TODO: temporary solution. DELETE when initminer mining public key is read from get_config
   init_mining_pubkey = public_key_type(SOPHIATX_INIT_PUBLIC_KEY_STR);
   bool private_net = false;
   if (options.count("initminer-mining-pubkey") ) {
      init_mining_pubkey = public_key_type(options.at( "initminer-mining-pubkey" ).as< std::string >());
      private_net = true;
   }


   auto initial_state = [&] {

        if( (private_net && options.count("initminer-account-pubkey")) ||
            (private_net && options.count("genesis-json") == 0 ) ){
            genesis_state_type genesis;
            genesis.genesis_time = time_point_sec::from_iso_string("2018-01-01T08:00:00");
            genesis.initial_balace = 0;
            genesis.initial_public_key = options.count("initminer-account-pubkey") ?
                                       public_key_type(options.at( "initminer-account-pubkey" ).as< std::string >()) :
                                       public_key_type(options.at( "initminer-mining-pubkey" ).as< std::string >());
            genesis.is_private_net = true;

            fc::sha256::encoder enc;
            fc::raw::pack( enc, genesis );
            genesis.initial_chain_id = enc.result();

            return genesis;
        }

        if( options.count("genesis-json") )
        {
           std::string genesis_str;
           fc::read_file_contents( options.at("genesis-json").as<boost::filesystem::path>(), genesis_str );
           genesis_state_type genesis = fc::json::from_string( genesis_str ).as<genesis_state_type>();
           genesis.initial_chain_id = fc::sha256::hash( genesis_str);
           genesis.is_private_net = private_net;
           return genesis;
        }
        else
        {
           std::string egenesis_json;
           sophiatx::egenesis::compute_egenesis_json( egenesis_json );
           FC_ASSERT( !egenesis_json.empty() );
           FC_ASSERT( sophiatx::egenesis::get_egenesis_json_hash() == fc::sha256::hash( egenesis_json ) );
           auto genesis = fc::json::from_string( egenesis_json ).as<genesis_state_type>();
           genesis.initial_chain_id = fc::sha256::hash( egenesis_json );

           return genesis;
        }
   };

   genesis             = initial_state();
   replay              = options.at( "replay-blockchain").as<bool>();
   resync              = options.at( "resync-blockchain").as<bool>();
   stop_replay_at      =
      options.count( "stop-replay-at-block" ) ? options.at( "stop-replay-at-block" ).as<uint32_t>() : 0;
   benchmark_interval  =
      options.count( "set-benchmark-interval" ) ? options.at( "set-benchmark-interval" ).as<uint32_t>() : 0;
   check_locks         = options.at( "check-locks" ).as< bool >();
   validate_invariants = options.at( "validate-database-invariants" ).as<bool>();
   dump_memory_details = options.at( "dump-memory-details" ).as<bool>();

   if( options.count( "flush-state-interval" ) )
      flush_interval = options.at( "flush-state-interval" ).as<uint32_t>();
   else
      flush_interval = 10000;

   if(options.count("checkpoint"))
   {
      auto cps = options.at("checkpoint").as<vector<string>>();
      loaded_checkpoints.reserve(cps.size());
      for(const auto& cp : cps)
      {
         auto item = fc::json::from_string(cp).as<std::pair<uint32_t,block_id_type>>();
         loaded_checkpoints[item.first] = item.second;
      }
   }
}

#define BENCHMARK_FILE_NAME "replay_benchmark.json"

void chain_plugin_full::plugin_startup()
{
   ilog( "Starting chain with shared_file_size: ${n} bytes", ("n", shared_memory_size) );

   chain_id_type chain_id = genesis.compute_chain_id();

   shared_memory_dir = app().data_dir() / chain_id.str() / "blockchain";

   // correct directories, TODO can be removed after next HF2
   if( ! genesis.is_private_net && bfs::exists( app().data_dir() / "blockchain" ) ){
      bfs::create_directories ( shared_memory_dir );
      bfs::rename( app().data_dir() / "blockchain", shared_memory_dir );
   }

   ilog("Starting node with chain id ${i}", ("i", chain_id));

   start_write_processing();

   if(resync)
   {
      wlog("resync requested: deleting block log and shared memory");
      db_->wipe( shared_memory_dir, true );
   }

   db_->set_flush_interval( flush_interval );
   db_->add_checkpoints( loaded_checkpoints );
   db_->set_require_locking( check_locks );

   bool dump_memory_details_ = dump_memory_details;
   sophiatx::utilities::benchmark_dumper dumper;

   const auto& abstract_index_cntr = db_->get_abstract_index_cntr();

   typedef sophiatx::utilities::benchmark_dumper::index_memory_details_cntr_t index_memory_details_cntr_t;
   auto get_indexes_memory_details = [dump_memory_details_, &abstract_index_cntr]
      (index_memory_details_cntr_t& index_memory_details_cntr, bool onlyStaticInfo)
   {
      if (!dump_memory_details_)
         return;

      for (auto idx : abstract_index_cntr)
      {
         auto info = idx->get_statistics(onlyStaticInfo);
         index_memory_details_cntr.emplace_back(std::move(info._value_type_name), info._item_count,
            info._item_sizeof, info._item_additional_allocation, info._additional_container_allocation);
      }
   };

   database::open_args db_open_args;
   db_open_args.shared_mem_dir = shared_memory_dir;
   db_open_args.shared_file_size = shared_memory_size;
   db_open_args.shared_file_full_threshold = shared_file_full_threshold;
   db_open_args.shared_file_scale_rate = shared_file_scale_rate;
   db_open_args.do_validate_invariants = validate_invariants;
   db_open_args.stop_replay_at = stop_replay_at;

   auto benchmark_lambda = [&dumper, &get_indexes_memory_details, dump_memory_details_] ( uint32_t current_block_number,
      const chainbase::database::abstract_index_cntr_t& abstract_index_cntr )
   {
      if( current_block_number == 0 ) // initial call
      {
         typedef sophiatx::utilities::benchmark_dumper::database_object_sizeof_cntr_t database_object_sizeof_cntr_t;
         auto get_database_objects_sizeofs = [dump_memory_details_, &abstract_index_cntr]
            (database_object_sizeof_cntr_t& database_object_sizeof_cntr)
         {
            if (!dump_memory_details_)
               return;

            for (auto idx : abstract_index_cntr)
            {
               auto info = idx->get_statistics(true);
               database_object_sizeof_cntr.emplace_back(std::move(info._value_type_name), info._item_sizeof);
            }
         };

         dumper.initialize(get_database_objects_sizeofs, BENCHMARK_FILE_NAME);
         return;
      }

      const sophiatx::utilities::benchmark_dumper::measurement& measure =
         dumper.measure(current_block_number, get_indexes_memory_details);
      ilog( "Performance report at block ${n}. Elapsed time: ${rt} ms (real), ${ct} ms (cpu). Memory usage: ${cm} (current), ${pm} (peak) kilobytes.",
         ("n", current_block_number)
         ("rt", measure.real_ms)
         ("ct", measure.cpu_ms)
         ("cm", measure.current_mem)
         ("pm", measure.peak_mem) );
   };

   if(replay)
   {
      ilog("Replaying blockchain on user request.");
      uint32_t last_block_number = 0;
      db_open_args.benchmark = sophiatx::chain::database::TBenchmark(benchmark_interval, benchmark_lambda);
      last_block_number = db_->reindex( db_open_args, genesis, init_mining_pubkey );

      if( benchmark_interval > 0 )
      {
         const sophiatx::utilities::benchmark_dumper::measurement& total_data = dumper.dump(true, get_indexes_memory_details);
         ilog( "Performance report (total). Blocks: ${b}. Elapsed time: ${rt} ms (real), ${ct} ms (cpu). Memory usage: ${cm} (current), ${pm} (peak) kilobytes.",
               ("b", total_data.block_number)
               ("rt", total_data.real_ms)
               ("ct", total_data.cpu_ms)
               ("cm", total_data.current_mem)
               ("pm", total_data.peak_mem) );
      }

      if( stop_replay_at > 0 && stop_replay_at == last_block_number )
      {
         ilog("Stopped blockchain replaying on user request. Last applied block number: ${n}.", ("n", last_block_number));
         appbase::app().quit();
         return;
      }
   }
   else
   {
      db_open_args.benchmark = sophiatx::chain::database::TBenchmark(dump_memory_details_, benchmark_lambda);

      try
      {
         ilog("Opening shared memory from ${path}", ("path",shared_memory_dir.generic_string()));

         db_->open( db_open_args, genesis, init_mining_pubkey );

         if( dump_memory_details_ )
            dumper.dump( true, get_indexes_memory_details );
      }
      catch( const fc::exception& e )
      {
         wlog("Error opening database, attempting to replay blockchain. Error: ${e}", ("e", e));

         try
         {
            db_->reindex( db_open_args, genesis, init_mining_pubkey );
         }
         catch( sophiatx::chain::block_log_exception& )
         {
            wlog( "Error opening block log. Having to resync from network..." );
            db_->open( db_open_args, genesis, init_mining_pubkey );
         }
      }
   }

   ilog( "Started on blockchain with ${n} blocks", ("n", db_->head_block_num()) );
   on_sync();
}

void chain_plugin_full::plugin_shutdown()
{
   ilog("closing chain database");
   stop_write_processing();
   db_->close();
   ilog("database closed successfully");
}

bool chain_plugin_full::accept_block( const sophiatx::chain::signed_block& block, bool currently_syncing, uint32_t skip )
{
   if (currently_syncing && block.block_num() % 10000 == 0) {
      ilog("Syncing Blockchain --- Got block: #${n} time: ${t} producer: ${p}",
           ("t", block.timestamp)
           ("n", block.block_num())
           ("p", block.witness) );
   }

   check_time_in_block( block );

   boost::promise< void > prom;
   write_context cxt;
   cxt.req_ptr = &block;
   cxt.skip = currently_syncing? skip | database::skip_validate_invariants : skip;
   cxt.prom_ptr = &prom;

   write_queue.push( &cxt );

   prom.get_future().get();

   if( cxt.except ) throw *(cxt.except);

   return cxt.success;
}

void chain_plugin_full::accept_transaction( const sophiatx::chain::signed_transaction& trx )
{
   boost::promise< void > prom;
   write_context cxt;
   cxt.req_ptr = &trx;
   cxt.prom_ptr = &prom;

   write_queue.push( &cxt );

   prom.get_future().get();

   if( cxt.except ) throw *(cxt.except);

   return;
}

void chain_plugin_full::check_time_in_block( const sophiatx::chain::signed_block& block )
{
   time_point_sec now = fc::time_point::now();

   uint64_t max_accept_time = now.sec_since_epoch();
   max_accept_time += allow_future_time;
   FC_ASSERT( block.timestamp.sec_since_epoch() <= max_accept_time );
}

sophiatx::chain::signed_block chain_plugin_full::generate_block( const fc::time_point_sec& when, const account_name_type& witness_owner,
                                                         const fc::ecc::private_key& block_signing_private_key, uint32_t skip )
{
   generate_block_request req( when, witness_owner, block_signing_private_key, skip );
   boost::promise< void > prom;
   write_context cxt;
   cxt.req_ptr = &req;
   cxt.prom_ptr = &prom;

   write_queue.push( &cxt );

   prom.get_future().get();

   if( cxt.except ) throw *(cxt.except);

   FC_ASSERT( cxt.success, "Block could not be generated" );

   return req.block;
}

int16_t chain_plugin_full::set_write_lock_hold_time( int16_t new_time )
{
   FC_ASSERT( get_state() == appbase::abstract_plugin::state::initialized,
              "Can only change write_lock_hold_time while chain_plugin_full is initialized." );

   int16_t old_time = write_lock_hold_time;
   write_lock_hold_time = new_time;
   return old_time;
}

} } } // namespace sophiatx::plugis::chain::chain_apis
