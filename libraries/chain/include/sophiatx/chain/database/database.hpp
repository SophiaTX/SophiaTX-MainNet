/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 */
#pragma once
#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/evaluator_registry.hpp>

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
class database : public database_interface {
public:
   database() {}

   virtual ~database();


   void set_producing(bool p) { _is_producing = p; }

   /**
    * @brief Open a database, creating a new one if necessary
    *
    * Opens a database in the specified directory. If no initialized database is found the database
    * will be initialized with the default state.
    *
    * @param data_dir Path to open or create database in
    */
   void open(const open_args &args, const genesis_state_type &genesis,
             const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  );

   /**
    * @brief Rebuild object graph from block history and open detabase
    *
    * This method may be called after or instead of @ref database::open, and will rebuild the object graph by
    * replaying blockchain history. When this method exits successfully, the database will be open.
    *
    * @return the last replayed block number.
    */
   uint32_t reindex(const open_args &args, const genesis_state_type &genesis,
                    const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  );

   void close(bool rewind = true);

   //////////////////// db_block.cpp ////////////////////

   /**
    *  @return true if the block is in our fork DB or saved to disk as
    *  part of the official chain, otherwise return false
    */
   bool is_known_block(const block_id_type &id) const;

   bool is_known_transaction(const transaction_id_type &id) const;

   block_id_type find_block_id_for_num(uint32_t block_num) const;

   block_id_type get_block_id_for_num(uint32_t block_num) const;

   optional<signed_block> fetch_block_by_id(const block_id_type &id) const;

   optional<signed_block> fetch_block_by_number(uint32_t num) const;

   const signed_transaction get_recent_transaction(const transaction_id_type &trx_id) const;

   std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;

   const witness_object &get_witness(const account_name_type &name) const;

   const witness_object *find_witness(const account_name_type &name) const;

   const account_object &get_account(const account_name_type &name) const;

   const account_object *find_account(const account_name_type &name) const;

   const account_object *find_account(const account_id_type &id) const;

   const escrow_object &get_escrow(const account_name_type &name, uint32_t escrow_id) const;

   const escrow_object *find_escrow(const account_name_type &name, uint32_t escrow_id) const;

   const application_object &get_application(const string &name) const;

   const application_object &get_application_by_id(const application_id_type id) const;

   const application_buying_object &
   get_application_buying(const account_name_type &buyer, const application_id_type app_id) const;

   const dynamic_global_property_object &get_dynamic_global_properties() const;

   const economic_model_object &get_economic_model() const;

   const feed_history_object &get_feed_history(asset_symbol_type a) const;

   const witness_schedule_object &get_witness_schedule_object() const;

   const hardfork_property_object &get_hardfork_property_object() const;

   /**
    *  Deducts fee from the account and the share supply
    */
   void pay_fee(const account_object &a, asset fee);

   /**
    *  Calculate the percent of block production slots that were missed in the
    *  past 128 blocks, not including the current block.
    */
   uint32_t witness_participation_rate() const;

   void add_checkpoints(const flat_map<uint32_t, block_id_type> &checkpts);

   bool before_last_checkpoint() const;

   bool push_block(const signed_block &b, uint32_t skip = skip_nothing);

   void push_transaction(const signed_transaction &trx, uint32_t skip = skip_nothing);

   void _maybe_warn_multiple_production(uint32_t height) const;

   bool _push_block(const signed_block &b);

   void _push_transaction(const signed_transaction &trx);

   signed_block generate_block(
         const fc::time_point_sec when,
         const account_name_type &witness_owner,
         const fc::ecc::private_key &block_signing_private_key,
         uint32_t skip
   );

   signed_block _generate_block(
         const fc::time_point_sec when,
         const account_name_type &witness_owner,
         const fc::ecc::private_key &block_signing_private_key
   );

   void pop_block();

   void clear_pending();

   //////////////////// db_witness_schedule.cpp ////////////////////

   /**
    * @brief Get the witness scheduled for block production in a slot.
    *
    * slot_num always corresponds to a time in the future.
    *
    * If slot_num == 1, returns the next scheduled witness.
    * If slot_num == 2, returns the next scheduled witness after
    * 1 block gap.
    *
    * Use the get_slot_time() and get_slot_at_time() functions
    * to convert between slot_num and timestamp.
    *
    * Passing slot_num == 0 returns SOPHIATX_NULL_WITNESS
    */
   account_name_type get_scheduled_witness(uint32_t slot_num) const;

   /**
    * Get the time at which the given slot occurs.
    *
    * If slot_num == 0, return time_point_sec().
    *
    * If slot_num == N for N > 0, return the Nth next
    * block-interval-aligned time greater than head_block_time().
    */
   fc::time_point_sec get_slot_time(uint32_t slot_num) const;

   /**
    * Get the last slot which occurs AT or BEFORE the given time.
    *
    * The return value is the greatest value N such that
    * get_slot_time( N ) <= when.
    *
    * If no such N exists, return 0.
    */
   uint32_t get_slot_at_time(fc::time_point_sec when) const;

   bool is_private_net() const {
      return get_dynamic_global_properties().private_net;
   }

   asset to_sbd(const asset &sophiatx, asset_symbol_type to_symbol) const {
      return util::to_sbd(get_feed_history(to_symbol).current_median_history, sophiatx);
   }

