#include <sophiatx/protocol/sophiatx_operations.hpp>

#include <sophiatx/chain/block_summary_object.hpp>
#include <sophiatx/chain/compound.hpp>
#include <sophiatx/chain/custom_operation_interpreter.hpp>
#include <sophiatx/chain/database/database.hpp>
#include <sophiatx/chain/database/database_exceptions.hpp>
#include <sophiatx/chain/database/db_with.hpp>
#include <sophiatx/chain/global_property_object.hpp>
#include <sophiatx/chain/history_object.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/sophiatx_evaluator.hpp>
#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/custom_content_object.hpp>
#include <sophiatx/chain/transaction_object.hpp>
#include <sophiatx/chain/shared_db_merkle.hpp>
#include <sophiatx/chain/operation_notification.hpp>
#include <sophiatx/chain/witness_schedule.hpp>
#include <sophiatx/chain/application_object.hpp>

#include <sophiatx/chain/util/uint256.hpp>

#include <fc/uint128.hpp>

#include <fc/container/deque.hpp>

#include <fc/io/fstream.hpp>

#include <boost/scope_exit.hpp>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

namespace sophiatx { namespace chain {

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

FC_REFLECT( sophiatx::chain::object_schema_repr, (space_type)(type) )
FC_REFLECT( sophiatx::chain::operation_schema_repr, (id)(type) )
FC_REFLECT( sophiatx::chain::db_schema, (types)(object_types)(operation_type)(custom_operation_types) )

namespace sophiatx { namespace chain {

using boost::container::flat_set;

database::~database()
{
   clear_pending();
}

void database::open( const open_args& args, const genesis_state_type& genesis, const public_key_type& init_pubkey )
{
   try
   {

      ilog("initializing database...");
      chain_id_type chain_id = genesis.compute_chain_id();

      chainbase::database::open( args.shared_mem_dir, args.chainbase_flags, args.shared_file_size );

      initialize_indexes();
      initialize_evaluators();

      if( !find< dynamic_global_property_object >() )
         with_write_lock( [&]()
         {
            init_genesis( genesis, chain_id, init_pubkey );
         });

      _block_log.open( args.shared_mem_dir / "block_log" );

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
   FC_CAPTURE_LOG_AND_RETHROW( (args.shared_mem_dir)(args.shared_file_size) )
}

uint32_t database::reindex( const open_args& args, const genesis_state_type& genesis, const public_key_type& init_pubkey )
{
   bool reindex_success = false;
   uint32_t last_block_number = 0; // result

   BOOST_SCOPE_EXIT(this_,&reindex_success,&last_block_number) {
      SOPHIATX_TRY_NOTIFY(this_->_on_reindex_done, reindex_success, last_block_number);
   } BOOST_SCOPE_EXIT_END

   try
   {
      SOPHIATX_TRY_NOTIFY(_on_reindex_start);

      ilog( "Reindexing Blockchain" );
      wipe( args.shared_mem_dir, false );
      open( args, genesis, init_pubkey );
      _fork_db.reset();    // override effect of _fork_db.start_block() call in open()

      auto start = fc::time_point::now();
      SOPHIATX_ASSERT( _block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain." );

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
            if( cur_block_num % 10000 == 0 )
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
   FC_CAPTURE_AND_RETHROW( (args.shared_mem_dir) )

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
   fc::raw::unpack_from_buffer( itr->packed_trx, trx, 0 );
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

const account_object*  database::find_account( const account_id_type& id)const
{
   return find< account_object, by_id >( id );
}

const escrow_object& database::get_escrow( const account_name_type& name, uint32_t escrow_id )const
{ try {
   return get< escrow_object, by_from_id >( boost::make_tuple( name, escrow_id ) );
} FC_CAPTURE_AND_RETHROW( (name)(escrow_id) ) }

const escrow_object* database::find_escrow( const account_name_type& name, uint32_t escrow_id )const
{
   return find< escrow_object, by_from_id >( boost::make_tuple( name, escrow_id ) );
}

const application_object &database::get_application(const string &name) const
{
   try {
      return get< application_object, by_name >( name );
   } FC_CAPTURE_AND_RETHROW( (name) )
}

const application_object &database::get_application_by_id(  const application_id_type id )const
{
    try {
       return get< application_object, by_id >( id );
    } FC_CAPTURE_AND_RETHROW( (id) )
}

const application_buying_object &database::get_application_buying(const account_name_type &buyer, const application_id_type app_id) const
{
    try {
       return get< application_buying_object, by_buyer_app >( boost::make_tuple(buyer, app_id) );
    } FC_CAPTURE_AND_RETHROW( (buyer)(app_id) )
}

const dynamic_global_property_object&database::get_dynamic_global_properties() const
{ try {
      return get< dynamic_global_property_object >();
   } FC_CAPTURE_AND_RETHROW() }

const economic_model_object&database::get_economic_model() const
{ try {
   return get< economic_model_object >();
} FC_CAPTURE_AND_RETHROW() }

const feed_history_object & database::get_feed_history(asset_symbol_type a) const
{ try {
      const auto& fh_idx = get_index<feed_history_index>().indices().get<by_symbol>();
      const auto fh_itr = fh_idx.find(a);
      FC_ASSERT(fh_itr != fh_idx.end(), "Symbol history not found");
      return *fh_itr;
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

   FC_ASSERT(fee.symbol == SOPHIATX_SYMBOL);

   FC_ASSERT( account.balance >= fee );
   adjust_balance( account, -fee );
   adjust_supply( -fee );
   const auto& econ = get_economic_model();
   modify(econ, [&](economic_model_object&e){
      e.add_fee(fee.amount);
   });
}

asset database::process_operation_fee( const operation& op )
{
   class op_visitor{
      public:
      database* db;
      op_visitor(database* _db){db = _db;}; 
      typedef asset result_type;
      result_type operator()(const base_operation& bop){
         if(bop.has_special_fee() || db->is_private_net())
            return asset(0, SOPHIATX_SYMBOL);
         asset req_fee = bop.get_required_fee(bop.fee.symbol);
         FC_ASSERT(bop.fee.symbol == req_fee.symbol, "fee cannot be paid in with symbol ${s}", ("s", bop.fee.symbol));
         FC_ASSERT(bop.fee >= req_fee);
         auto sponsor = db->get_sponsor(bop.get_fee_payer());
         const auto& fee_payer = db->get_account(sponsor? *sponsor : bop.get_fee_payer());

         asset to_pay;
         if(bop.fee.symbol==SOPHIATX_SYMBOL){
            to_pay = bop.fee;
         }else{
            to_pay = db->to_sophiatx(bop.fee);
         }
         FC_ASSERT(to_pay.symbol == SOPHIATX_SYMBOL && to_pay.amount >= 0);
         db->pay_fee(fee_payer, to_pay);
         return to_pay;
      };
   };
   op_visitor op_v(this);
   asset paid_fee = op.visit(op_v);
   return paid_fee;
}

account_name_type database::get_fee_payer(const operation& op){
   class op_visitor{
   public:
      database* db;
      op_visitor(database* _db){db = _db;};
      typedef account_name_type result_type;
      result_type operator()(const base_operation& bop){
         auto sponsor = db->get_sponsor(bop.get_fee_payer());
         return sponsor? *sponsor : bop.get_fee_payer();
      }
   };

   op_visitor op_v(this);
   return op.visit(op_v);
}

optional<account_name_type> database::get_sponsor(const account_name_type& who) const {
   try {
      const account_fee_sponsor_object *s = find<account_fee_sponsor_object, by_sponsored>(who);
      if( s )
         return s->sponsor;
      return optional<account_name_type>();
   } FC_LOG_AND_RETHROW()
}

   uint32_t database::witness_participation_rate()const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(SOPHIATX_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
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

      wlog( "Encountered block num collision at block ${n} due to a fork, witnesses are: ${w}", ("n", height)("w", witness_time_pairs) );
   }
   return;
}

bool database::_push_block(const signed_block& new_block)
{ try {

   uint32_t skip = node_properties().skip_flags;
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
         FC_ASSERT( fc::raw::pack_size(trx) <= SOPHIATX_MAX_TRANSACTION_SIZE, "Transaction size is bigger than SOPHIATX_MAX_TRANSACTION_SIZE");
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
   uint32_t skip = node_properties().skip_flags;
   uint32_t slot_num = get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   string scheduled_witness = get_scheduled_witness( slot_num );
   FC_ASSERT( scheduled_witness == witness_owner );

   const auto& witness_obj = get_witness( witness_owner );

   if( !(skip & skip_witness_signature) )
      FC_ASSERT( witness_obj.signing_key == block_signing_private_key.get_public_key(),
                 "The witness signing key ${ws} is different to the block generation key ${bs}",
                 ("ws",witness_obj.signing_key)("bs", block_signing_private_key.get_public_key()) );

   static const size_t max_block_header_size = fc::raw::pack_size( signed_block_header() ) + 4;
   auto maximum_block_size = get_dynamic_global_properties().maximum_block_size; //SOPHIATX_MAX_BLOCK_SIZE;
   size_t total_block_size = max_block_header_size;

   signed_block pending_block;

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

      if( witness.running_version != SOPHIATX_BLOCKCHAIN_VERSION )
         pending_block.extensions.insert( block_header_extensions( SOPHIATX_BLOCKCHAIN_VERSION ) );

      const auto& hfp = get_hardfork_property_object();

      if( hfp.current_hardfork_version < SOPHIATX_BLOCKCHAIN_VERSION // Binary is newer hardfork than has been applied
         && ( witness.hardfork_version_vote != _hardfork_versions[ hfp.last_hardfork + 1 ] || witness.hardfork_time_vote != _hardfork_times[ hfp.last_hardfork + 1 ] ) ) // Witness vote does not match binary configuration
      {
         // Make vote match binary configuration
         pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork + 1 ], _hardfork_times[ hfp.last_hardfork + 1 ] ) ) );
      }
      else if( hfp.current_hardfork_version == SOPHIATX_BLOCKCHAIN_VERSION // Binary does not know of a new hardfork
         && witness.hardfork_version_vote > SOPHIATX_BLOCKCHAIN_VERSION ) // Voting for hardfork in the future, that we do not know of...
      {
         // Make vote match binary configuration. This is vote to not apply the new hardfork.
         pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork ], _hardfork_times[ hfp.last_hardfork ] ) ) );
      }
   }

