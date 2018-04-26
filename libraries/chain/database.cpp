#include <steem/protocol/steem_operations.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/compound.hpp>
#include <steem/chain/custom_operation_interpreter.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/db_with.hpp>
#include <steem/chain/evaluator_registry.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/shared_db_merkle.hpp>
#include <steem/chain/operation_notification.hpp>
#include <steem/chain/witness_schedule.hpp>

#include <steem/chain/util/asset.hpp>
#include <steem/chain/util/uint256.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <fc/container/deque.hpp>

#include <fc/io/fstream.hpp>

#include <boost/scope_exit.hpp>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

namespace steem { namespace chain {

struct object_schema_repr
{
   std::pair< uint16_t, uint16_t > space_type;
   std::string type;
};

struct operation_schema_repr
{
   std::string id;
   std::string type;
};

struct db_schema
{
   std::map< std::string, std::string > types;
   std::vector< object_schema_repr > object_types;
   std::string operation_type;
   std::vector< operation_schema_repr > custom_operation_types;
};

} }

FC_REFLECT( steem::chain::object_schema_repr, (space_type)(type) )
FC_REFLECT( steem::chain::operation_schema_repr, (id)(type) )
FC_REFLECT( steem::chain::db_schema, (types)(object_types)(operation_type)(custom_operation_types) )

namespace steem { namespace chain {

using boost::container::flat_set;

struct reward_fund_context
{
   uint128_t   recent_claims = 0;
   asset       reward_balance = asset( 0, STEEM_SYMBOL );
   share_type  steem_awarded = 0;
};

class database_impl
{
   public:
      database_impl( database& self );

      database&                              _self;
      evaluator_registry< operation >        _evaluator_registry;
};

database_impl::database_impl( database& self )
   : _self(self), _evaluator_registry(self) {}

database::database()
   : _my( new database_impl(*this) )
{
   set_chain_id( STEEM_CHAIN_ID_NAME );
}

database::~database()
{
   clear_pending();
}

void database::open( const open_args& args )
{
   try
   {
      init_schema();
      chainbase::database::open( args.shared_mem_dir, args.chainbase_flags, args.shared_file_size );

      initialize_indexes();
      initialize_evaluators();

      if( !find< dynamic_global_property_object >() )
         with_write_lock( [&]()
         {
            init_genesis( args.initial_supply );
         });

      _block_log.open( args.data_dir / "block_log" );

      auto log_head = _block_log.head();

      // Rewind all undo state. This should return us to the state at the last irreversible block.
      with_write_lock( [&]()
      {
         undo_all();
         FC_ASSERT( revision() == head_block_num(), "Chainbase revision does not match head block num",
            ("rev", revision())("head_block", head_block_num()) );
         if (args.do_validate_invariants)
            validate_invariants();
      });

      if( head_block_num() )
      {
         auto head_block = _block_log.read_block_by_num( head_block_num() );
         // This assertion should be caught and a reindex should occur
         FC_ASSERT( head_block.valid() && head_block->id() == head_block_id(), "Chain state does not match block log. Please reindex blockchain." );

         _fork_db.start_block( *head_block );
      }

      with_read_lock( [&]()
      {
         init_hardforks(); // Writes to local state, but reads from db
      });

      if (args.benchmark.first)
      {
         args.benchmark.second(0, get_abstract_index_cntr());
         auto last_block_num = _block_log.head()->block_num();
         args.benchmark.second(last_block_num, get_abstract_index_cntr());
      }

      _shared_file_full_threshold = args.shared_file_full_threshold;
      _shared_file_scale_rate = args.shared_file_scale_rate;
   }
   FC_CAPTURE_LOG_AND_RETHROW( (args.data_dir)(args.shared_mem_dir)(args.shared_file_size) )
}

uint32_t database::reindex( const open_args& args )
{
   bool reindex_success = false;
   uint32_t last_block_number = 0; // result

   BOOST_SCOPE_EXIT(this_,&reindex_success,&last_block_number) {
      STEEM_TRY_NOTIFY(this_->_on_reindex_done, reindex_success, last_block_number);
   } BOOST_SCOPE_EXIT_END

   try
   {
      STEEM_TRY_NOTIFY(_on_reindex_start);

      ilog( "Reindexing Blockchain" );
      wipe( args.data_dir, args.shared_mem_dir, false );
      open( args );
      _fork_db.reset();    // override effect of _fork_db.start_block() call in open()

      auto start = fc::time_point::now();
      STEEM_ASSERT( _block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain." );

      ilog( "Replaying blocks..." );

      uint64_t skip_flags =
         skip_witness_signature |
         skip_transaction_signatures |
         skip_transaction_dupe_check |
         skip_tapos_check |
         skip_merkle_check |
         skip_witness_schedule_check |
         skip_authority_check |
         skip_validate | /// no need to validate operations
         skip_validate_invariants |
         skip_block_log;

      with_write_lock( [&]()
      {
         _block_log.set_locking( false );
         auto itr = _block_log.read_block( 0 );
         auto last_block_num = _block_log.head()->block_num();
         if( args.stop_replay_at > 0 && args.stop_replay_at < last_block_num )
            last_block_num = args.stop_replay_at;
         if( args.benchmark.first > 0 )
         {
            args.benchmark.second( 0, get_abstract_index_cntr() );
         }

         while( itr.first.block_num() != last_block_num )
         {
            auto cur_block_num = itr.first.block_num();
            if( cur_block_num % 100000 == 0 )
               std::cerr << "   " << double( cur_block_num * 100 ) / last_block_num << "%   " << cur_block_num << " of " << last_block_num <<
               "   (" << (get_free_memory() / (1024*1024)) << "M free)\n";
            apply_block( itr.first, skip_flags );

            if( (args.benchmark.first > 0) && (cur_block_num % args.benchmark.first == 0) )
               args.benchmark.second( cur_block_num, get_abstract_index_cntr() );
            itr = _block_log.read_block( itr.second );
         }

         apply_block( itr.first, skip_flags );
         last_block_number = itr.first.block_num();

         if( (args.benchmark.first > 0) && (last_block_number % args.benchmark.first == 0) )
            args.benchmark.second( last_block_number, get_abstract_index_cntr() );
         set_revision( head_block_num() );
         _block_log.set_locking( true );
      });

      if( _block_log.head()->block_num() )
         _fork_db.start_block( *_block_log.head() );

      auto end = fc::time_point::now();
      ilog( "Done reindexing, elapsed time: ${t} sec", ("t",double((end-start).count())/1000000.0 ) );

      reindex_success = true;

      return last_block_number;
   }
   FC_CAPTURE_AND_RETHROW( (args.data_dir)(args.shared_mem_dir) )

}

void database::wipe( const fc::path& data_dir, const fc::path& shared_mem_dir, bool include_blocks)
{
   close();
   chainbase::database::wipe( shared_mem_dir );
   if( include_blocks )
   {
      fc::remove_all( data_dir / "block_log" );
      fc::remove_all( data_dir / "block_log.index" );
   }
}

void database::close(bool rewind)
{
   try
   {
      // Since pop_block() will move tx's in the popped blocks into pending,
      // we have to clear_pending() after we're done popping to get a clean
      // DB state (issue #336).
      clear_pending();

      chainbase::database::flush();
      chainbase::database::close();

      _block_log.close();

      _fork_db.reset();
   }
   FC_CAPTURE_AND_RETHROW()
}

bool database::is_known_block( const block_id_type& id )const
{ try {
   return fetch_block_by_id( id ).valid();
} FC_CAPTURE_AND_RETHROW() }

/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool database::is_known_transaction( const transaction_id_type& id )const
{ try {
   const auto& trx_idx = get_index<transaction_index>().indices().get<by_trx_id>();
   return trx_idx.find( id ) != trx_idx.end();
} FC_CAPTURE_AND_RETHROW() }

block_id_type database::find_block_id_for_num( uint32_t block_num )const
{
   try
   {
      if( block_num == 0 )
         return block_id_type();

      // Reversible blocks are *usually* in the TAPOS buffer.  Since this
      // is the fastest check, we do it first.
      block_summary_id_type bsid = block_num & 0xFFFF;
      const block_summary_object* bs = find< block_summary_object, by_id >( bsid );
      if( bs != nullptr )
      {
         if( protocol::block_header::num_from_id(bs->block_id) == block_num )
            return bs->block_id;
      }

      // Next we query the block log.   Irreversible blocks are here.
      auto b = _block_log.read_block_by_num( block_num );
      if( b.valid() )
         return b->id();

      // Finally we query the fork DB.
      shared_ptr< fork_item > fitem = _fork_db.fetch_block_on_main_branch_by_number( block_num );
      if( fitem )
         return fitem->id;

      return block_id_type();
   }
   FC_CAPTURE_AND_RETHROW( (block_num) )
}

block_id_type database::get_block_id_for_num( uint32_t block_num )const
{
   block_id_type bid = find_block_id_for_num( block_num );
   FC_ASSERT( bid != block_id_type() );
   return bid;
}


optional<signed_block> database::fetch_block_by_id( const block_id_type& id )const
{ try {
   auto b = _fork_db.fetch_block( id );
   if( !b )
   {
      auto tmp = _block_log.read_block_by_num( protocol::block_header::num_from_id( id ) );

      if( tmp && tmp->id() == id )
         return tmp;

      tmp.reset();
      return tmp;
   }

   return b->data;
} FC_CAPTURE_AND_RETHROW() }

optional<signed_block> database::fetch_block_by_number( uint32_t block_num )const
{ try {
   optional< signed_block > b;

   auto results = _fork_db.fetch_block_by_number( block_num );
   if( results.size() == 1 )
      b = results[0]->data;
   else
      b = _block_log.read_block_by_num( block_num );

   return b;
} FC_LOG_AND_RETHROW() }

const signed_transaction database::get_recent_transaction( const transaction_id_type& trx_id ) const
{ try {
   auto& index = get_index<transaction_index>().indices().get<by_trx_id>();
   auto itr = index.find(trx_id);
   FC_ASSERT(itr != index.end());
   signed_transaction trx;
   fc::raw::unpack_from_buffer( itr->packed_trx, trx );
   return trx;;
} FC_CAPTURE_AND_RETHROW() }

std::vector< block_id_type > database::get_block_ids_on_fork( block_id_type head_of_fork ) const
{ try {
   pair<fork_database::branch_type, fork_database::branch_type> branches = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);
   if( !((branches.first.back()->previous_id() == branches.second.back()->previous_id())) )
   {
      edump( (head_of_fork)
             (head_block_id())
             (branches.first.size())
             (branches.second.size()) );
      assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
   }
   std::vector< block_id_type > result;
   for( const item_ptr& fork_block : branches.second )
      result.emplace_back(fork_block->id);
   result.emplace_back(branches.first.back()->previous_id());
   return result;
} FC_CAPTURE_AND_RETHROW() }

chain_id_type database::get_chain_id() const
{
   return steem_chain_id;
}

void database::set_chain_id( const std::string& _chain_id_name )
{
   steem_chain_id = generate_chain_id( _chain_id_name );
}

const witness_object& database::get_witness( const account_name_type& name ) const
{ try {
   return get< witness_object, by_name >( name );
} FC_CAPTURE_AND_RETHROW( (name) ) }

const witness_object* database::find_witness( const account_name_type& name ) const
{
   return find< witness_object, by_name >( name );
}

const account_object& database::get_account( const account_name_type& name )const
{ try {
   return get< account_object, by_name >( name );
} FC_CAPTURE_AND_RETHROW( (name) ) }

const account_object* database::find_account( const account_name_type& name )const
{
   return find< account_object, by_name >( name );
}

const escrow_object& database::get_escrow( const account_name_type& name, uint32_t escrow_id )const
{ try {
   return get< escrow_object, by_from_id >( boost::make_tuple( name, escrow_id ) );
} FC_CAPTURE_AND_RETHROW( (name)(escrow_id) ) }

const escrow_object* database::find_escrow( const account_name_type& name, uint32_t escrow_id )const
{
   return find< escrow_object, by_from_id >( boost::make_tuple( name, escrow_id ) );
}

const dynamic_global_property_object&database::get_dynamic_global_properties() const
{ try {
   return get< dynamic_global_property_object >();
} FC_CAPTURE_AND_RETHROW() }

const node_property_object& database::get_node_properties() const
{
   return _node_property_object;
}

const feed_history_object& database::get_feed_history()const
{ try {
   return get< feed_history_object >();
} FC_CAPTURE_AND_RETHROW() }

const witness_schedule_object& database::get_witness_schedule_object()const
{ try {
   return get< witness_schedule_object >();
} FC_CAPTURE_AND_RETHROW() }

const hardfork_property_object& database::get_hardfork_property_object()const
{ try {
   return get< hardfork_property_object >();
} FC_CAPTURE_AND_RETHROW() }

void database::pay_fee( const account_object& account, asset fee )
{
   FC_ASSERT( fee.amount >= 0 ); /// NOTE if this fails then validate() on some operation is probably wrong
   if( fee.amount == 0 )
      return;

   FC_ASSERT( account.balance >= fee );
   adjust_balance( account, -fee );
   adjust_supply( -fee );
}

uint32_t database::witness_participation_rate()const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(STEEM_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
}

void database::add_checkpoints( const flat_map< uint32_t, block_id_type >& checkpts )
{
   for( const auto& i : checkpts )
      _checkpoints[i.first] = i.second;
}

bool database::before_last_checkpoint()const
{
   return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
}

/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
bool database::push_block(const signed_block& new_block, uint32_t skip)
{
   //fc::time_point begin_time = fc::time_point::now();

   bool result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      detail::without_pending_transactions( *this, std::move(_pending_tx), [&]()
      {
         try
         {
            result = _push_block(new_block);
         }
         FC_CAPTURE_AND_RETHROW( (new_block) )

         check_free_memory( false, new_block.block_num() );
      });
   });

