#ifndef SOPHIATX_HYBRID_DATABASE_HPP
#define SOPHIATX_HYBRID_DATABASE_HPP

#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/database/remote_db_api.hpp>
#include <sophiatx/chain/sophiatx_object_types.hpp>

#include <fc/rpc/websocket_api.hpp>
#include <fc/network/http/websocket.hpp>

namespace sophiatx {
namespace chain {

using sophiatx::protocol::signed_transaction;
using sophiatx::protocol::operation;
using sophiatx::protocol::authority;
using sophiatx::protocol::asset;
using sophiatx::protocol::asset_symbol_type;
using sophiatx::protocol::price;

/**
 *   @class database
 *   @brief tracks the blockchain state in an extensible manner
 */
class hybrid_database : public database_interface {
public:
   hybrid_database() : _remote_api_thread("hybrid_db_remote_api") {}

   virtual ~hybrid_database() {}

   inline void not_implemented() const {
      FC_ASSERT(false, "Not implemented for hybrid_database");
   }

   void open(const open_args &args, const genesis_state_type &genesis,
             const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  );

   uint32_t reindex(const open_args &args, const genesis_state_type &genesis,
                    const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  ) {
      not_implemented();
      return 0;
   }

   void close(bool rewind = true);

   bool is_known_block(const block_id_type &id) const {
      not_implemented();
      return false;
   }

   bool is_known_transaction(const transaction_id_type &id) const {
      not_implemented();
      return false;
   }

   block_id_type get_block_id_for_num(uint32_t block_num) const {
      not_implemented();
      return block_id_type();
   }

   optional<signed_block> fetch_block_by_id(const block_id_type &id) const {
      not_implemented();
      return optional<signed_block>();
   }

   optional<signed_block> fetch_block_by_number(uint32_t num) const {
      not_implemented();
      return optional<signed_block>();
   }

   const signed_transaction get_recent_transaction(const transaction_id_type &trx_id) const {
      not_implemented();
      return signed_transaction();
   }

   std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const {
      not_implemented();
      return std::vector<block_id_type>();
   }

   const account_object &get_account(const account_name_type &name) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      account_object *dummy = nullptr;
      return *dummy;
   }

   const dynamic_global_property_object &get_dynamic_global_properties() const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      dynamic_global_property_object *dummy = nullptr;
      return *dummy;
   }

   const hardfork_property_object &get_hardfork_property_object() const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      hardfork_property_object *dummy = nullptr;
      return *dummy;
   }

   void add_checkpoints(const flat_map<uint32_t, block_id_type> &checkpts) {
      not_implemented();
   }

   bool push_block(const signed_block &b, uint32_t skip = skip_nothing) {
      not_implemented();
      return false;
   }

   void push_transaction(const signed_transaction &trx, uint32_t skip = skip_nothing) {
      not_implemented();
   }

   void _maybe_warn_multiple_production(uint32_t height) const {
      not_implemented();
   }

   bool _push_block(const signed_block &b) {
      not_implemented();
      return false;
   }

   void _push_transaction(const signed_transaction &trx) {
      not_implemented();
   }

   void validate_invariants() const {
      not_implemented();
   }

   ///////////

   void start_sync_with_full_node();

   bool is_sync(fc::api<sophiatx::chain::remote_db_api> &con) const;

   const get_app_custom_messages_return::const_iterator &
   get_unprocessed_op(const get_app_custom_messages_return::const_iterator &start,
                      const get_app_custom_messages_return::const_iterator &end,
                      size_t size) const;

   void apply_custom_op(const received_object &obj);

   const hybrid_db_property_object &get_hybrid_db_properties() const;

private:
   uint64_t _head_op_number;
   uint64_t _head_op_id;
   uint64_t _app_id;
   fc::thread _remote_api_thread;
   bool _running;
   string _ws_endpoint;
};

}
}

#endif //SOPHIATX_HYBRID_DATABASE_HPP