   if( !(skip & skip_witness_signature) )
      pending_block.sign( block_signing_private_key, has_hardfork(SOPHIATX_HARDFORK_1_1) ? fc::ecc::bip_0062 : fc::ecc::fc_canonical);

   // TODO:  Move this to _push_block() so session is restored.
   if( !(skip & skip_block_size_check) )
   {
      FC_ASSERT( fc::raw::pack_size(pending_block) <= SOPHIATX_MAX_BLOCK_SIZE );
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
      SOPHIATX_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );

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

   auto interval = SOPHIATX_BLOCK_INTERVAL;
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
   return (when - first_slot_time).to_seconds() / SOPHIATX_BLOCK_INTERVAL + 1;
}

void  database::vest( const account_name_type& name, const share_type delta)
{try{
   const auto& a = get_account(name);
   vest(a, delta);
}FC_LOG_AND_RETHROW() }

void database::vest(const account_object& a, const share_type delta)
{
   FC_ASSERT(a.balance.amount >= delta);
   modify(a, [&](account_object& ao){
      ao.balance.amount -= delta;
      ao.vesting_shares.amount += delta;
   });
   const auto& cprops = get_dynamic_global_properties();

   modify( cprops, [&]( dynamic_global_property_object& o )
   {
        o.total_vesting_shares.amount += delta;
   });
}

