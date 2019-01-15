#ifndef SOPHIATX_HYBRID_DATABASE_HPP
#define SOPHIATX_HYBRID_DATABASE_HPP

#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/database/remote_db_api.hpp>
#include <sophiatx/chain/sophiatx_object_types.hpp>

#include <fc/rpc/websocket_api.hpp>

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
   hybrid_database() {}

   virtual ~hybrid_database() {}

   inline void not_implemented() const {
      FC_ASSERT(false, "Not implemented for hybrid_database");
   }

   void open(const open_args &args, const genesis_state_type &genesis,
             const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  );

   uint32_t reindex(const open_args &args, const genesis_state_type &genesis,
                    const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  );

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

   const witness_object &get_witness(const account_name_type &name) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      witness_object *dummy = nullptr;
      return *dummy;
   }

   const witness_object *find_witness(const account_name_type &name) const {
      not_implemented();
      return nullptr;
   }

   const account_object &get_account(const account_name_type &name) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      account_object *dummy = nullptr;
      return *dummy;
   }

   const account_object *find_account(const account_name_type &name) const {
      not_implemented();
      return nullptr;
   }

   const account_object *find_account(const account_id_type &id) const {
      not_implemented();
      return nullptr;
   }

   const escrow_object &get_escrow(const account_name_type &name, uint32_t escrow_id) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      escrow_object *dummy = nullptr;
      return *dummy;
   }

   const escrow_object *find_escrow(const account_name_type &name, uint32_t escrow_id) const {
      not_implemented();
      return nullptr;
   }

   const application_object &get_application(const string &name) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      application_object *dummy = nullptr;
      return *dummy;
   }

   const application_object &get_application_by_id(const application_id_type id) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      application_object *dummy = nullptr;
      return *dummy;
   }

   const application_buying_object &
   get_application_buying(const account_name_type &buyer, const application_id_type app_id) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      application_buying_object *dummy = nullptr;
      return *dummy;
   }

   const dynamic_global_property_object &get_dynamic_global_properties() const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      dynamic_global_property_object *dummy = nullptr;
      return *dummy;
   }

   const economic_model_object &get_economic_model() const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      economic_model_object *dummy = nullptr;
      return *dummy;
   }

   const feed_history_object &get_feed_history(asset_symbol_type a) const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      feed_history_object *dummy = nullptr;
      return *dummy;
   }

   const witness_schedule_object &get_witness_schedule_object() const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      witness_schedule_object *dummy = nullptr;
      return *dummy;
   }

   const hardfork_property_object &get_hardfork_property_object() const {
      not_implemented();
      //TODO ugly hack so it will compile without warnings
      hardfork_property_object *dummy = nullptr;
      return *dummy;
   }

   void pay_fee(const account_object &a, asset fee) {
      not_implemented();
   }

   uint32_t witness_participation_rate() const {
      not_implemented();
      return 0;
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

   void pop_block() {
      not_implemented();
   }

   void clear_pending() {
      not_implemented();
   }

   account_name_type get_scheduled_witness(uint32_t slot_num) const {
      not_implemented();
      return account_name_type();
   }

   fc::time_point_sec get_slot_time(uint32_t slot_num) const {
      not_implemented();
      return fc::time_point_sec();
   }

   uint32_t get_slot_at_time(fc::time_point_sec when) const {
      not_implemented();
      return 0;
   }

   void vest(const account_name_type &name, const share_type delta) {
      not_implemented();
   }

   void vest(const account_object &a, const share_type delta) {
      not_implemented();
   }

   void adjust_balance(const account_object &a, const asset &delta) {
      not_implemented();
   }

   void adjust_balance(const account_name_type &name, const asset &delta) {
      not_implemented();
   }

   void adjust_supply(const asset &delta) {
      not_implemented();
   }

   void update_owner_authority(const account_object &account, const authority &owner_authority) {
      not_implemented();
   }

   asset get_balance(const account_object &a, asset_symbol_type symbol) const {
      not_implemented();
      return asset();
   }

   void adjust_proxied_witness_votes(const account_object &a,
                                     const std::array<share_type, SOPHIATX_MAX_PROXY_RECURSION_DEPTH + 1> &delta,
                                     int depth = 0) {
      not_implemented();
   }

   void adjust_proxied_witness_votes(const account_object &a, share_type delta, int depth = 0) {
      not_implemented();
   }

   void adjust_witness_votes(const account_object &a, share_type delta) {
      not_implemented();
   }

   void adjust_witness_vote(const witness_object &obj, share_type delta) {
      not_implemented();
   }

   void clear_witness_votes(const account_object &a) {
      not_implemented();
   }

   void retally_witness_votes() {
      not_implemented();
   }

   void set_hardfork(uint32_t hardfork, bool process_now = true) {
      not_implemented();
   }

   void validate_invariants() const {
      not_implemented();
   }

   optional<account_name_type> get_sponsor(const account_name_type &who) const {
      not_implemented();
      return nullptr;
   }
   ///////////

   void run();

   void apply_custom_op(const received_object &obj);

   const hybrid_db_property_object &get_hybrid_db_properties() const;

private:
   fc::api<remote_db_api> _remote_api;
   boost::signals2::scoped_connection _closed_connection;
   std::shared_ptr<fc::rpc::websocket_api_connection> _apic;
   uint64_t _head_op_number;
   uint64_t _app_id;

};

}
}

#endif //SOPHIATX_HYBRID_DATABASE_HPP