   //fc::time_point end_time = fc::time_point::now();
   //fc::microseconds dt = end_time - begin_time;
   //if( ( new_block.block_num() % 10000 ) == 0 )
   //   ilog( "push_block ${b} took ${t} microseconds", ("b", new_block.block_num())("t", dt.count()) );
   return result;
}

void database::_maybe_warn_multiple_production( uint32_t height )const
{
   auto blocks = _fork_db.fetch_block_by_number( height );
   if( blocks.size() > 1 )
   {
      vector< std::pair< account_name_type, fc::time_point_sec > > witness_time_pairs;
      for( const auto& b : blocks )
      {
         witness_time_pairs.push_back( std::make_pair( b->data.witness, b->data.timestamp ) );
      }

      ilog( "Encountered block num collision at block ${n} due to a fork, witnesses are: ${w}", ("n", height)("w", witness_time_pairs) );
   }
   return;
}

bool database::_push_block(const signed_block& new_block)
{ try {
   #ifdef IS_TEST_NET
   FC_ASSERT(new_block.block_num() < TESTNET_BLOCK_LIMIT, "Testnet block limit exceeded");
   #endif /// IS_TEST_NET

   uint32_t skip = get_node_properties().skip_flags;
   //uint32_t skip_undo_db = skip & skip_undo_block;

   if( !(skip&skip_fork_db) )
   {
      shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
      _maybe_warn_multiple_production( new_head->num );

      //If the head block from the longest chain does not build off of the current head, we need to switch forks.
      if( new_head->data.previous != head_block_id() )
      {
         //If the newly pushed block is the same height as head, we get head back in new_head
         //Only switch forks if new_head is actually higher than head
         if( new_head->data.block_num() > head_block_num() )
         {
            // wlog( "Switching to fork: ${id}", ("id",new_head->data.id()) );
            auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());

            // pop blocks until we hit the forked block
            while( head_block_id() != branches.second.back()->data.previous )
               pop_block();

            // push all blocks on the new fork
            for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr )
            {
                // ilog( "pushing blocks from fork ${n} ${id}", ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()) );
                optional<fc::exception> except;
                try
                {
                   auto session = start_undo_session();
                   apply_block( (*ritr)->data, skip );
                   session.push();
                }
                catch ( const fc::exception& e ) { except = e; }
                if( except )
                {
                   // wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );
                   // remove the rest of branches.first from the fork_db, those blocks are invalid
                   while( ritr != branches.first.rend() )
                   {
                      _fork_db.remove( (*ritr)->data.id() );
                      ++ritr;
                   }
                   _fork_db.set_head( branches.second.front() );

                   // pop all blocks from the bad fork
                   while( head_block_id() != branches.second.back()->data.previous )
                      pop_block();

                   // restore all blocks from the good fork
                   for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr )
                   {
                      auto session = start_undo_session();
                      apply_block( (*ritr)->data, skip );
                      session.push();
                   }
                   throw *except;
                }
            }
            return true;
         }
         else
            return false;
      }
   }

   try
   {
      auto session = start_undo_session();
      apply_block(new_block, skip);
      session.push();
   }
   catch( const fc::exception& e )
   {
      elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
      _fork_db.remove(new_block.id());
      throw;
   }

   return false;
} FC_CAPTURE_AND_RETHROW() }

/**
 * Attempts to push the transaction into the pending queue
 *
 * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
 * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
 * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
 * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
 * queues.
 */
void database::push_transaction( const signed_transaction& trx, uint32_t skip )
{
   try
   {
      try
      {
         FC_ASSERT( fc::raw::pack_size(trx) <= (get_dynamic_global_properties().maximum_block_size - 256) );
         set_producing( true );
         detail::with_skip_flags( *this, skip,
            [&]()
            {
               _push_transaction( trx );
            });
         set_producing( false );
      }
      catch( ... )
      {
         set_producing( false );
         throw;
      }
   }
   FC_CAPTURE_AND_RETHROW( (trx) )
}

void database::_push_transaction( const signed_transaction& trx )
{
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if( !_pending_tx_session.valid() )
      _pending_tx_session = start_undo_session();

   // Create a temporary undo session as a child of _pending_tx_session.
   // The temporary session will be discarded by the destructor if
   // _apply_transaction fails.  If we make it to merge(), we
   // apply the changes.

   auto temp_session = start_undo_session();
   _apply_transaction( trx );
   _pending_tx.push_back( trx );

   notify_changed_objects();
   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   // notify anyone listening to pending transactions
   notify_on_pending_transaction( trx );
}

signed_block database::generate_block(
   fc::time_point_sec when,
   const account_name_type& witness_owner,
   const fc::ecc::private_key& block_signing_private_key,
   uint32_t skip /* = 0 */
   )
{
   signed_block result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      try
      {
         result = _generate_block( when, witness_owner, block_signing_private_key );
      }
      FC_CAPTURE_AND_RETHROW( (witness_owner) )
   });
   return result;
}


signed_block database::_generate_block(
   fc::time_point_sec when,
   const account_name_type& witness_owner,
   const fc::ecc::private_key& block_signing_private_key
   )
{
   uint32_t skip = get_node_properties().skip_flags;
   uint32_t slot_num = get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   string scheduled_witness = get_scheduled_witness( slot_num );
   FC_ASSERT( scheduled_witness == witness_owner );

   const auto& witness_obj = get_witness( witness_owner );

   if( !(skip & skip_witness_signature) )
      FC_ASSERT( witness_obj.signing_key == block_signing_private_key.get_public_key() );

   static const size_t max_block_header_size = fc::raw::pack_size( signed_block_header() ) + 4;
   auto maximum_block_size = get_dynamic_global_properties().maximum_block_size; //STEEM_MAX_BLOCK_SIZE;
   size_t total_block_size = max_block_header_size;

   signed_block pending_block;

   with_write_lock( [&]()
   {
      //
      // The following code throws away existing pending_tx_session and
      // rebuilds it by re-applying pending transactions.
      //
      // This rebuild is necessary because pending transactions' validity
      // and semantics may have changed since they were received, because
      // time-based semantics are evaluated based on the current block
      // time.  These changes can only be reflected in the database when
      // the value of the "when" variable is known, which means we need to
      // re-apply pending transactions in this method.
      //
      _pending_tx_session.reset();
      _pending_tx_session = start_undo_session();

      uint64_t postponed_tx_count = 0;
      // pop pending state (reset to head block state)
      for( const signed_transaction& tx : _pending_tx )
      {
         // Only include transactions that have not expired yet for currently generating block,
         // this should clear problem transactions and allow block production to continue

         if( tx.expiration < when )
            continue;

         uint64_t new_total_size = total_block_size + fc::raw::pack_size( tx );

         // postpone transaction if it would make block too big
         if( new_total_size >= maximum_block_size )
         {
            postponed_tx_count++;
            continue;
         }

         try
         {
            auto temp_session = start_undo_session();
            _apply_transaction( tx );
            temp_session.squash();

            total_block_size += fc::raw::pack_size( tx );
            pending_block.transactions.push_back( tx );
         }
         catch ( const fc::exception& e )
         {
            // Do nothing, transaction will not be re-applied
            //wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
            //wlog( "The transaction was ${t}", ("t", tx) );
         }
      }
      if( postponed_tx_count > 0 )
      {
         wlog( "Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count) );
      }

      _pending_tx_session.reset();
   });

   // We have temporarily broken the invariant that
   // _pending_tx_session is the result of applying _pending_tx, as
   // _pending_tx now consists of the set of postponed transactions.
   // However, the push_block() call below will re-create the
   // _pending_tx_session.

   pending_block.previous = head_block_id();
   pending_block.timestamp = when;
   pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
   pending_block.witness = witness_owner;
   {
      const auto& witness = get_witness( witness_owner );

      if( witness.running_version != STEEM_BLOCKCHAIN_VERSION )
         pending_block.extensions.insert( block_header_extensions( STEEM_BLOCKCHAIN_VERSION ) );

      const auto& hfp = get_hardfork_property_object();

      if( hfp.current_hardfork_version < STEEM_BLOCKCHAIN_VERSION // Binary is newer hardfork than has been applied
         && ( witness.hardfork_version_vote != _hardfork_versions[ hfp.last_hardfork + 1 ] || witness.hardfork_time_vote != _hardfork_times[ hfp.last_hardfork + 1 ] ) ) // Witness vote does not match binary configuration
      {
         // Make vote match binary configuration
         pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork + 1 ], _hardfork_times[ hfp.last_hardfork + 1 ] ) ) );
      }
      else if( hfp.current_hardfork_version == STEEM_BLOCKCHAIN_VERSION // Binary does not know of a new hardfork
         && witness.hardfork_version_vote > STEEM_BLOCKCHAIN_VERSION ) // Voting for hardfork in the future, that we do not know of...
      {
         // Make vote match binary configuration. This is vote to not apply the new hardfork.
         pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork ], _hardfork_times[ hfp.last_hardfork ] ) ) );
      }
   }

   if( !(skip & skip_witness_signature) )
      pending_block.sign( block_signing_private_key );

   // TODO:  Move this to _push_block() so session is restored.
   if( !(skip & skip_block_size_check) )
   {
      FC_ASSERT( fc::raw::pack_size(pending_block) <= STEEM_MAX_BLOCK_SIZE );
   }

   push_block( pending_block, skip );

   return pending_block;
}

