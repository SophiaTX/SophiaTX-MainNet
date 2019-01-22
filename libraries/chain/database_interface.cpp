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