void database::adjust_proxied_witness_votes( const account_object& a,
                                   const std::array< share_type, SOPHIATX_MAX_PROXY_RECURSION_DEPTH+1 >& delta,
                                   int depth )
{
   if( a.proxy != SOPHIATX_PROXY_TO_SELF_ACCOUNT )
   {
      /// nested proxies are not supported, vote will not propagate
      if( depth >= SOPHIATX_MAX_PROXY_RECURSION_DEPTH )
         return;

      const auto& proxy = get_account( a.proxy );

      modify( proxy, [&]( account_object& a )
      {
         for( int i = SOPHIATX_MAX_PROXY_RECURSION_DEPTH - depth - 1; i >= 0; --i )
         {
            a.proxied_vsf_votes[i+depth] += delta[i];
         }
      } );

      adjust_proxied_witness_votes( proxy, delta, depth + 1 );
   }
   else
   {
      share_type total_delta = 0;
      for( int i = SOPHIATX_MAX_PROXY_RECURSION_DEPTH - depth; i >= 0; --i )
         total_delta += delta[i];
      adjust_witness_votes( a, total_delta );
   }
}

void database::adjust_proxied_witness_votes( const account_object& a, share_type delta, int depth )
{
   if( a.proxy != SOPHIATX_PROXY_TO_SELF_ACCOUNT )
   {
      /// nested proxies are not supported, vote will not propagate
      if( depth >= SOPHIATX_MAX_PROXY_RECURSION_DEPTH )
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


      w.virtual_scheduled_time = w.virtual_last_update + (SOPHIATX_VIRTUAL_SCHEDULE_LAP_LENGTH2 - w.virtual_position)/(w.votes.value+1);
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

   const auto& null_account = get_account( SOPHIATX_NULL_ACCOUNT );
   asset total_sophiatx( 0, SOPHIATX_SYMBOL );

   if( null_account.balance.amount > 0 )
   {
      total_sophiatx += null_account.balance;
      adjust_balance( null_account, -null_account.balance );
   }

   if( null_account.vesting_shares.amount > 0 )
   {
      const auto& gpo = get_dynamic_global_properties();
      auto converted_sophiatx = null_account.vesting_shares;

      modify( gpo, [&]( dynamic_global_property_object& g )
      {
         g.total_vesting_shares -= null_account.vesting_shares;
      });

      modify( null_account, [&]( account_object& a )
      {
         a.vesting_shares.amount = 0;
      });

      total_sophiatx.amount += converted_sophiatx.amount;
   }

   if( total_sophiatx.amount > 0 )
      adjust_supply( -total_sophiatx );

}

void database::update_owner_authority( const account_object& account, const authority& owner_authority )
{

   create< owner_authority_history_object >( [&]( owner_authority_history_object& hist ) {
        hist.account = account.name;
        hist.previous_owner_authority = get< account_authority_object, by_account >( account.name ).owner;
        hist.last_valid_time = head_block_time();
   });


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


      modify( from_account, [&]( account_object& a )
      {
         a.vesting_shares.amount -= to_withdraw;
         a.balance.amount += to_withdraw;
         a.withdrawn += to_withdraw;

         if( a.withdrawn >= a.to_withdraw || a.vesting_shares.amount == 0 )
         {
            a.vesting_withdraw_rate.amount = 0;
            a.next_vesting_withdrawal = fc::time_point_sec::maximum();
         }
         else
         {
            a.next_vesting_withdrawal += fc::seconds( SOPHIATX_VESTING_WITHDRAW_INTERVAL_SECONDS );
         }
      });

      modify( cprops, [&]( dynamic_global_property_object& o )
      {
         o.total_vesting_shares.amount -= to_withdraw;
      });

      //if( to_withdraw > 0 )
      //   adjust_proxied_witness_votes( from_account, -to_withdraw );

      push_virtual_operation( fill_vesting_withdraw_operation( from_account.name, from_account.name, asset( to_withdraw, VESTS_SYMBOL ), asset( to_withdraw, SOPHIATX_SYMBOL ) ) );
   }
}