/**
 * Removes the most recent block from the database and
 * undoes any changes it made.
 */
void database::pop_block()
{
   try
   {
      _pending_tx_session.reset();
      auto head_id = head_block_id();

      /// save the head block so we can recover its transactions
      optional<signed_block> head_block = fetch_block_by_id( head_id );
      STEEM_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );

      _fork_db.pop_block();
      undo();

      _popped_tx.insert( _popped_tx.begin(), head_block->transactions.begin(), head_block->transactions.end() );

   }
   FC_CAPTURE_AND_RETHROW()
}

void database::clear_pending()
{
   try
   {
      assert( (_pending_tx.size() == 0) || _pending_tx_session.valid() );
      _pending_tx.clear();
      _pending_tx_session.reset();
   }
   FC_CAPTURE_AND_RETHROW()
}

void database::notify_pre_apply_operation( operation_notification& note )
{
   note.trx_id       = _current_trx_id;
   note.block        = _current_block_num;
   note.trx_in_block = _current_trx_in_block;
   note.op_in_trx    = _current_op_in_trx;

   STEEM_TRY_NOTIFY( pre_apply_operation, note )
}

void database::notify_post_apply_operation( const operation_notification& note )
{
   STEEM_TRY_NOTIFY( post_apply_operation, note )
}

inline const void database::push_virtual_operation( const operation& op, bool force )
{
   /*
   if( !force )
   {
      #if defined( IS_LOW_MEM ) && ! defined( IS_TEST_NET )
      return;
      #endif
   }
   */

   FC_ASSERT( is_virtual_operation( op ) );
   operation_notification note(op);
   notify_pre_apply_operation( note );
   notify_post_apply_operation( note );
}

void database::notify_applied_block( const signed_block& block )
{
   STEEM_TRY_NOTIFY( applied_block, block )
}

void database::notify_on_pending_transaction( const signed_transaction& tx )
{
   STEEM_TRY_NOTIFY( on_pending_transaction, tx )
}

void database::notify_on_pre_apply_transaction( const signed_transaction& tx )
{
   STEEM_TRY_NOTIFY( on_pre_apply_transaction, tx )
}

void database::notify_on_applied_transaction( const signed_transaction& tx )
{
   STEEM_TRY_NOTIFY( on_applied_transaction, tx )
}

account_name_type database::get_scheduled_witness( uint32_t slot_num )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const witness_schedule_object& wso = get_witness_schedule_object();
   uint64_t current_aslot = dpo.current_aslot + slot_num;
   return wso.current_shuffled_witnesses[ current_aslot % wso.num_scheduled_witnesses ];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0 )
      return fc::time_point_sec();

   auto interval = STEEM_BLOCK_INTERVAL;
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      fc::time_point_sec genesis_time = dpo.time;
      return genesis_time + slot_num * interval;
   }

   int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
   fc::time_point_sec head_slot_time( head_block_abs_slot * interval );

   // "slot 0" is head_slot_time
   // "slot 1" is head_slot_time,
   //   plus maint interval if head block is a maint block
   //   plus block interval if head block is not a maint block
   return head_slot_time + (slot_num * interval);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / STEEM_BLOCK_INTERVAL + 1;
}


//TODO_SOPHIA - rework!!!
/**
 * @param to_account - the account to receive the new vesting shares
 * @param STEEM - STEEM to be converted to vesting shares
 */
asset database::create_vesting( const account_object& to_account, asset steem, bool to_reward_balance )
{
   try
   {
      const auto& cprops = get_dynamic_global_properties();

      /**
       *  The ratio of total_vesting_shares / total_vesting_fund_steem should not
       *  change as the result of the user adding funds
       *
       *  V / C  = (V+Vn) / (C+Cn)
       *
       *  Simplifies to Vn = (V * Cn ) / C
       *
       *  If Cn equals o.amount, then we must solve for Vn to know how many new vesting shares
       *  the user should receive.
       *
       *  128 bit math is requred due to multiplying of 64 bit numbers. This is done in asset and price.
       */
      asset new_vesting = steem * ( cprops.get_vesting_share_price() );

      modify( to_account, [&]( account_object& to )
      {
         to.vesting_shares += new_vesting;
      } );

      modify( cprops, [&]( dynamic_global_property_object& props )
      {
         props.total_vesting_shares += new_vesting;
      } );

      adjust_proxied_witness_votes(to_account, new_vesting.amount);

      return new_vesting;
   }
   FC_CAPTURE_AND_RETHROW( (to_account.name)(steem) )
}

void database::adjust_proxied_witness_votes( const account_object& a,
                                   const std::array< share_type, STEEM_MAX_PROXY_RECURSION_DEPTH+1 >& delta,
                                   int depth )
{
   if( a.proxy != STEEM_PROXY_TO_SELF_ACCOUNT )
   {
      /// nested proxies are not supported, vote will not propagate
      if( depth >= STEEM_MAX_PROXY_RECURSION_DEPTH )
         return;

      const auto& proxy = get_account( a.proxy );

      modify( proxy, [&]( account_object& a )
      {
         for( int i = STEEM_MAX_PROXY_RECURSION_DEPTH - depth - 1; i >= 0; --i )
         {
            a.proxied_vsf_votes[i+depth] += delta[i];
         }
      } );

      adjust_proxied_witness_votes( proxy, delta, depth + 1 );
   }
   else
   {
      share_type total_delta = 0;
      for( int i = STEEM_MAX_PROXY_RECURSION_DEPTH - depth; i >= 0; --i )
         total_delta += delta[i];
      adjust_witness_votes( a, total_delta );
   }
}

void database::adjust_proxied_witness_votes( const account_object& a, share_type delta, int depth )
{
   if( a.proxy != STEEM_PROXY_TO_SELF_ACCOUNT )
   {
      /// nested proxies are not supported, vote will not propagate
      if( depth >= STEEM_MAX_PROXY_RECURSION_DEPTH )
         return;

      const auto& proxy = get_account( a.proxy );

      modify( proxy, [&]( account_object& a )
      {
         a.proxied_vsf_votes[depth] += delta;
      } );

      adjust_proxied_witness_votes( proxy, delta, depth + 1 );
   }
   else
   {
     adjust_witness_votes( a, delta );
   }
}

void database::adjust_witness_votes( const account_object& a, share_type delta )
{
   const auto& vidx = get_index< witness_vote_index >().indices().get< by_account_witness >();
   auto itr = vidx.lower_bound( boost::make_tuple( a.name, account_name_type() ) );
   while( itr != vidx.end() && itr->account == a.name )
   {
      adjust_witness_vote( get< witness_object, by_name >(itr->witness), delta );
      ++itr;
   }
}

void database::adjust_witness_vote( const witness_object& witness, share_type delta )
{
   const witness_schedule_object& wso = get_witness_schedule_object();
   modify( witness, [&]( witness_object& w )
   {
      auto delta_pos = w.votes.value * (wso.current_virtual_time - w.virtual_last_update);
      w.virtual_position += delta_pos;

      w.virtual_last_update = wso.current_virtual_time;
      w.votes += delta;
      FC_ASSERT( w.votes <= get_dynamic_global_properties().total_vesting_shares.amount, "", ("w.votes", w.votes)("props",get_dynamic_global_properties().total_vesting_shares) );

      w.virtual_scheduled_time = w.virtual_last_update + (STEEM_VIRTUAL_SCHEDULE_LAP_LENGTH2 - w.virtual_position)/(w.votes.value+1);
      /** witnesses with a low number of votes could overflow the time field and end up with a scheduled time in the past */

      if( w.virtual_scheduled_time < wso.current_virtual_time )
         w.virtual_scheduled_time = fc::uint128::max_value();

   } );
}

void database::clear_witness_votes( const account_object& a )
{
   const auto& vidx = get_index< witness_vote_index >().indices().get<by_account_witness>();
   auto itr = vidx.lower_bound( boost::make_tuple( a.name, account_name_type() ) );
   while( itr != vidx.end() && itr->account == a.name )
   {
      const auto& current = *itr;
      ++itr;
      remove(current);
   }

   modify( a, [&](account_object& acc )
   {
        acc.witnesses_voted_for = 0;
   });
}

void database::clear_null_account_balance()
{

   const auto& null_account = get_account( STEEM_NULL_ACCOUNT );
   asset total_steem( 0, STEEM_SYMBOL );

   if( null_account.balance.amount > 0 )
   {
      total_steem += null_account.balance;
      adjust_balance( null_account, -null_account.balance );
   }

   if( null_account.vesting_shares.amount > 0 )
   {
      const auto& gpo = get_dynamic_global_properties();
      auto converted_steem = null_account.vesting_shares * gpo.get_vesting_share_price();

      modify( gpo, [&]( dynamic_global_property_object& g )
      {
         g.total_vesting_shares -= null_account.vesting_shares;
      });

      modify( null_account, [&]( account_object& a )
      {
         a.vesting_shares.amount = 0;
      });

      total_steem += converted_steem;
   }

   if( total_steem.amount > 0 )
      adjust_supply( -total_steem );

}

void database::update_owner_authority( const account_object& account, const authority& owner_authority )
{
   if( head_block_num() >= STEEM_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM )
   {
      create< owner_authority_history_object >( [&]( owner_authority_history_object& hist )
      {
         hist.account = account.name;
         hist.previous_owner_authority = get< account_authority_object, by_account >( account.name ).owner;
         hist.last_valid_time = head_block_time();
      });
   }

   modify( get< account_authority_object, by_account >( account.name ), [&]( account_authority_object& auth )
   {
      auth.owner = owner_authority;
      auth.last_owner_update = head_block_time();
   });
}

