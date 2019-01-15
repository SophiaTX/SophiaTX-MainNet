#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/database/database_exceptions.hpp>

namespace sophiatx {
namespace chain {

void database_interface::set_custom_operation_interpreter(const uint32_t id,
                                                          std::shared_ptr<custom_operation_interpreter> registry) {
   bool inserted = _custom_operation_interpreters.emplace(id, registry).second;
   // This assert triggering means we're mis-configured (multiple registrations of custom JSON evaluator for same ID)
   FC_ASSERT(inserted);
}

std::shared_ptr<custom_operation_interpreter> database_interface::get_custom_json_evaluator(const uint64_t id) {
   auto it = _custom_operation_interpreters.find(id);
   if( it != _custom_operation_interpreters.end())
      return it->second;
   return std::shared_ptr<custom_operation_interpreter>();
}

void database_interface::notify_pre_apply_operation(operation_notification &note) {
   note.trx_id = _current_trx_id;
   note.block = _current_block_num;
   note.trx_in_block = _current_trx_in_block;
   note.op_in_trx = _current_op_in_trx;


   SOPHIATX_TRY_NOTIFY(pre_apply_operation, note)
}

void database_interface::notify_post_apply_operation(const operation_notification &note) {
   SOPHIATX_TRY_NOTIFY(post_apply_operation, note)
}

void database_interface::notify_applied_block(const signed_block &block) {
   SOPHIATX_TRY_NOTIFY(applied_block, block)
}

void database_interface::notify_on_pending_transaction(const signed_transaction &tx) {
   SOPHIATX_TRY_NOTIFY(on_pending_transaction, tx)
}

void database_interface::notify_on_pre_apply_transaction(const signed_transaction &tx) {
   SOPHIATX_TRY_NOTIFY(on_pre_apply_transaction, tx)
}

void database_interface::notify_on_applied_transaction(const signed_transaction &tx) {
   SOPHIATX_TRY_NOTIFY(on_applied_transaction, tx)
}

void database_interface::check_free_memory(bool force_print, uint32_t current_block_num) {
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

void database_interface::wipe( const fc::path& shared_mem_dir, bool include_blocks)
{
   close();
   chainbase::database::wipe( shared_mem_dir );
   if( include_blocks )
   {
      fc::remove_all( shared_mem_dir / "block_log" );
      fc::remove_all( shared_mem_dir / "block_log.index" );
   }
}

}
} //sophiatx::chain