//TODO_SOPHIA - rework
/**
 *  Overall the network has an inflation rate of 102% of virtual sophiatx per year
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
   const auto& props = get_dynamic_global_properties();
   const auto& wso = get_witness_schedule_object();
   const auto& economics = get_economic_model();

   share_type witness_reward;
   uint32_t denominator = wso.witness_pay_normalization_factor;
   uint32_t nominator = 0;

   const auto& cwit = get_witness( props.current_witness );
   if( cwit.schedule == witness_object::timeshare )
      nominator = wso.timeshare_weight;
   else if( cwit.schedule == witness_object::top19 )
      nominator = wso.top19_weight;
   // b - avg block reward
   //(no_voted * x + no_timeshared *y) = (no_voted+no_timeshared) * b
   //y=3x
   //25 * x = 21 * b
   // x = 21 / 25 * b
   // y = 63 / 25 * b

   modify(economics, [&](economic_model_object& e){
      witness_reward = e.withdraw_mining_reward(_current_block_num, nominator, denominator);
   });

   push_virtual_operation( producer_reward_operation( cwit.owner, asset(witness_reward, VESTS_SYMBOL) ) );
   create_vesting(cwit.owner, asset(witness_reward, VESTS_SYMBOL));

   modify( props, [&]( dynamic_global_property_object& p )
   {
        p.current_supply           += asset( witness_reward, SOPHIATX_SYMBOL );
        p.total_vesting_shares     += asset( witness_reward, VESTS_SYMBOL );
   });

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

   while( hist != hist_idx.end() && time_point_sec( hist->last_valid_time + SOPHIATX_OWNER_AUTH_RECOVERY_PERIOD ) < head_block_time() )
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

      adjust_balance( old_escrow.from, old_escrow.sophiatx_balance );
      adjust_balance( old_escrow.from, old_escrow.pending_fee );

      remove( old_escrow );
   }
}

void database::initialize_evaluators()
{
   _evaluator_registry.register_db(std::static_pointer_cast<database>(shared_from_this()));

   _evaluator_registry.register_evaluator< transfer_evaluator                       >();
   _evaluator_registry.register_evaluator< transfer_to_vesting_evaluator            >();
   _evaluator_registry.register_evaluator< withdraw_vesting_evaluator               >();
   _evaluator_registry.register_evaluator< account_create_evaluator                 >();
   _evaluator_registry.register_evaluator< account_update_evaluator                 >();
   _evaluator_registry.register_evaluator< account_delete_evaluator                 >();
   _evaluator_registry.register_evaluator< witness_update_evaluator                 >();
   _evaluator_registry.register_evaluator< witness_stop_evaluator                   >();
   _evaluator_registry.register_evaluator< account_witness_vote_evaluator           >();
   _evaluator_registry.register_evaluator< account_witness_proxy_evaluator          >();
   _evaluator_registry.register_evaluator< custom_evaluator                         >();
   _evaluator_registry.register_evaluator< custom_binary_evaluator                  >();
   _evaluator_registry.register_evaluator< custom_json_evaluator                    >();
   _evaluator_registry.register_evaluator< feed_publish_evaluator                   >();
   _evaluator_registry.register_evaluator< request_account_recovery_evaluator       >();
   _evaluator_registry.register_evaluator< recover_account_evaluator                >();
   _evaluator_registry.register_evaluator< change_recovery_account_evaluator        >();
   _evaluator_registry.register_evaluator< escrow_transfer_evaluator                >();
   _evaluator_registry.register_evaluator< escrow_approve_evaluator                 >();
   _evaluator_registry.register_evaluator< escrow_dispute_evaluator                 >();
   _evaluator_registry.register_evaluator< escrow_release_evaluator                 >();
   _evaluator_registry.register_evaluator< reset_account_evaluator                  >();
   _evaluator_registry.register_evaluator< set_reset_account_evaluator              >();
   _evaluator_registry.register_evaluator< application_create_evaluator             >();
   _evaluator_registry.register_evaluator< application_update_evaluator             >();
   _evaluator_registry.register_evaluator< application_delete_evaluator             >();
   _evaluator_registry.register_evaluator< buy_application_evaluator                >();
   _evaluator_registry.register_evaluator< cancel_application_buying_evaluator      >();
   _evaluator_registry.register_evaluator< witness_set_properties_evaluator         >();
   _evaluator_registry.register_evaluator< transfer_from_promotion_pool_evaluator   >();
   _evaluator_registry.register_evaluator< sponsor_fees_evaluator                   >();
   _evaluator_registry.register_evaluator< admin_witness_update_evaluator           >();
}

void database::initialize_indexes()
{
   add_core_index< dynamic_global_property_index           >(shared_from_this());
   add_core_index< economic_model_index                    >(shared_from_this());
   add_core_index< account_index                           >(shared_from_this());
   add_core_index< account_authority_index                 >(shared_from_this());
   add_core_index< witness_index                           >(shared_from_this());
   add_core_index< transaction_index                       >(shared_from_this());
   add_core_index< block_summary_index                     >(shared_from_this());
   add_core_index< witness_schedule_index                  >(shared_from_this());
   add_core_index< witness_vote_index                      >(shared_from_this());
   add_core_index< feed_history_index                      >(shared_from_this());
   add_core_index< operation_index                         >(shared_from_this());
   add_core_index< account_history_index                   >(shared_from_this());
   add_core_index< hardfork_property_index                 >(shared_from_this());
   add_core_index< owner_authority_history_index           >(shared_from_this());
   add_core_index< account_recovery_request_index          >(shared_from_this());
   add_core_index< change_recovery_account_request_index   >(shared_from_this());
   add_core_index< escrow_index                            >(shared_from_this());
   add_core_index< application_index                       >(shared_from_this());
   add_core_index< application_buying_index                >(shared_from_this());
   add_core_index< custom_content_index                    >(shared_from_this());
   add_core_index< account_fee_sponsor_index               >(shared_from_this());
   _plugin_index_signal();
}

void database::init_genesis( genesis_state_type genesis, chain_id_type chain_id, const public_key_type& init_pubkey )
{
   try
   {
      struct auth_inhibitor
      {
         auth_inhibitor(const std::shared_ptr<database>& db) : db(db), old_flags(db->node_properties().skip_flags)
         { db->node_properties().skip_flags |= skip_authority_check; }
         ~auth_inhibitor()
         { db->node_properties().skip_flags = old_flags; }
      private:
         std::shared_ptr<database> db;
         uint32_t old_flags;
      } inhibitor(std::static_pointer_cast<database>(shared_from_this()));

      share_type total_initial_balance = 0;
      // Create blockchain accounts
      public_key_type      init_public_key = genesis.initial_public_key;

      create< account_object >( [&]( account_object& a )
      {
         a.name = SOPHIATX_MINER_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = SOPHIATX_MINER_ACCOUNT;
         auth.owner.weight_threshold = 1;
         auth.active.weight_threshold = 1;
      });

      create< account_object >( [&]( account_object& a )
      {
         a.name = SOPHIATX_NULL_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = SOPHIATX_NULL_ACCOUNT;
         auth.owner.weight_threshold = 1;
         auth.active.weight_threshold = 1;
      });

      create< account_object >( [&]( account_object& a )
      {
         a.name = SOPHIATX_TEMP_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = SOPHIATX_TEMP_ACCOUNT;
         auth.owner.weight_threshold = 0;
         auth.active.weight_threshold = 0;
      });

      total_initial_balance += genesis.initial_balace;
      create< account_object >( [&]( account_object& a )
                                {
                                     a.name = SOPHIATX_INIT_MINER_NAME;
                                     a.memo_key = init_public_key;
                                     a.balance  = asset( genesis.initial_balace, SOPHIATX_SYMBOL );
                                     a.holdings_considered_for_interests = a.balance.amount * 2;
                                } );

      create< account_authority_object >( [&]( account_authority_object& auth )
                                          {
                                               auth.account = SOPHIATX_INIT_MINER_NAME;
                                               auth.owner.add_authority( init_public_key, 1 );
                                               auth.owner.weight_threshold = 1;
                                               auth.active  = auth.owner;
                                          });

      create< witness_object >( [&]( witness_object& w )
                                {
                                     w.owner        = SOPHIATX_INIT_MINER_NAME;
                                     // TODO: use initminer mining public key from get_config when solution is implemnted
                                     w.signing_key  = init_pubkey;
                                     w.schedule = witness_object::top19;
                                     if(genesis.is_private_net)
                                        w.props.account_creation_fee = asset (0, SOPHIATX_SYMBOL);
                                } );

      for( const auto &acc: genesis.initial_accounts) {
         total_initial_balance += acc.balance;
         authority owner;
         authority active;
         active.add_authority(acc.key, 1);
         active.weight_threshold = 1;
         if(acc.key == public_key_type()){
            owner.add_authority(SOPHIATX_INIT_MINER_NAME, 1);
            owner.weight_threshold = 1;
         }else{
            owner = active;
         }
         try {
            create<account_object>([ & ](account_object &a) {
                 a.name = acc.name;
                 a.memo_key = acc.key;
                 a.balance = asset(acc.balance, SOPHIATX_SYMBOL);
                 a.holdings_considered_for_interests = a.balance.amount * 2;
                 a.recovery_account = SOPHIATX_INIT_MINER_NAME;
                 a.reset_account = SOPHIATX_INIT_MINER_NAME;
            });

            create<account_authority_object>([ & ](account_authority_object &auth) {
                 auth.account = acc.name;
                 auth.owner = owner;
                 auth.active = active;
            });
         }FC_CAPTURE_AND_RETHROW((acc))
      }


      create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p )
      {
         p.current_witness = SOPHIATX_INIT_MINER_NAME;
         p.time = genesis.genesis_time;
         p.recent_slots_filled = fc::uint128::max_value();
         p.participation_count = 128;
         p.current_supply = asset( total_initial_balance, SOPHIATX_SYMBOL );
         p.maximum_block_size = SOPHIATX_MAX_BLOCK_SIZE;
         p.witness_required_vesting = asset(genesis.is_private_net? 0 : SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE, VESTS_SYMBOL);
         p.genesis_time = genesis.genesis_time;
         p.chain_id = chain_id;
         p.private_net = genesis.is_private_net;
      } );

      create< economic_model_object >( [&]( economic_model_object& e )
                                                {
                                                    e.init_economics(total_initial_balance, SOPHIATX_TOTAL_SUPPLY);
                                                } );
      // Nothing to do
      create< feed_history_object >( [&]( feed_history_object& o ) {o.symbol = SBD1_SYMBOL;});
      create< feed_history_object >( [&]( feed_history_object& o ) {o.symbol = SBD2_SYMBOL;});
      create< feed_history_object >( [&]( feed_history_object& o ) {o.symbol = SBD3_SYMBOL;});
      create< feed_history_object >( [&]( feed_history_object& o ) {o.symbol = SBD4_SYMBOL;});
      create< feed_history_object >( [&]( feed_history_object& o ) {o.symbol = SBD5_SYMBOL;});

      for( int i = 0; i < 0x10000; i++ )
         create< block_summary_object >( [&]( block_summary_object& ) {});

      create< hardfork_property_object >( [&](hardfork_property_object& hpo )
      {
         hpo.processed_hardforks.push_back( genesis.genesis_time );
      } );

      // Create witness scheduler
      create< witness_schedule_object >( [&]( witness_schedule_object& wso )
      {
         wso.current_shuffled_witnesses[0] = SOPHIATX_INIT_MINER_NAME;
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
      SOPHIATX_TRY_NOTIFY( changed_objects, ids )*/
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
         SOPHIATX_TRY_NOTIFY( changed_objects, changed_ids )
      }
      */
   }
   FC_CAPTURE_AND_RETHROW()

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

   try
   {
         /// check invariants
         if( is_producing() || !( skip & skip_validate_invariants ) )
            validate_invariants();
   }
   FC_CAPTURE_AND_RETHROW( (next_block) );

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