void database::process_vesting_withdrawals()
{
   const auto& widx = get_index< account_index, by_next_vesting_withdrawal >();
   auto current = widx.begin();

   const auto& cprops = get_dynamic_global_properties();

   while( current != widx.end() && current->next_vesting_withdrawal <= head_block_time() )
   {
      const auto& from_account = *current; ++current;

      /**
      *  Let T = total tokens in vesting fund
      *  Let V = total vesting shares
      *  Let v = total vesting shares being cashed out
      *
      *  The user may withdraw  vT / V tokens
      */
      share_type to_withdraw;
      if ( from_account.to_withdraw - from_account.withdrawn < from_account.vesting_withdraw_rate.amount )
         to_withdraw = std::min( from_account.vesting_shares.amount, from_account.to_withdraw % from_account.vesting_withdraw_rate.amount ).value;
      else
         to_withdraw = std::min( from_account.vesting_shares.amount, from_account.vesting_withdraw_rate.amount ).value;


      auto converted_steem = asset( to_withdraw, VESTS_SYMBOL ) * cprops.get_vesting_share_price();

      modify( from_account, [&]( account_object& a )
      {
         a.vesting_shares.amount -= to_withdraw;
         a.balance += converted_steem;
         a.withdrawn += to_withdraw;

         if( a.withdrawn >= a.to_withdraw || a.vesting_shares.amount == 0 )
         {
            a.vesting_withdraw_rate.amount = 0;
            a.next_vesting_withdrawal = fc::time_point_sec::maximum();
         }
         else
         {
            a.next_vesting_withdrawal += fc::seconds( STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
         }
      });

      modify( cprops, [&]( dynamic_global_property_object& o )
      {
         o.total_vesting_shares.amount -= to_withdraw;
      });

      //if( to_withdraw > 0 )
      //   adjust_proxied_witness_votes( from_account, -to_withdraw );

      push_virtual_operation( fill_vesting_withdraw_operation( from_account.name, from_account.name, asset( to_withdraw, VESTS_SYMBOL ), converted_steem ) );
   }
}

//TODO_SOPHIA - rework
/**
 *  Overall the network has an inflation rate of 102% of virtual steem per year
 *  90% of inflation is directed to vesting shares
 *  10% of inflation is directed to subjective proof of work voting
 *  1% of inflation is directed to liquidity providers
 *  1% of inflation is directed to block producers
 *
 *  This method pays out vesting and reward shares every block, and liquidity shares once per day.
 *  This method does not pay out witnesses.
 */
void database::process_funds()
{
 /*  const auto& props = get_dynamic_global_properties();
   const auto& wso = get_witness_schedule_object();

   {
      /**
       * At block 7,000,000 have a 9.5% instantaneous inflation rate, decreasing to 0.95% at a rate of 0.01%
       * every 250k blocks. This narrowing will take approximately 20.5 years and will complete on block 220,750,000

      int64_t start_inflation_rate = int64_t( STEEM_INFLATION_RATE_START_PERCENT );
      int64_t inflation_rate_adjustment = int64_t( head_block_num() / STEEM_INFLATION_NARROWING_PERIOD );
      int64_t inflation_rate_floor = int64_t( STEEM_INFLATION_RATE_STOP_PERCENT );

      // below subtraction cannot underflow int64_t because inflation_rate_adjustment is <2^32
      int64_t current_inflation_rate = std::max( start_inflation_rate - inflation_rate_adjustment, inflation_rate_floor );

      auto new_steem = ( props.virtual_supply.amount * current_inflation_rate ) / ( int64_t( STEEM_100_PERCENT ) * int64_t( STEEM_BLOCKS_PER_YEAR ) );
      auto content_reward = ( new_steem * STEEM_CONTENT_REWARD_PERCENT ) / STEEM_100_PERCENT;
      content_reward = pay_reward_funds( content_reward ); /// 75% to content creator
      auto vesting_reward = ( new_steem * STEEM_VESTING_FUND_PERCENT ) / STEEM_100_PERCENT; /// 15% to vesting fund
      auto witness_reward = new_steem - content_reward - vesting_reward; /// Remaining 10% to witness pay

      const auto& cwit = get_witness( props.current_witness );
      witness_reward *= STEEM_MAX_WITNESSES;

      if( cwit.schedule == witness_object::timeshare )
         witness_reward *= wso.timeshare_weight;
      else if( cwit.schedule == witness_object::top19 )
         witness_reward *= wso.top19_weight;
      else
         wlog( "Encountered unknown witness type for witness: ${w}", ("w", cwit.owner) );

      witness_reward /= wso.witness_pay_normalization_factor;

      new_steem = content_reward + vesting_reward + witness_reward;

      modify( props, [&]( dynamic_global_property_object& p )
      {
         p.current_supply           += asset( new_steem, STEEM_SYMBOL );
         p.virtual_supply           += asset( new_steem, STEEM_SYMBOL );
      });

      const auto& producer_reward = create_vesting( get_account( cwit.owner ), asset( witness_reward, STEEM_SYMBOL ) );
      push_virtual_operation( producer_reward_operation( cwit.owner, producer_reward ) );

   }*/

}

asset database::to_sbd( const asset& steem )const
{
   return util::to_sbd( get_feed_history().current_median_history, steem );
}

asset database::to_steem( const asset& sbd )const
{
   return util::to_steem( get_feed_history().current_median_history, sbd );
}

void database::account_recovery_processing()
{
   // Clear expired recovery requests
   const auto& rec_req_idx = get_index< account_recovery_request_index >().indices().get< by_expiration >();
   auto rec_req = rec_req_idx.begin();

   while( rec_req != rec_req_idx.end() && rec_req->expires <= head_block_time() )
   {
      remove( *rec_req );
      rec_req = rec_req_idx.begin();
   }

   // Clear invalid historical authorities
   const auto& hist_idx = get_index< owner_authority_history_index >().indices(); //by id
   auto hist = hist_idx.begin();

   while( hist != hist_idx.end() && time_point_sec( hist->last_valid_time + STEEM_OWNER_AUTH_RECOVERY_PERIOD ) < head_block_time() )
   {
      remove( *hist );
      hist = hist_idx.begin();
   }

   // Apply effective recovery_account changes
   const auto& change_req_idx = get_index< change_recovery_account_request_index >().indices().get< by_effective_date >();
   auto change_req = change_req_idx.begin();

   while( change_req != change_req_idx.end() && change_req->effective_on <= head_block_time() )
   {
      modify( get_account( change_req->account_to_recover ), [&]( account_object& a )
      {
         a.recovery_account = change_req->recovery_account;
      });

      remove( *change_req );
      change_req = change_req_idx.begin();
   }
}

void database::expire_escrow_ratification()
{
   const auto& escrow_idx = get_index< escrow_index >().indices().get< by_ratification_deadline >();
   auto escrow_itr = escrow_idx.lower_bound( false );

   while( escrow_itr != escrow_idx.end() && !escrow_itr->is_approved() && escrow_itr->ratification_deadline <= head_block_time() )
   {
      const auto& old_escrow = *escrow_itr;
      ++escrow_itr;

      adjust_balance( old_escrow.from, old_escrow.steem_balance );
      adjust_balance( old_escrow.from, old_escrow.pending_fee );

      remove( old_escrow );
   }
}

time_point_sec database::head_block_time()const
{
   return get_dynamic_global_properties().time;
}

uint32_t database::head_block_num()const
{
   return get_dynamic_global_properties().head_block_number;
}

block_id_type database::head_block_id()const
{
   return get_dynamic_global_properties().head_block_id;
}

node_property_object& database::node_properties()
{
   return _node_property_object;
}

uint32_t database::last_non_undoable_block_num() const
{
   return get_dynamic_global_properties().last_irreversible_block_num;
}

void database::initialize_evaluators()
{
   _my->_evaluator_registry.register_evaluator< transfer_evaluator                       >();
   _my->_evaluator_registry.register_evaluator< transfer_to_vesting_evaluator            >();
   _my->_evaluator_registry.register_evaluator< withdraw_vesting_evaluator               >();
   _my->_evaluator_registry.register_evaluator< account_create_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< account_update_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< witness_update_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< account_witness_vote_evaluator           >();
   _my->_evaluator_registry.register_evaluator< account_witness_proxy_evaluator          >();
   _my->_evaluator_registry.register_evaluator< custom_evaluator                         >();
   _my->_evaluator_registry.register_evaluator< custom_binary_evaluator                  >();
   _my->_evaluator_registry.register_evaluator< custom_json_evaluator                    >();
   _my->_evaluator_registry.register_evaluator< report_over_production_evaluator         >();
   _my->_evaluator_registry.register_evaluator< feed_publish_evaluator                   >();
   _my->_evaluator_registry.register_evaluator< placeholder_a_evaluator                  >();
   _my->_evaluator_registry.register_evaluator< placeholder_b_evaluator                  >();
   _my->_evaluator_registry.register_evaluator< request_account_recovery_evaluator       >();
   _my->_evaluator_registry.register_evaluator< recover_account_evaluator                >();
   _my->_evaluator_registry.register_evaluator< change_recovery_account_evaluator        >();
   _my->_evaluator_registry.register_evaluator< escrow_transfer_evaluator                >();
   _my->_evaluator_registry.register_evaluator< escrow_approve_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< escrow_dispute_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< escrow_release_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< reset_account_evaluator                  >();
   _my->_evaluator_registry.register_evaluator< set_reset_account_evaluator              >();
#ifdef STEEM_ENABLE_SMT
   _my->_evaluator_registry.register_evaluator< claim_reward_balance2_evaluator          >();
#endif
   _my->_evaluator_registry.register_evaluator< witness_set_properties_evaluator         >();

#ifdef STEEM_ENABLE_SMT
   _my->_evaluator_registry.register_evaluator< smt_setup_evaluator                      >();
   _my->_evaluator_registry.register_evaluator< smt_cap_reveal_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< smt_refund_evaluator                     >();
   _my->_evaluator_registry.register_evaluator< smt_setup_emissions_evaluator            >();
   _my->_evaluator_registry.register_evaluator< smt_set_setup_parameters_evaluator       >();
   _my->_evaluator_registry.register_evaluator< smt_set_runtime_parameters_evaluator     >();
   _my->_evaluator_registry.register_evaluator< smt_create_evaluator                     >();
#endif
}


void database::set_custom_operation_interpreter( const uint32_t id, std::shared_ptr< custom_operation_interpreter > registry )
{
   bool inserted = _custom_operation_interpreters.emplace( id, registry ).second;
   // This assert triggering means we're mis-configured (multiple registrations of custom JSON evaluator for same ID)
   FC_ASSERT( inserted );
}

std::shared_ptr< custom_operation_interpreter > database::get_custom_json_evaluator( const uint32_t id )
{
   auto it = _custom_operation_interpreters.find( id );
   if( it != _custom_operation_interpreters.end() )
      return it->second;
   return std::shared_ptr< custom_operation_interpreter >();
}

void database::initialize_indexes()
{
   add_core_index< dynamic_global_property_index           >(*this);
   add_core_index< account_index                           >(*this);
   add_core_index< account_authority_index                 >(*this);
   add_core_index< witness_index                           >(*this);
   add_core_index< transaction_index                       >(*this);
   add_core_index< block_summary_index                     >(*this);
   add_core_index< witness_schedule_index                  >(*this);
   add_core_index< witness_vote_index                      >(*this);
   add_core_index< feed_history_index                      >(*this);
   add_core_index< operation_index                         >(*this);
   add_core_index< account_history_index                   >(*this);
   add_core_index< hardfork_property_index                 >(*this);
   add_core_index< owner_authority_history_index           >(*this);
   add_core_index< account_recovery_request_index          >(*this);
   add_core_index< change_recovery_account_request_index   >(*this);
   add_core_index< escrow_index                            >(*this);
   add_core_index< reward_fund_index                       >(*this);
#ifdef STEEM_ENABLE_SMT
   add_core_index< smt_token_index                         >(*this);
   add_core_index< account_regular_balance_index           >(*this);
   add_core_index< account_rewards_balance_index           >(*this);
#endif

   _plugin_index_signal();
}

const std::string& database::get_json_schema()const
{
   return _json_schema;
}

void database::init_schema()
{
   /*done_adding_indexes();

   db_schema ds;

   std::vector< std::shared_ptr< abstract_schema > > schema_list;

   std::vector< object_schema > object_schemas;
   get_object_schemas( object_schemas );

   for( const object_schema& oschema : object_schemas )
   {
      ds.object_types.emplace_back();
      ds.object_types.back().space_type.first = oschema.space_id;
      ds.object_types.back().space_type.second = oschema.type_id;
      oschema.schema->get_name( ds.object_types.back().type );
      schema_list.push_back( oschema.schema );
   }

   std::shared_ptr< abstract_schema > operation_schema = get_schema_for_type< operation >();
   operation_schema->get_name( ds.operation_type );
   schema_list.push_back( operation_schema );

   for( const std::pair< std::string, std::shared_ptr< custom_operation_interpreter > >& p : _custom_operation_interpreters )
   {
      ds.custom_operation_types.emplace_back();
      ds.custom_operation_types.back().id = p.first;
      schema_list.push_back( p.second->get_operation_schema() );
      schema_list.back()->get_name( ds.custom_operation_types.back().type );
   }

   graphene::db::add_dependent_schemas( schema_list );
   std::sort( schema_list.begin(), schema_list.end(),
      []( const std::shared_ptr< abstract_schema >& a,
          const std::shared_ptr< abstract_schema >& b )
      {
         return a->id < b->id;
      } );
   auto new_end = std::unique( schema_list.begin(), schema_list.end(),
      []( const std::shared_ptr< abstract_schema >& a,
          const std::shared_ptr< abstract_schema >& b )
      {
         return a->id == b->id;
      } );
   schema_list.erase( new_end, schema_list.end() );

   for( std::shared_ptr< abstract_schema >& s : schema_list )
   {
      std::string tname;
      s->get_name( tname );
      FC_ASSERT( ds.types.find( tname ) == ds.types.end(), "types with different ID's found for name ${tname}", ("tname", tname) );
      std::string ss;
      s->get_str_schema( ss );
      ds.types.emplace( tname, ss );
   }

   _json_schema = fc::json::to_string( ds );
   return;*/
}

void database::init_genesis( uint64_t init_supply )
{
   try
   {
      struct auth_inhibitor
      {
         auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags)
         { db.node_properties().skip_flags |= skip_authority_check; }
         ~auth_inhibitor()
         { db.node_properties().skip_flags = old_flags; }
      private:
         database& db;
         uint32_t old_flags;
      } inhibitor(*this);

      // Create blockchain accounts
      public_key_type      init_public_key(STEEM_INIT_PUBLIC_KEY);

      create< account_object >( [&]( account_object& a )
      {
         a.name = STEEM_MINER_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = STEEM_MINER_ACCOUNT;
         auth.owner.weight_threshold = 1;
         auth.active.weight_threshold = 1;
      });

      create< account_object >( [&]( account_object& a )
      {
         a.name = STEEM_NULL_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = STEEM_NULL_ACCOUNT;
         auth.owner.weight_threshold = 1;
         auth.active.weight_threshold = 1;
      });

      create< account_object >( [&]( account_object& a )
      {
         a.name = STEEM_TEMP_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = STEEM_TEMP_ACCOUNT;
         auth.owner.weight_threshold = 0;
         auth.active.weight_threshold = 0;
      });

      for( int i = 0; i < STEEM_NUM_INIT_MINERS; ++i )
      {
         create< account_object >( [&]( account_object& a )
         {
            a.name = STEEM_INIT_MINER_NAME + ( i ? fc::to_string( i ) : std::string() );
            a.memo_key = init_public_key;
            a.balance  = asset( i ? 0 : init_supply, STEEM_SYMBOL );
            elog("initializing initminer account ${a} with ${bal} steems, a.balance is ${abal}",("a", a)("bal", init_supply)("abal",a.balance.amount));
         } );

         create< account_authority_object >( [&]( account_authority_object& auth )
         {
            auth.account = STEEM_INIT_MINER_NAME + ( i ? fc::to_string( i ) : std::string() );
            auth.owner.add_authority( init_public_key, 1 );
            auth.owner.weight_threshold = 1;
            auth.active  = auth.owner;
         });

         create< witness_object >( [&]( witness_object& w )
         {
            w.owner        = STEEM_INIT_MINER_NAME + ( i ? fc::to_string(i) : std::string() );
            w.signing_key  = init_public_key;
            w.schedule = witness_object::top19;
         } );
      }

      create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p )
      {
         p.current_witness = STEEM_INIT_MINER_NAME;
         p.time = STEEM_GENESIS_TIME;
         p.recent_slots_filled = fc::uint128::max_value();
         p.participation_count = 128;
         p.current_supply = asset( init_supply, STEEM_SYMBOL );
         p.virtual_supply = p.current_supply;
         p.maximum_block_size = STEEM_MAX_BLOCK_SIZE;
      } );

      // Nothing to do
      create< feed_history_object >( [&]( feed_history_object& o ) {});
      for( int i = 0; i < 0x10000; i++ )
         create< block_summary_object >( [&]( block_summary_object& ) {});
      create< hardfork_property_object >( [&](hardfork_property_object& hpo )
      {
         hpo.processed_hardforks.push_back( STEEM_GENESIS_TIME );
      } );

      // Create witness scheduler
      create< witness_schedule_object >( [&]( witness_schedule_object& wso )
      {
         wso.current_shuffled_witnesses[0] = STEEM_INIT_MINER_NAME;
      } );
   }
   FC_CAPTURE_AND_RETHROW()
}