   asset to_sophiatx(const asset &sbd) const {
      return util::to_sophiatx(get_feed_history(sbd.symbol).current_median_history, sbd);
   }

   node_property_object &node_properties() {
      return _node_property_object;
   }

   void vest(const account_name_type &name, const share_type delta);

   void vest(const account_object &a, const share_type delta);

   void create_vesting(const account_object &a, const asset &delta);

   void create_vesting(const account_name_type &name, const asset &delta);

   void adjust_balance(const account_object &a, const asset &delta);

   void adjust_balance(const account_name_type &name, const asset &delta);

   void adjust_supply(const asset &delta);

   void update_owner_authority(const account_object &account, const authority &owner_authority);

   asset get_balance(const account_object &a, asset_symbol_type symbol) const;

   asset get_balance(const string &aname, asset_symbol_type symbol) const {
      return get_balance(get_account(aname), symbol);
   }

   /** this updates the votes for witnesses as a result of account voting proxy changing */
   void adjust_proxied_witness_votes(const account_object &a,
                                     const std::array<share_type, SOPHIATX_MAX_PROXY_RECURSION_DEPTH + 1> &delta,
                                     int depth = 0);

   /** this updates the votes for all witnesses as a result of account VESTS changing */
   void adjust_proxied_witness_votes(const account_object &a, share_type delta, int depth = 0);

   /** this is called by `adjust_proxied_witness_votes` when account proxy to self */
   void adjust_witness_votes(const account_object &a, share_type delta);

   /** this updates the vote of a single witness as a result of a vote being added or removed*/
   void adjust_witness_vote(const witness_object &obj, share_type delta);

   /** clears all vote records for a particular account but does not update the
    * witness vote totals.  Vote totals should be updated first via a call to
    * adjust_proxied_witness_votes( a, -a.witness_vote_weight() )
    */
   void clear_witness_votes(const account_object &a);

   void process_vesting_withdrawals();

   void process_interests();

   void process_funds();

   void account_recovery_processing();

   void expire_escrow_ratification();

   void update_median_feeds();

   //////////////////// db_init.cpp ////////////////////

   void initialize_evaluators();

   /// Reset the object graph in-memory
   void initialize_indexes();

   void init_genesis(genesis_state_type genesis, chain_id_type chain_id,
                     const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */ );

   /**
    *  This method validates transactions without adding it to the pending state.
    *  @throw if an error occurs
    */
   void validate_transaction(const signed_transaction &trx);

   void retally_witness_votes();


   /* For testing and debugging only. Given a hardfork
      with id N, applies all hardforks with id <= N */
   void set_hardfork(uint32_t hardfork, bool process_now = true);

   void check_free_memory(bool force_print, uint32_t current_block_num);

   void validate_invariants() const;

   asset process_operation_fee(const operation &op);

   account_name_type get_fee_payer(const operation &op);

   optional<account_name_type> get_sponsor(const account_name_type &who) const;

protected:
   //Mark pop_undo() as protected -- we do not want outside calling pop_undo(); it should call pop_block() instead
   //void pop_undo() { object_database::pop_undo(); }
   void notify_changed_objects();

private:
   optional<chainbase::database::session> _pending_tx_session;

   void apply_block(const signed_block &next_block, uint32_t skip = skip_nothing);

   void apply_transaction(const signed_transaction &trx, uint32_t skip = skip_nothing);

   void _apply_block(const signed_block &next_block);

   void _apply_transaction(const signed_transaction &trx);

   void apply_operation(const operation &op);

   /**
    * @brief Process transaction in terms of bandwidth and updates corresponding accounts
    * @note see update_account_bandwidth for possible scenarios when exception is thrown through SOPHIATX_ASSERT
    *
    * @param trx
    */
   void process_tx_bandwidth(const signed_transaction& trx);

   /**
    * @brief Updates account bandwidth according to provided transaction data
    * @throws tx_exceeded_bandwidth if at least one of the following conditions is met:
    *           1. max allowed fee-free operations bandwidth for provided account was exceeded
    *           2. max allowed fee-free operations count for provided account was exceeded
    *
    * @param account
    * @param trx
    */
   void update_account_bandwidth(const account_name_type& account, const account_bandwidth_object& trx_bandwidth_data);

   ///Steps involved in applying a new block
   ///@{

   const witness_object &validate_block_header(uint32_t skip, const signed_block &next_block) const;

   void create_block_summary(const signed_block &next_block);

   void clear_null_account_balance();

   void update_global_dynamic_data(const signed_block &b);

   void update_signing_witness(const witness_object &signing_witness, const signed_block &new_block);

   void update_last_irreversible_block();

   void clear_expired_transactions();

   void process_header_extensions(const signed_block &next_block);

   void init_hardforks();

   void process_hardforks();

   void apply_hardfork(uint32_t hardfork);

   void modify_balance(const account_object &a, const asset &delta, bool check_balance);

   void recalculate_all_votes();

   evaluator_registry<operation> _evaluator_registry;

   node_property_object _node_property_object;

   fork_database _fork_db;
   fc::time_point_sec _hardfork_times[SOPHIATX_NUM_HARDFORKS + 1];
   protocol::hardfork_version _hardfork_versions[SOPHIATX_NUM_HARDFORKS + 1];

   block_log _block_log;

   flat_map<uint32_t, block_id_type> _checkpoints;
};

}
}