void database::_apply_block( const signed_block& next_block )
{ try {
   uint32_t next_block_num = next_block.block_num();
   //block_id_type next_block_id = next_block.id();

   uint32_t skip = node_properties().skip_flags;

   if( BOOST_UNLIKELY( next_block_num == 1 ) )
   {
      // For every existing before the head_block_time (genesis time), apply the hardfork
      // This allows the test net to launch with past hardforks and apply the next harfork when running

      uint32_t n;
      for( n=0; n<SOPHIATX_NUM_HARDFORKS; n++ )
      {
         if( _hardfork_times[n+1] > next_block.timestamp )
            break;
      }

      if( n > 0 )
      {
         wlog( "Processing ${n} genesis hardforks", ("n", n) );
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


   if( block_size < SOPHIATX_MIN_BLOCK_SIZE )
   {
      elog( "Block size is too small",
         ("next_block_num",next_block_num)("block_size", block_size)("min",SOPHIATX_MIN_BLOCK_SIZE)
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

   _current_virtual_op = 0;
   _current_trx_in_block = -1;

   update_global_dynamic_data(next_block);
   update_signing_witness(signing_witness, next_block);

   update_last_irreversible_block();

   create_block_summary(next_block);
   clear_expired_transactions();
   update_witness_schedule(std::static_pointer_cast<database>(shared_from_this()));
   if(!is_private_net()) {
      process_interests();
      update_median_feeds();
      clear_null_account_balance();
      process_funds();
      process_vesting_withdrawals();
   }
   account_recovery_processing();
   expire_escrow_ratification();

   process_hardforks();

   // notify observers that the block has been applied
   notify_applied_block( next_block );

   notify_changed_objects();

   if(!is_private_net()) {
      const auto& econ = get_economic_model();
      const auto& gpo = get_dynamic_global_properties();
      modify(econ, [ & ](economic_model_object &e) {
           e.record_block(next_block_num, gpo.current_supply.amount);
      });
   }

}
FC_CAPTURE_LOG_AND_RETHROW( (next_block.block_num()) )
}

struct process_header_visitor
{
   process_header_visitor( const std::string& witness, const std::shared_ptr<database>& db ) : _witness( witness ), _db( db ) {}

   typedef void result_type;

   const std::string& _witness;
   std::shared_ptr<database> _db;

   void operator()( const void_t& obj ) const
   {
      //Nothing to do.
   }

   void operator()( const version& reported_version ) const
   {
      const auto& signing_witness = _db->get_witness( _witness );
      //idump( (next_block.witness)(signing_witness.running_version)(reported_version) );

      if( reported_version != signing_witness.running_version )
      {
         _db->modify( signing_witness, [&]( witness_object& wo )
         {
            wo.running_version = reported_version;
         });
      }
   }

   void operator()( const hardfork_version_vote& hfv ) const
   {
      const auto& signing_witness = _db->get_witness( _witness );
      //idump( (next_block.witness)(signing_witness.running_version)(hfv) );

      if( hfv.hf_version != signing_witness.hardfork_version_vote || hfv.hf_time != signing_witness.hardfork_time_vote )
         _db->modify( signing_witness, [&]( witness_object& wo )
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

void database::check_free_memory(bool force_print, uint32_t current_block_num) {
   uint64_t free_mem = get_free_memory();
   uint64_t max_mem = get_max_memory();

   if( BOOST_UNLIKELY(_shared_file_full_threshold != 0 && _shared_file_scale_rate != 0 && free_mem < ((uint128_t(
         SOPHIATX_100_PERCENT - _shared_file_full_threshold) * max_mem) / SOPHIATX_100_PERCENT).to_uint64())) {
      uint64_t new_max = (uint128_t(max_mem * _shared_file_scale_rate) / SOPHIATX_100_PERCENT).to_uint64() + max_mem;

      wlog("Memory is almost full, increasing to ${mem}M", ("mem", new_max / (1024 * 1024)));

      resize(new_max);

      uint32_t free_mb = uint32_t(get_free_memory() / (1024 * 1024));
      wlog("Free memory is now ${free}M", ("free", free_mb));
      _last_free_gb_printed = free_mb / 1024;
   } else {
      uint32_t free_gb = uint32_t(free_mem / (1024 * 1024 * 1024));
      if( BOOST_UNLIKELY(force_print || (free_gb < _last_free_gb_printed) || (free_gb > _last_free_gb_printed + 1))) {
         ilog("Free memory is now ${n}G. Current block number: ${block}", ("n", free_gb)("block", current_block_num));
         _last_free_gb_printed = free_gb;
      }

      if( BOOST_UNLIKELY(free_gb == 0)) {
         uint32_t free_mb = uint32_t(free_mem / (1024 * 1024));

#ifdef IS_TEST_NET
         if( !disable_low_mem_warning )
#endif
            if( free_mb <= 100 && head_block_num() % 10 == 0 )
               elog("Free memory is now ${n}M. Increase shared file size immediately!", ("n", free_mb));
      }
   }
}

void database::process_interests() {
   try {
      uint32_t block_no = head_block_num(); //process_interests is called after the current block is accepted
      uint32_t interest_blocks = SOPHIATX_INTEREST_BLOCKS;
      uint32_t batch = block_no % interest_blocks;
      const auto &econ = get_economic_model();
      share_type supply_increase = 0;
      uint64_t id = batch;
      while( const account_object *a = find_account(id)) {
         share_type interest = 0;

         if( head_block_num() > SOPHIATX_INTEREST_DELAY) {
            modify(econ, [ & ](economic_model_object &eo) {
               interest = eo.withdraw_interests(a->holdings_considered_for_interests,
                     std::min(uint32_t(interest_blocks), head_block_num()));
            });
         }

         if( interest > 0 ) {
            supply_increase += interest;
            push_virtual_operation(interest_operation(a->name, asset(interest, SOPHIATX_SYMBOL)));
            if( has_hardfork(SOPHIATX_HARDFORK_1_1))
               adjust_proxied_witness_votes(*a, interest);
         }

         modify(*a, [ & ](account_object &ao) {
              ao.balance.amount += interest;
              ao.holdings_considered_for_interests = ao.total_balance() * interest_blocks;
         });
         id += interest_blocks;
      }

      adjust_supply(asset(supply_increase, SOPHIATX_SYMBOL));
   }FC_CAPTURE_AND_RETHROW()
}

void database::process_header_extensions( const signed_block& next_block )
{
   process_header_visitor _v( next_block.witness, std::static_pointer_cast<database>(shared_from_this()) );

   for( const auto& e : next_block.extensions )
      e.visit( _v );
}

void database::update_median_feeds() {
try {
   if( (head_block_num() % SOPHIATX_FEED_INTERVAL_BLOCKS) != 0 )
      return;

   auto now = head_block_time();
   const witness_schedule_object& wso = get_witness_schedule_object();
   map<asset_symbol_type, vector<price>> all_feeds;
   for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
   {
      const auto& wit = get_witness( wso.current_shuffled_witnesses[i] );
      {
         for ( const auto& r: wit.submitted_exchange_rates )
            if( r.second.last_change + SOPHIATX_MAX_FEED_AGE_SECONDS > now  && !r.second.rate.is_null() )
               all_feeds[r.first].push_back(r.second.rate);
      }
   }

   for ( const auto& feed: all_feeds){
      if( feed.second.size() >= SOPHIATX_MIN_FEEDS )
      {
         vector<price> f = feed.second;
         std::sort( f.begin(), f.end() );
         auto median_feed = f[f.size()/2];

         modify(get_feed_history(feed.first), [&](feed_history_object& fho )
         {
            fho.price_history.push_back( median_feed );
            size_t sophiatx_feed_history_window = SOPHIATX_FEED_HISTORY_WINDOW;

            if( fho.price_history.size() > sophiatx_feed_history_window )
               fho.price_history.pop_front();

            fho.current_median_history = median_feed;
         });
      }
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
   _current_virtual_op = 0;
   uint32_t skip = node_properties().skip_flags;

   if( !(skip&skip_validate) ) {   /* issue #505 explains why this skip_flag is disabled */
      trx.validate();
   }

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
         trx.verify_authority( chain_id, get_active, get_owner, SOPHIATX_MAX_SIG_CHECK_DEPTH,
                               has_hardfork(SOPHIATX_HARDFORK_1_1) ? fc::ecc::bip_0062 : fc::ecc::fc_canonical);
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
         SOPHIATX_ASSERT( trx.ref_block_prefix == tapos_block_summary.block_id._hash[1], transaction_tapos_exception,
                    "", ("trx.ref_block_prefix", trx.ref_block_prefix)
                    ("tapos_block_summary",tapos_block_summary.block_id._hash[1]));
      }

      fc::time_point_sec now = head_block_time();

      SOPHIATX_ASSERT( trx.expiration <= now + fc::seconds(SOPHIATX_MAX_TIME_UNTIL_EXPIRATION), transaction_expiration_exception,
                  "", ("trx.expiration",trx.expiration)("now",now)("max_til_exp",SOPHIATX_MAX_TIME_UNTIL_EXPIRATION));
      SOPHIATX_ASSERT( now < trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
      SOPHIATX_ASSERT( now <= trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
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

   note.fee_payer = get_fee_payer(op);

   notify_pre_apply_operation( note );
   process_operation_fee(op);
   _evaluator_registry.get_evaluator( op ).apply( op );
   notify_post_apply_operation( note );
}

const witness_object& database::validate_block_header( uint32_t skip, const signed_block& next_block )const
{ try {
   FC_ASSERT( head_block_id() == next_block.previous, "", ("head_block_id",head_block_id())("next.prev",next_block.previous) );
   FC_ASSERT( head_block_time() < next_block.timestamp, "", ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()) );
   const witness_object& witness = get_witness( next_block.witness );

   if( !(skip&skip_witness_signature) )
      FC_ASSERT( next_block.validate_signee( witness.signing_key,
            has_hardfork(SOPHIATX_HARDFORK_1_1) ? fc::ecc::bip_0062 : fc::ecc::fc_canonical ) );

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
               wlog("Witness ${w} missed block at time ${t}", ("w", witness_missed.owner)("t", get_slot_time(i + 1)));

               if( head_block_num() - w.last_confirmed_block_num  > SOPHIATX_BLOCKS_PER_DAY )
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
      if(!is_private_net()){
         uint64_t switch_block;
         if(has_hardfork(SOPHIATX_HARDFORK_1_1))
            switch_block = SOPHIATX_WITNESS_VESTING_INCREASE_DAYS_HF_1_1 * SOPHIATX_BLOCKS_PER_DAY;
         else
            switch_block = SOPHIATX_WITNESS_VESTING_INCREASE_DAYS * SOPHIATX_BLOCKS_PER_DAY;

         if( head_block_num() >= switch_block ){
            dgp.witness_required_vesting = asset(SOPHIATX_FINAL_WITNESS_REQUIRED_VESTING_BALANCE, VESTS_SYMBOL);
         }else
            dgp.witness_required_vesting = asset(SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE, VESTS_SYMBOL);
      }
   } );

   if( !(node_properties().skip_flags & skip_undo_history_check) )
   {
      SOPHIATX_ASSERT( _dgp.head_block_number - _dgp.last_irreversible_block_num  < SOPHIATX_MAX_UNDO_HISTORY, undo_database_exception,
                 "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                 "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                 ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)
                 ("max_undo",SOPHIATX_MAX_UNDO_HISTORY) );
   }
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
   if( head_block_num() < SOPHIATX_START_MINER_VOTING_BLOCK )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         if ( head_block_num() > SOPHIATX_MAX_WITNESSES )
            _dpo.last_irreversible_block_num = head_block_num() - SOPHIATX_MAX_WITNESSES;
      } );
   }
   else
   {
      const witness_schedule_object& wso = get_witness_schedule_object();

      vector< const witness_object* > wit_objs;
      wit_objs.reserve( wso.num_scheduled_witnesses );
      for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
         wit_objs.push_back( &get_witness( wso.current_shuffled_witnesses[i] ) );

      static_assert( SOPHIATX_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

      // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
      // 1 1 1 1 1 1 1 2 2 2 -> 1
      // 3 3 3 3 3 3 3 3 3 3 -> 3

      size_t offset = ((SOPHIATX_100_PERCENT - SOPHIATX_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / SOPHIATX_100_PERCENT);

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

   if( !( node_properties().skip_flags & skip_block_log ) )
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

void database::create_vesting( const account_object& a, const asset& delta){
   FC_ASSERT(delta.symbol == VESTS_SYMBOL, "invalid symbol");
   FC_ASSERT(delta.amount >= 0, "cannot remove vests");

   modify( a, [&]( account_object& acnt )
   {
        acnt.vesting_shares.amount += delta.amount;
        acnt.update_considered_holding(delta.amount, head_block_num() );
   } );
   adjust_proxied_witness_votes(a, delta.amount);
}


void database::modify_balance( const account_object& a, const asset& delta, bool check_balance )
{
   FC_ASSERT(delta.symbol == SOPHIATX_SYMBOL, "invalid symbol");

   modify( a, [&]( account_object& acnt )
   {
        acnt.balance.amount += delta.amount;
        acnt.update_considered_holding(delta.amount, head_block_num() );
        if( check_balance )
        {
           FC_ASSERT( acnt.balance.amount.value >= 0, "Insufficient SOPHIATX funds" );
        }

   } );
   adjust_proxied_witness_votes(a, delta.amount);
}


void database::adjust_balance( const account_object& a, const asset& delta )
{
   modify_balance( a, delta, true );
}

void database::adjust_balance( const account_name_type& name, const asset& delta )
{
   const auto& a = get_account( name );
   modify_balance( a, delta, true );
}

void database::create_vesting( const account_name_type& name, const asset& delta){
   const auto& a = get_account( name );
   create_vesting( a, delta);
}

void database::adjust_supply( const asset& delta )
{
   const auto& props = get_dynamic_global_properties();

   modify( props, [&]( dynamic_global_property_object& props )
   {
      switch( delta.symbol.value )
      {
         case SOPHIATX_SYMBOL_SER:
         {
            props.current_supply += delta;
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
      case SOPHIATX_SYMBOL_SER:
         return a.balance;

      default:
      {
         FC_ASSERT( false, "invalid symbol" );
      }
   }
}

void database::init_hardforks()
{
   _hardfork_times[ 0 ] = fc::time_point_sec( get_genesis_time() );
   _hardfork_versions[ 0 ] = hardfork_version( 1, 0 );

   FC_ASSERT( SOPHIATX_HARDFORK_1_1 == 1, "Invalid hardfork configuration" );
   _hardfork_times[ SOPHIATX_HARDFORK_1_1 ] = fc::time_point_sec( SOPHIATX_HARDFORK_1_1_TIME );
   _hardfork_versions[ SOPHIATX_HARDFORK_1_1 ] = SOPHIATX_HARDFORK_1_1_VERSION;

   const auto& hardforks = get_hardfork_property_object();
   FC_ASSERT( hardforks.last_hardfork <= SOPHIATX_NUM_HARDFORKS, "Chain knows of more hardforks than configuration", ("hardforks.last_hardfork",hardforks.last_hardfork)("SOPHIATX_NUM_HARDFORKS",SOPHIATX_NUM_HARDFORKS) );
   FC_ASSERT( _hardfork_versions[ hardforks.last_hardfork ] <= SOPHIATX_BLOCKCHAIN_VERSION, "Blockchain version is older than last applied hardfork" );
   FC_ASSERT( SOPHIATX_BLOCKCHAIN_HARDFORK_VERSION >= SOPHIATX_BLOCKCHAIN_VERSION );
   FC_ASSERT( SOPHIATX_BLOCKCHAIN_HARDFORK_VERSION == _hardfork_versions[ SOPHIATX_NUM_HARDFORKS ] );
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
            if( hardforks.last_hardfork < SOPHIATX_NUM_HARDFORKS ) {
               apply_hardfork( hardforks.last_hardfork + 1 );
            }
            else
               throw unknown_hardfork_exception();
         }
      }

   }
   FC_CAPTURE_AND_RETHROW()
}

void database::set_hardfork( uint32_t hardfork, bool apply_now )
{
   auto const& hardforks = get_hardfork_property_object();

   for( uint32_t i = hardforks.last_hardfork + 1; i <= hardfork && i <= SOPHIATX_NUM_HARDFORKS; i++ )
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
      case SOPHIATX_HARDFORK_1_1:
         recalculate_all_votes();
         break;
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

void database::recalculate_all_votes(){
   const auto& account_idx = get_index< account_index >().indices().get<by_name>();
   const auto& witness_idx = get_index< witness_index >().indices();
   for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr ){
      //clear all witness votes
      ilog("${w} - ${h}",("w", itr->owner)("h", itr->votes));
      modify(*itr, [&](witness_object &wo){
         wo.votes = 0;
      });
   }
   for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr ){
      adjust_proxied_witness_votes(*itr, itr->total_balance());
   }
   for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr ){
      ilog("${w} - ${h}",("w", itr->owner)("h", itr->votes));
   }
}


/**
 * Verifies all supply invariantes check out
 */
void database::validate_invariants()const
{
   try
   {
      if(is_private_net())
         return;
      const auto& account_idx = get_index<account_index>().indices().get<by_id>();
      asset total_supply = asset( 0, SOPHIATX_SYMBOL );
      asset total_vesting = asset( 0, VESTS_SYMBOL );

      const auto& gpo = get_dynamic_global_properties();
      const auto& econ = get_economic_model();

      /// verify no witness has too many votes
      const auto& witness_idx = get_index< witness_index >().indices();
      for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
         FC_ASSERT( itr->votes <= gpo.current_supply.amount, "", ("itr",*itr) );

      for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
      {
         total_supply += itr->balance;
         total_vesting += itr->vesting_shares;
      }

      const auto& escrow_idx = get_index< escrow_index >().indices().get< by_id >();

      for( auto itr = escrow_idx.begin(); itr != escrow_idx.end(); ++itr )
      {
         total_supply += itr->sophiatx_balance;

         if( itr->pending_fee.symbol == SOPHIATX_SYMBOL )
            total_supply += itr->pending_fee;
         else
            FC_ASSERT( false, "found escrow pending fee that is not SPHTX" );
      }


      FC_ASSERT( gpo.current_supply == total_supply + asset(total_vesting.amount, SOPHIATX_SYMBOL), "", ("gpo.current_supply",gpo.current_supply)("total_supply",total_supply) );
      FC_ASSERT( gpo.total_vesting_shares == total_vesting, "", ("gpo.total_vesting_shares",gpo.total_vesting_shares)("total_vesting",total_vesting) );

      FC_ASSERT( (gpo.current_supply.amount + econ.interest_pool_from_fees + econ.interest_pool_from_coinbase +
                 econ.mining_pool_from_fees + econ.mining_pool_from_coinbase + econ.promotion_pool + econ.burn_pool) == SOPHIATX_TOTAL_SUPPLY, "difference is $diff", ("diff", SOPHIATX_TOTAL_SUPPLY -
                 (gpo.current_supply.amount + econ.interest_pool_from_fees + econ.interest_pool_from_coinbase +
                 econ.mining_pool_from_fees + econ.mining_pool_from_coinbase + econ.promotion_pool + econ.burn_pool)));

   }
   FC_CAPTURE_LOG_AND_RETHROW( (head_block_num()) );
}


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
      if( itr->proxy != SOPHIATX_PROXY_TO_SELF_ACCOUNT ) continue;

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

} } //sophiatx::chain