void database::validate_transaction( const signed_transaction& trx )
{
   database::with_write_lock( [&]()
   {
      auto session = start_undo_session();
      _apply_transaction( trx );
      session.undo();
   });
}

void database::notify_changed_objects()
{
   try
   {
      /*vector< chainbase::generic_id > ids;
      get_changed_ids( ids );
      STEEM_TRY_NOTIFY( changed_objects, ids )*/
      /*
      if( _undo_db.enabled() )
      {
         const auto& head_undo = _undo_db.head();
         vector<object_id_type> changed_ids;  changed_ids.reserve(head_undo.old_values.size());
         for( const auto& item : head_undo.old_values ) changed_ids.push_back(item.first);
         for( const auto& item : head_undo.new_ids ) changed_ids.push_back(item);
         vector<const object*> removed;
         removed.reserve( head_undo.removed.size() );
         for( const auto& item : head_undo.removed )
         {
            changed_ids.push_back( item.first );
            removed.emplace_back( item.second.get() );
         }
         STEEM_TRY_NOTIFY( changed_objects, changed_ids )
      }
      */
   }
   FC_CAPTURE_AND_RETHROW()

}

void database::set_flush_interval( uint32_t flush_blocks )
{
   _flush_blocks = flush_blocks;
   _next_flush_block = 0;
}

//////////////////// private methods ////////////////////

void database::apply_block( const signed_block& next_block, uint32_t skip )
{ try {
   //fc::time_point begin_time = fc::time_point::now();

   auto block_num = next_block.block_num();
   if( _checkpoints.size() && _checkpoints.rbegin()->second != block_id_type() )
   {
      auto itr = _checkpoints.find( block_num );
      if( itr != _checkpoints.end() )
         FC_ASSERT( next_block.id() == itr->second, "Block did not match checkpoint", ("checkpoint",*itr)("block_id",next_block.id()) );

      if( _checkpoints.rbegin()->first >= block_num )
         skip = skip_witness_signature
              | skip_transaction_signatures
              | skip_transaction_dupe_check
              | skip_fork_db
              | skip_block_size_check
              | skip_tapos_check
              | skip_authority_check
              /* | skip_merkle_check While blockchain is being downloaded, txs need to be validated against block headers */
              | skip_undo_history_check
              | skip_witness_schedule_check
              | skip_validate
              | skip_validate_invariants
              ;
   }

   detail::with_skip_flags( *this, skip, [&]()
   {
      _apply_block( next_block );
   } );

   /*try
   {
   /// check invariants
   if( is_producing() || !( skip & skip_validate_invariants ) )
      validate_invariants();
   }
   FC_CAPTURE_AND_RETHROW( (next_block) );*/

   //fc::time_point end_time = fc::time_point::now();
   //fc::microseconds dt = end_time - begin_time;
   if( _flush_blocks != 0 )
   {
      if( _next_flush_block == 0 )
      {
         uint32_t lep = block_num + 1 + _flush_blocks * 9 / 10;
         uint32_t rep = block_num + 1 + _flush_blocks;

         // use time_point::now() as RNG source to pick block randomly between lep and rep
         uint32_t span = rep - lep;
         uint32_t x = lep;
         if( span > 0 )
         {
            uint64_t now = uint64_t( fc::time_point::now().time_since_epoch().count() );
            x += now % span;
         }
         _next_flush_block = x;
         //ilog( "Next flush scheduled at block ${b}", ("b", x) );
      }

      if( _next_flush_block == block_num )
      {
         _next_flush_block = 0;
         //ilog( "Flushing database shared memory at block ${b}", ("b", block_num) );
         chainbase::database::flush();
      }
   }

} FC_CAPTURE_AND_RETHROW( (next_block) ) }

void database::check_free_memory( bool force_print, uint32_t current_block_num )
{
   uint64_t free_mem = get_free_memory();
   uint64_t max_mem = get_max_memory();

   if( BOOST_UNLIKELY( _shared_file_full_threshold != 0 && _shared_file_scale_rate != 0 && free_mem < ( ( uint128_t( STEEM_100_PERCENT - _shared_file_full_threshold ) * max_mem ) / STEEM_100_PERCENT ).to_uint64() ) )
   {
      uint64_t new_max = ( uint128_t( max_mem * _shared_file_scale_rate ) / STEEM_100_PERCENT ).to_uint64() + max_mem;

      wlog( "Memory is almost full, increasing to ${mem}M", ("mem", new_max / (1024*1024)) );

      resize( new_max );

      uint32_t free_mb = uint32_t( get_free_memory() / (1024*1024) );
      wlog( "Free memory is now ${free}M", ("free", free_mb) );
      _last_free_gb_printed = free_mb / 1024;
   }
   else
   {
      uint32_t free_gb = uint32_t( free_mem / (1024*1024*1024) );
      if( BOOST_UNLIKELY( force_print || (free_gb < _last_free_gb_printed) || (free_gb > _last_free_gb_printed+1) ) )
      {
         ilog( "Free memory is now ${n}G. Current block number: ${block}", ("n", free_gb)("block",current_block_num) );
         _last_free_gb_printed = free_gb;
      }

      if( BOOST_UNLIKELY( free_gb == 0 ) )
      {
         uint32_t free_mb = uint32_t( free_mem / (1024*1024) );

   #ifdef IS_TEST_NET
      if( !disable_low_mem_warning )
   #endif
         if( free_mb <= 100 && head_block_num() % 10 == 0 )
            elog( "Free memory is now ${n}M. Increase shared file size immediately!" , ("n", free_mb) );
      }
   }
}

void database::_apply_block( const signed_block& next_block )
{ try {
   uint32_t next_block_num = next_block.block_num();
   //block_id_type next_block_id = next_block.id();

   uint32_t skip = get_node_properties().skip_flags;

   if( BOOST_UNLIKELY( next_block_num == 1 ) )
   {
      // For every existing before the head_block_time (genesis time), apply the hardfork
      // This allows the test net to launch with past hardforks and apply the next harfork when running

      uint32_t n;
      for( n=0; n<STEEM_NUM_HARDFORKS; n++ )
      {
         if( _hardfork_times[n+1] > next_block.timestamp )
            break;
      }

      if( n > 0 )
      {
         ilog( "Processing ${n} genesis hardforks", ("n", n) );
         set_hardfork( n, true );

         const hardfork_property_object& hardfork_state = get_hardfork_property_object();
         FC_ASSERT( hardfork_state.current_hardfork_version == _hardfork_versions[n], "Unexpected genesis hardfork state" );

         const auto& witness_idx = get_index<witness_index>().indices().get<by_id>();
         vector<witness_id_type> wit_ids_to_update;
         for( auto it=witness_idx.begin(); it!=witness_idx.end(); ++it )
            wit_ids_to_update.push_back(it->id);

         for( witness_id_type wit_id : wit_ids_to_update )
         {
            modify( get( wit_id ), [&]( witness_object& wit )
            {
               wit.running_version = _hardfork_versions[n];
               wit.hardfork_version_vote = _hardfork_versions[n];
               wit.hardfork_time_vote = _hardfork_times[n];
            } );
         }
      }
   }

   if( !( skip & skip_merkle_check ) )
   {
      auto merkle_root = next_block.calculate_merkle_root();

      try
      {
         FC_ASSERT( next_block.transaction_merkle_root == merkle_root, "Merkle check failed", ("next_block.transaction_merkle_root",next_block.transaction_merkle_root)("calc",merkle_root)("next_block",next_block)("id",next_block.id()) );
      }
      catch( fc::assert_exception& e )
      {
         const auto& merkle_map = get_shared_db_merkle();
         auto itr = merkle_map.find( next_block_num );

         if( itr == merkle_map.end() || itr->second != merkle_root )
            throw e;
      }
   }

   const witness_object& signing_witness = validate_block_header(skip, next_block);

   _current_block_num    = next_block_num;
   _current_trx_in_block = 0;

   const auto& gprops = get_dynamic_global_properties();
   auto block_size = fc::raw::pack_size( next_block );
   FC_ASSERT( block_size <= gprops.maximum_block_size, "Block Size is too Big", ("next_block_num",next_block_num)("block_size", block_size)("max",gprops.maximum_block_size) );


   if( block_size < STEEM_MIN_BLOCK_SIZE )
   {
      elog( "Block size is too small",
         ("next_block_num",next_block_num)("block_size", block_size)("min",STEEM_MIN_BLOCK_SIZE)
      );
   }

   /// modify current witness so transaction evaluators can know who included the transaction,
   /// this is mostly for POW operations which must pay the current_witness
   modify( gprops, [&]( dynamic_global_property_object& dgp ){
      dgp.current_witness = next_block.witness;
   });

   /// parse witness version reporting
   process_header_extensions( next_block );

   const auto& witness = get_witness( next_block.witness );
   const auto& hardfork_state = get_hardfork_property_object();
   FC_ASSERT( witness.running_version >= hardfork_state.current_hardfork_version,
         "Block produced by witness that is not running current hardfork",
         ("witness",witness)("next_block.witness",next_block.witness)("hardfork_state", hardfork_state)
      );


   for( const auto& trx : next_block.transactions )
   {
      /* We do not need to push the undo state for each transaction
       * because they either all apply and are valid or the
       * entire block fails to apply.  We only need an "undo" state
       * for transactions when validating broadcast transactions or
       * when building a block.
       */
      apply_transaction( trx, skip );
      ++_current_trx_in_block;
   }

   update_global_dynamic_data(next_block);
   update_signing_witness(signing_witness, next_block);

   update_last_irreversible_block();

   create_block_summary(next_block);
   clear_expired_transactions();
   update_witness_schedule(*this);

   update_median_feed();
   update_virtual_supply();

   clear_null_account_balance();
   process_funds();
   process_vesting_withdrawals();
   update_virtual_supply();

   account_recovery_processing();
   expire_escrow_ratification();

   process_hardforks();

   // notify observers that the block has been applied
   notify_applied_block( next_block );

   notify_changed_objects();
} //FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }
FC_CAPTURE_LOG_AND_RETHROW( (next_block.block_num()) )
}

struct process_header_visitor
{
   process_header_visitor( const std::string& witness, database& db ) : _witness( witness ), _db( db ) {}

   typedef void result_type;

   const std::string& _witness;
   database& _db;

   void operator()( const void_t& obj ) const
   {
      //Nothing to do.
   }

   void operator()( const version& reported_version ) const
   {
      const auto& signing_witness = _db.get_witness( _witness );
      //idump( (next_block.witness)(signing_witness.running_version)(reported_version) );

      if( reported_version != signing_witness.running_version )
      {
         _db.modify( signing_witness, [&]( witness_object& wo )
         {
            wo.running_version = reported_version;
         });
      }
   }

   void operator()( const hardfork_version_vote& hfv ) const
   {
      const auto& signing_witness = _db.get_witness( _witness );
      //idump( (next_block.witness)(signing_witness.running_version)(hfv) );

      if( hfv.hf_version != signing_witness.hardfork_version_vote || hfv.hf_time != signing_witness.hardfork_time_vote )
         _db.modify( signing_witness, [&]( witness_object& wo )
         {
            wo.hardfork_version_vote = hfv.hf_version;
            wo.hardfork_time_vote = hfv.hf_time;
         });
   }

   template<typename T>
   void operator()( const T& unknown_obj ) const
   {
      FC_ASSERT( false, "Unknown extension in block header" );
   }
};

void database::process_header_extensions( const signed_block& next_block )
{
   process_header_visitor _v( next_block.witness, *this );

   for( const auto& e : next_block.extensions )
      e.visit( _v );
}

void database::update_median_feed() {
try {
   if( (head_block_num() % STEEM_FEED_INTERVAL_BLOCKS) != 0 )
      return;

   auto now = head_block_time();
   const witness_schedule_object& wso = get_witness_schedule_object();
   vector<price> feeds; feeds.reserve( wso.num_scheduled_witnesses );
   for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
   {
      const auto& wit = get_witness( wso.current_shuffled_witnesses[i] );
      {
         if( now < wit.last_sbd_exchange_update + STEEM_MAX_FEED_AGE_SECONDS
            && !wit.sbd_exchange_rate.is_null() )
         {
            feeds.push_back( wit.sbd_exchange_rate );
         }
      }
   }

   if( feeds.size() >= STEEM_MIN_FEEDS )
   {
      std::sort( feeds.begin(), feeds.end() );
      auto median_feed = feeds[feeds.size()/2];

      modify( get_feed_history(), [&]( feed_history_object& fho )
      {
         fho.price_history.push_back( median_feed );
         size_t steem_feed_history_window = STEEM_FEED_HISTORY_WINDOW_PRE_HF_16;
         steem_feed_history_window = STEEM_FEED_HISTORY_WINDOW;

         if( fho.price_history.size() > steem_feed_history_window )
            fho.price_history.pop_front();

         if( fho.price_history.size() )
         {
            /// BW-TODO Why deque is used here ? Also why don't make copy of whole container ?
            std::deque< price > copy;
            for( const auto& i : fho.price_history )
            {
               copy.push_back( i );
            }

            std::sort( copy.begin(), copy.end() ); /// TODO: use nth_item
            fho.current_median_history = copy[copy.size()/2];

#ifdef IS_TEST_NET
            if( skip_price_feed_limit_check )
               return;
#endif
         }
      });
   }
} FC_CAPTURE_AND_RETHROW() }

void database::apply_transaction(const signed_transaction& trx, uint32_t skip)
{
   detail::with_skip_flags( *this, skip, [&]() { _apply_transaction(trx); });
   notify_on_applied_transaction( trx );
}

void database::_apply_transaction(const signed_transaction& trx)
{ try {
   _current_trx_id = trx.id();
   uint32_t skip = get_node_properties().skip_flags;

   if( !(skip&skip_validate) )   /* issue #505 explains why this skip_flag is disabled */
      trx.validate();

   auto& trx_idx = get_index<transaction_index>();
   const chain_id_type& chain_id = get_chain_id();
   auto trx_id = trx.id();
   // idump((trx_id)(skip&skip_transaction_dupe_check));
   FC_ASSERT( (skip & skip_transaction_dupe_check) ||
              trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end(),
              "Duplicate transaction check failed", ("trx_ix", trx_id) );

   if( !(skip & (skip_transaction_signatures | skip_authority_check) ) )
   {
      auto get_active  = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).active ); };
      auto get_owner   = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).owner );  };

      try
      {
         trx.verify_authority( chain_id, get_active, get_owner, STEEM_MAX_SIG_CHECK_DEPTH );
      }
      catch( protocol::tx_missing_active_auth& e )
      {
         if( get_shared_db_merkle().find( head_block_num() + 1 ) == get_shared_db_merkle().end() )
            throw e;
      }
   }

   //Skip all manner of expiration and TaPoS checking if we're on block 1; It's impossible that the transaction is
   //expired, and TaPoS makes no sense as no blocks exist.
   if( BOOST_LIKELY(head_block_num() > 0) )
   {
      if( !(skip & skip_tapos_check) )
      {
         const auto& tapos_block_summary = get< block_summary_object >( trx.ref_block_num );
         //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
         STEEM_ASSERT( trx.ref_block_prefix == tapos_block_summary.block_id._hash[1], transaction_tapos_exception,
                    "", ("trx.ref_block_prefix", trx.ref_block_prefix)
                    ("tapos_block_summary",tapos_block_summary.block_id._hash[1]));
      }

      fc::time_point_sec now = head_block_time();

      STEEM_ASSERT( trx.expiration <= now + fc::seconds(STEEM_MAX_TIME_UNTIL_EXPIRATION), transaction_expiration_exception,
                  "", ("trx.expiration",trx.expiration)("now",now)("max_til_exp",STEEM_MAX_TIME_UNTIL_EXPIRATION));
      STEEM_ASSERT( now < trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
      STEEM_ASSERT( now <= trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
   }

   //Insert transaction into unique transactions database.
   if( !(skip & skip_transaction_dupe_check) )
   {
      create<transaction_object>([&](transaction_object& transaction) {
         transaction.trx_id = trx_id;
         transaction.expiration = trx.expiration;
         fc::raw::pack_to_buffer( transaction.packed_trx, trx );
      });
   }

   notify_on_pre_apply_transaction( trx );

   //Finally process the operations
   _current_op_in_trx = 0;
   for( const auto& op : trx.operations )
   { try {
      apply_operation(op);
      ++_current_op_in_trx;
     } FC_CAPTURE_AND_RETHROW( (op) );
   }
   _current_trx_id = transaction_id_type();

} FC_CAPTURE_AND_RETHROW( (trx) ) }

void database::apply_operation(const operation& op)
{
   operation_notification note(op);
   notify_pre_apply_operation( note );
   _my->_evaluator_registry.get_evaluator( op ).apply( op );
   notify_post_apply_operation( note );
}

const witness_object& database::validate_block_header( uint32_t skip, const signed_block& next_block )const
{ try {
   FC_ASSERT( head_block_id() == next_block.previous, "", ("head_block_id",head_block_id())("next.prev",next_block.previous) );
   FC_ASSERT( head_block_time() < next_block.timestamp, "", ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()) );
   const witness_object& witness = get_witness( next_block.witness );

   if( !(skip&skip_witness_signature) )
      FC_ASSERT( next_block.validate_signee( witness.signing_key ) );

   if( !(skip&skip_witness_schedule_check) )
   {
      uint32_t slot_num = get_slot_at_time( next_block.timestamp );
      FC_ASSERT( slot_num > 0 );

      string scheduled_witness = get_scheduled_witness( slot_num );

      FC_ASSERT( witness.owner == scheduled_witness, "Witness produced block at wrong time",
                 ("block witness",next_block.witness)("scheduled",scheduled_witness)("slot_num",slot_num) );
   }

   return witness;
} FC_CAPTURE_AND_RETHROW() }

void database::create_block_summary(const signed_block& next_block)
{ try {
   block_summary_id_type sid( next_block.block_num() & 0xffff );
   modify( get< block_summary_object >( sid ), [&](block_summary_object& p) {
         p.block_id = next_block.id();
   });
} FC_CAPTURE_AND_RETHROW() }

void database::update_global_dynamic_data( const signed_block& b )
{ try {
   const dynamic_global_property_object& _dgp =
      get_dynamic_global_properties();

   uint32_t missed_blocks = 0;
   if( head_block_time() != fc::time_point_sec() )
   {
      missed_blocks = get_slot_at_time( b.timestamp );
      assert( missed_blocks != 0 );
      missed_blocks--;
      for( uint32_t i = 0; i < missed_blocks; ++i )
      {
         const auto& witness_missed = get_witness( get_scheduled_witness( i + 1 ) );
         if(  witness_missed.owner != b.witness )
         {
            modify( witness_missed, [&]( witness_object& w )
            {
               w.total_missed++;

               if( head_block_num() - w.last_confirmed_block_num  > STEEM_BLOCKS_PER_DAY )
               {
                  w.signing_key = public_key_type();
                  push_virtual_operation( shutdown_witness_operation( w.owner ) );
               }

            } );
         }
      }
   }

   // dynamic global properties updating
   modify( _dgp, [&]( dynamic_global_property_object& dgp )
   {
      // This is constant time assuming 100% participation. It is O(B) otherwise (B = Num blocks between update)
      for( uint32_t i = 0; i < missed_blocks + 1; i++ )
      {
         dgp.participation_count -= dgp.recent_slots_filled.hi & 0x8000000000000000ULL ? 1 : 0;
         dgp.recent_slots_filled = ( dgp.recent_slots_filled << 1 ) + ( i == 0 ? 1 : 0 );
         dgp.participation_count += ( i == 0 ? 1 : 0 );
      }

      dgp.head_block_number = b.block_num();
      dgp.head_block_id = b.id();
      dgp.time = b.timestamp;
      dgp.current_aslot += missed_blocks+1;
   } );

   if( !(get_node_properties().skip_flags & skip_undo_history_check) )
   {
      STEEM_ASSERT( _dgp.head_block_number - _dgp.last_irreversible_block_num  < STEEM_MAX_UNDO_HISTORY, undo_database_exception,
                 "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                 "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                 ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)
                 ("max_undo",STEEM_MAX_UNDO_HISTORY) );
   }
} FC_CAPTURE_AND_RETHROW() }

//TODO_SOPHIA - Rework
void database::update_virtual_supply()
{ try {
   modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& dgp )
   {
      dgp.virtual_supply = dgp.current_supply;
   });
} FC_CAPTURE_AND_RETHROW() }

void database::update_signing_witness(const witness_object& signing_witness, const signed_block& new_block)
{ try {
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time( new_block.timestamp );

   modify( signing_witness, [&]( witness_object& _wit )
   {
      _wit.last_aslot = new_block_aslot;
      _wit.last_confirmed_block_num = new_block.block_num();
   } );
} FC_CAPTURE_AND_RETHROW() }

void database::update_last_irreversible_block()
{ try {
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   /**
    * Prior to voting taking over, we must be more conservative...
    *
    */
   if( head_block_num() < STEEM_START_MINER_VOTING_BLOCK )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         if ( head_block_num() > STEEM_MAX_WITNESSES )
            _dpo.last_irreversible_block_num = head_block_num() - STEEM_MAX_WITNESSES;
      } );
   }
   else
   {
      const witness_schedule_object& wso = get_witness_schedule_object();

      vector< const witness_object* > wit_objs;
      wit_objs.reserve( wso.num_scheduled_witnesses );
      for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
         wit_objs.push_back( &get_witness( wso.current_shuffled_witnesses[i] ) );

      static_assert( STEEM_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

      // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
      // 1 1 1 1 1 1 1 2 2 2 -> 1
      // 3 3 3 3 3 3 3 3 3 3 -> 3

      size_t offset = ((STEEM_100_PERCENT - STEEM_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / STEEM_100_PERCENT);

      std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
         []( const witness_object* a, const witness_object* b )
         {
            return a->last_confirmed_block_num < b->last_confirmed_block_num;
         } );

      uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

      if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
      {
         modify( dpo, [&]( dynamic_global_property_object& _dpo )
         {
            _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
         } );
      }
   }

   commit( dpo.last_irreversible_block_num );

   if( !( get_node_properties().skip_flags & skip_block_log ) )
   {
      // output to block log based on new last irreverisible block num
      const auto& tmp_head = _block_log.head();
      uint64_t log_head_num = 0;

      if( tmp_head )
         log_head_num = tmp_head->block_num();

      if( log_head_num < dpo.last_irreversible_block_num )
      {
         while( log_head_num < dpo.last_irreversible_block_num )
         {
            shared_ptr< fork_item > block = _fork_db.fetch_block_on_main_branch_by_number( log_head_num+1 );
            FC_ASSERT( block, "Current fork in the fork database does not contain the last_irreversible_block" );
            _block_log.append( block->data );
            log_head_num++;
         }

         _block_log.flush();
      }
   }

   _fork_db.set_max_size( dpo.head_block_number - dpo.last_irreversible_block_num + 1 );
} FC_CAPTURE_AND_RETHROW() }


void database::clear_expired_transactions()
{
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = get_index< transaction_index >();
   const auto& dedupe_index = transaction_idx.indices().get< by_expiration >();
   while( ( !dedupe_index.empty() ) && ( head_block_time() > dedupe_index.begin()->expiration ) )
      remove( *dedupe_index.begin() );
}

#ifdef STEEM_ENABLE_SMT
template< typename smt_balance_object_type >
void database::adjust_smt_balance( const account_name_type& name, const asset& delta, bool check_account )
{
   const smt_balance_object_type* bo =
      find< smt_balance_object_type, by_owner_symbol >( boost::make_tuple( name, delta.symbol ) );
   // Note that SMT related code, being post-20-hf needs no hf-guard to do balance checks.
   if( bo == nullptr )
   {
      // No balance object related to the SMT means '0' balance. Check delta to avoid creation of negative balance.
      FC_ASSERT( delta.amount.value >= 0, "Insufficient SMT ${smt} funds", ("smt", delta.symbol) );
      // No need to create object with '0' balance (see comment above).
      if( delta.amount.value == 0 )
         return;

      if( check_account )
         get_account( name );

      create< smt_balance_object_type >( [&]( smt_balance_object_type& smt_balance )
      {
         smt_balance.owner = name;
         smt_balance.balance = delta;
      } );
   }
   else
   {
      asset result = bo->balance + delta;
      // Check result to avoid negative balance storing.
      FC_ASSERT( result.amount.value >= 0, "Insufficient SMT ${smt} funds", ( "smt", delta.symbol ) );
      // Zero balance is the same as non object balance at all.
      if( result.amount.value == 0 )
      {
         remove( *bo );
      }
      else
      {
         modify( *bo, [&]( smt_balance_object_type& smt_balance )
         {
            smt_balance.balance = result;
         } );
      }
   }
}
#endif

//TODO_SOPHIA - rework
void database::modify_balance( const account_object& a, const asset& delta, bool check_balance )
{
   modify( a, [&]( account_object& acnt )
   {
      switch( delta.symbol.value )
      {
         case STEEM_SYMBOL_SER:
            acnt.balance += delta;
            if( check_balance )
            {
               FC_ASSERT( acnt.balance.amount.value >= 0, "Insufficient STEEM funds" );
            }
            adjust_proxied_witness_votes(a, delta.amount);
            break;
         default:
            FC_ASSERT( false, "invalid symbol" );
      }
   } );
}


void database::adjust_balance( const account_object& a, const asset& delta )
{

#ifdef STEEM_ENABLE_SMT
   // No account object modification for SMT balance, hence separate handling here.
   // Note that SMT related code, being post-20-hf needs no hf-guard to do balance checks.
   if( delta.symbol.space() == asset_symbol_type::smt_nai_space )
   {
      adjust_smt_balance< account_regular_balance_object >( a.name, delta, false/*check_account*/ );
      return;
   }
#endif
   modify_balance( a, delta, true );
}

void database::adjust_balance( const account_name_type& name, const asset& delta )
{

#ifdef STEEM_ENABLE_SMT
   // No account object modification for SMT balance, hence separate handling here.
   // Note that SMT related code, being post-20-hf needs no hf-guard to do balance checks.
   if( delta.symbol.space() == asset_symbol_type::smt_nai_space )
   {
      adjust_smt_balance< account_regular_balance_object >( name, delta, true/*check_account*/ );
      return;
   }
#endif
   const auto& a = get_account( name );
   modify_balance( a, delta, true );
}


void database::adjust_supply( const asset& delta )
{
#ifdef STEEM_ENABLE_SMT
   if( delta.symbol.space() == asset_symbol_type::smt_nai_space )
   {
      const auto& smt = get< smt_token_object, by_symbol >( delta.symbol );
      auto smt_new_supply = smt.current_supply + delta.amount;
      FC_ASSERT( smt_new_supply >= 0 );
      modify( smt, [smt_new_supply]( smt_token_object& smt )
      {
         smt.current_supply = smt_new_supply;
      });
      return;
   }
#endif

   const auto& props = get_dynamic_global_properties();

   modify( props, [&]( dynamic_global_property_object& props )
   {
      switch( delta.symbol.value )
      {
         case STEEM_SYMBOL_SER:
         {
            props.current_supply += delta;
            props.virtual_supply += delta;
            FC_ASSERT( props.current_supply.amount.value >= 0 );

            break;
         }
         default:
            FC_ASSERT( false, "invalid symbol" );
      }
   } );
}


asset database::get_balance( const account_object& a, asset_symbol_type symbol )const
{
   switch( symbol.value )
   {
      case STEEM_SYMBOL_SER:
         return a.balance;

      default:
      {
#ifdef STEEM_ENABLE_SMT
         FC_ASSERT( symbol.space() == asset_symbol_type::smt_nai_space, "invalid symbol" );
         const account_regular_balance_object* arbo =
            find< account_regular_balance_object, by_owner_symbol >( boost::make_tuple(a.name, symbol) );
         if( arbo == nullptr )
         {
            return asset(0, symbol);
         }
         else
         {
            return arbo->balance;
         }
#else
      FC_ASSERT( false, "invalid symbol" );
#endif
      }
   }
}

void database::init_hardforks()
{
   _hardfork_times[ 0 ] = fc::time_point_sec( STEEM_GENESIS_TIME );
   _hardfork_versions[ 0 ] = hardfork_version( 0, 0 );

   const auto& hardforks = get_hardfork_property_object();
   FC_ASSERT( hardforks.last_hardfork <= STEEM_NUM_HARDFORKS, "Chain knows of more hardforks than configuration", ("hardforks.last_hardfork",hardforks.last_hardfork)("STEEM_NUM_HARDFORKS",STEEM_NUM_HARDFORKS) );
   FC_ASSERT( _hardfork_versions[ hardforks.last_hardfork ] <= STEEM_BLOCKCHAIN_VERSION, "Blockchain version is older than last applied hardfork" );
   FC_ASSERT( STEEM_BLOCKCHAIN_HARDFORK_VERSION >= STEEM_BLOCKCHAIN_VERSION );
   FC_ASSERT( STEEM_BLOCKCHAIN_HARDFORK_VERSION == _hardfork_versions[ STEEM_NUM_HARDFORKS ] );
}

void database::process_hardforks()
{
   try
   {
      // If there are upcoming hardforks and the next one is later, do nothing
      const auto& hardforks = get_hardfork_property_object();

      {
         while( _hardfork_versions[ hardforks.last_hardfork ] < hardforks.next_hardfork
            && hardforks.next_hardfork_time <= head_block_time() )
         {
            if( hardforks.last_hardfork < STEEM_NUM_HARDFORKS ) {
               apply_hardfork( hardforks.last_hardfork + 1 );
            }
            else
               throw unknown_hardfork_exception();
         }
      }

   }
   FC_CAPTURE_AND_RETHROW()
}

bool database::has_hardfork( uint32_t hardfork )const
{
   return get_hardfork_property_object().processed_hardforks.size() > hardfork;
}

void database::set_hardfork( uint32_t hardfork, bool apply_now )
{
   auto const& hardforks = get_hardfork_property_object();

   for( uint32_t i = hardforks.last_hardfork + 1; i <= hardfork && i <= STEEM_NUM_HARDFORKS; i++ )
   {

      modify( hardforks, [&]( hardfork_property_object& hpo ) {
            hpo.next_hardfork = _hardfork_versions[i];
            hpo.next_hardfork_time = head_block_time();
      });


      if( apply_now )
         apply_hardfork( i );
   }
}

void database::apply_hardfork( uint32_t hardfork )
{
   if( _log_hardforks )
      elog( "HARDFORK ${hf} at block ${b}", ("hf", hardfork)("b", head_block_num()) );



   switch( hardfork )
   {

      default:
         break;
   }

   modify( get_hardfork_property_object(), [&]( hardfork_property_object& hfp )
   {
      FC_ASSERT( hardfork == hfp.last_hardfork + 1, "Hardfork being applied out of order", ("hardfork",hardfork)("hfp.last_hardfork",hfp.last_hardfork) );
      FC_ASSERT( hfp.processed_hardforks.size() == hardfork, "Hardfork being applied out of order" );
      hfp.processed_hardforks.push_back( _hardfork_times[ hardfork ] );
      hfp.last_hardfork = hardfork;
      hfp.current_hardfork_version = _hardfork_versions[ hardfork ];
      FC_ASSERT( hfp.processed_hardforks[ hfp.last_hardfork ] == _hardfork_times[ hfp.last_hardfork ], "Hardfork processing failed sanity check..." );
   } );

   push_virtual_operation( hardfork_operation( hardfork ), true );
}

/**
 * Verifies all supply invariantes check out
 */
void database::validate_invariants()const
{
   try
   {
      const auto& account_idx = get_index<account_index>().indices().get<by_name>();
      asset total_supply = asset( 0, STEEM_SYMBOL );
      asset total_vesting = asset( 0, VESTS_SYMBOL );
      share_type total_vsf_votes = share_type( 0 );

      auto gpo = get_dynamic_global_properties();

      /// verify no witness has too many votes
      const auto& witness_idx = get_index< witness_index >().indices();
      for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
         FC_ASSERT( itr->votes <= gpo.total_vesting_shares.amount, "", ("itr",*itr) );

      for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
      {
         total_supply += itr->balance;
         total_vesting += itr->vesting_shares;
         total_vsf_votes += ( itr->proxy == STEEM_PROXY_TO_SELF_ACCOUNT ?
                                 itr->witness_vote_weight() :
                                 ( STEEM_MAX_PROXY_RECURSION_DEPTH > 0 ?
                                      itr->proxied_vsf_votes[STEEM_MAX_PROXY_RECURSION_DEPTH - 1] :
                                      itr->balance.amount ) );
      }

      const auto& escrow_idx = get_index< escrow_index >().indices().get< by_id >();

      for( auto itr = escrow_idx.begin(); itr != escrow_idx.end(); ++itr )
      {
         total_supply += itr->steem_balance;

         if( itr->pending_fee.symbol == STEEM_SYMBOL )
            total_supply += itr->pending_fee;
         else
            FC_ASSERT( false, "found escrow pending fee that is not SBD or STEEM" );
      }


      FC_ASSERT( gpo.current_supply == total_supply, "", ("gpo.current_supply",gpo.current_supply)("total_supply",total_supply) );
      FC_ASSERT( gpo.total_vesting_shares == total_vesting, "", ("gpo.total_vesting_shares",gpo.total_vesting_shares)("total_vesting",total_vesting) );
      FC_ASSERT( gpo.total_vesting_shares.amount == total_vsf_votes, "", ("total_vesting_shares",gpo.total_vesting_shares)("total_vsf_votes",total_vsf_votes) );

      FC_ASSERT( gpo.virtual_supply >= gpo.current_supply );

   }
   FC_CAPTURE_LOG_AND_RETHROW( (head_block_num()) );
}

#ifdef STEEM_ENABLE_SMT

namespace {
   typedef std::map< asset_symbol_type, asset > TTotalSupplyMap;

   template <typename index_type>
   void add_from_balance_index(const index_type& balance_idx, TTotalSupplyMap& theMap)
   {
      auto it = balance_idx.begin();
      auto end = balance_idx.end();
      for( ; it != end; ++it )
      {
         const auto& balance = *it;
         auto insertInfo = theMap.emplace( balance.balance.symbol, balance.balance );
         if( insertInfo.second == false )
            insertInfo.first->second += balance.balance;
      }
   }
}

/**
 * SMT version of validate_invariants.
 */
void database::validate_smt_invariants()const
{
   try
   {
      // Get total balances.
      TTotalSupplyMap theMap;

      // - Balances
      const auto& balance_idx = get_index< account_regular_balance_index, by_id >();
      add_from_balance_index( balance_idx, theMap );

      // - Reward balances
      const auto& rewards_balance_idx = get_index< account_rewards_balance_index, by_id >();
      add_from_balance_index( rewards_balance_idx, theMap );

      // - Total vesting
#pragma message( "TODO: Add SMT vesting support here once it is implemented." )

      // - Market orders
      const auto& limit_order_idx = get_index< limit_order_index >().indices();
      for( auto itr = limit_order_idx.begin(); itr != limit_order_idx.end(); ++itr )
      {
         if( itr->sell_price.base.symbol.space() == asset_symbol_type::smt_nai_space )
         {
            asset a( itr->for_sale, itr->sell_price.base.symbol );
            auto insertInfo = theMap.emplace( a.symbol, a );
            if( insertInfo.second == false )
               insertInfo.first->second += a;
         }
      }

      // - Reward funds
#pragma message( "TODO: Add reward_fund_object iteration here once they support SMTs." )

      // - Escrow & savings - no support of SMT is expected.

      // Do the verification of total balances.
      auto itr = get_index< smt_token_index, by_id >().begin();
      auto end = get_index< smt_token_index, by_id >().end();
      for( ; itr != end; ++itr )
      {
         const smt_token_object& smt = *itr;
         auto totalIt = theMap.find( smt.liquid_symbol );
         asset total_liquid_supply = totalIt == theMap.end() ? asset(0, smt.liquid_symbol) : totalIt->second;
         FC_ASSERT( asset(smt.current_supply, smt.liquid_symbol) == total_liquid_supply,
                    "", ("smt current_supply",smt.current_supply)("total_liquid_supply",total_liquid_supply) );
      }
   }
   FC_CAPTURE_LOG_AND_RETHROW( (head_block_num()) );
}
#endif

void database::retally_witness_votes()
{
   const auto& witness_idx = get_index< witness_index >().indices();

   // Clear all witness votes
   for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
   {
      modify( *itr, [&]( witness_object& w )
      {
         w.votes = 0;
         w.virtual_position = 0;
      } );
   }

   const auto& account_idx = get_index< account_index >().indices();

   // Apply all existing votes by account
   for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
   {
      if( itr->proxy != STEEM_PROXY_TO_SELF_ACCOUNT ) continue;

      const auto& a = *itr;

      const auto& vidx = get_index<witness_vote_index>().indices().get<by_account_witness>();
      auto wit_itr = vidx.lower_bound( boost::make_tuple( a.name, account_name_type() ) );
      while( wit_itr != vidx.end() && wit_itr->account == a.name )
      {
         adjust_witness_vote( get< witness_object, by_name >(wit_itr->witness), a.witness_vote_weight() );
         ++wit_itr;
      }
   }
}

#ifdef STEEM_ENABLE_SMT
// 1. NAI number is stored in 32 bits, minus 4 for precision, minus 1 for control.
// 2. NAI string has 8 characters (each between '0' and '9') available (11 minus '@@', minus checksum character is 8 )
// 3. Max 27 bit decimal is 134,217,727 but only 8 characters are available to represent it as string so we are left
//    with [0 : 99,999,999] range.
// 4. The numbers starting with 0 decimal digit are reserved. Now we are left with 10 milions of reserved NAIs
//    [0 : 09,999,999] and 90 millions available for SMT creators [10,000,000 : 99,999,999]
// 5. The least significant bit is used as liquid/vesting variant indicator so the 10 and 90 milions are numbers
//    of liquid/vesting *pairs* of reserved/available NAIs.
// 6. 45 milions of SMT await for their creators.

vector< asset_symbol_type > database::get_smt_next_identifier()
{
   // This is temporary dummy implementation using simple counter as nai source (_next_available_nai).
   // Although a container of available nais is returned, it contains only one entry for simplicity.
   // Note that no decimal places argument is required from SMT creator at this stage.
   // This is because asset_symbol_type's to_string method omits the precision when serializing.
   // For appropriate use of this method see e.g. smt_database_fixture::create_smt

   uint8_t decimal_places = 0;

   FC_ASSERT( _next_available_nai >= SMT_MIN_NON_RESERVED_NAI );
   FC_ASSERT( _next_available_nai <= SMT_MAX_NAI, "Out of available NAI numbers." );
   // Assume that _next_available_nai value shows the liquid version of NAI.
   FC_ASSERT( (_next_available_nai & 0x1) == 0, "Can't start with vesting version of NAI." );
   uint32_t new_nai = _next_available_nai;
   // Skip vesting version of produced NAI - it differs only by least significant bit set.
   _next_available_nai += 2;

   uint32_t new_asset_num = (new_nai << 5) | 0x10 | decimal_places;
   asset_symbol_type new_symbol = asset_symbol_type::from_asset_num( new_asset_num );
   new_symbol.validate();
   FC_ASSERT( new_symbol.space() == asset_symbol_type::smt_nai_space );

   return vector< asset_symbol_type >( 1, new_symbol );
}
#endif

} } //steem::chain
