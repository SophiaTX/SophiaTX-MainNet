#ifndef SOPHIATX_CHAIN_PLUGIN_FULL_HPP
#define SOPHIATX_CHAIN_PLUGIN_FULL_HPP
#pragma once

#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <boost/lockfree/queue.hpp>


namespace sophiatx { namespace plugins { namespace chain {

using std::unique_ptr;
using namespace appbase;
using namespace sophiatx::chain;

struct generate_block_request
{
   generate_block_request( const fc::time_point_sec w, const account_name_type& wo, const fc::ecc::private_key& priv_key, uint32_t s ) :
         when( w ),
         witness_owner( wo ),
         block_signing_private_key( priv_key ),
         skip( s ) {}

   const fc::time_point_sec when;
   const account_name_type& witness_owner;
   const fc::ecc::private_key& block_signing_private_key;
   uint32_t skip;
   signed_block block;
};

typedef fc::static_variant< const signed_block*, const signed_transaction*, generate_block_request* > write_request_ptr;
typedef fc::static_variant< boost::promise< void >*, fc::future< void >* > promise_ptr;

struct write_context
{
   write_request_ptr             req_ptr;
   uint32_t                      skip = 0;
   bool                          success = true;
   fc::optional< fc::exception > except;
   promise_ptr                   prom_ptr;

};

class chain_plugin_full : public chain_plugin
{
public:

   chain_plugin_full();
   virtual ~chain_plugin_full();

   void set_program_options( options_description& cli, options_description& cfg ) override;
   void plugin_initialize( const variables_map& options ) override;
   void plugin_startup() override;
   void plugin_shutdown() override;

   bool accept_block( const sophiatx::chain::signed_block& block, bool currently_syncing, uint32_t skip ) override;
   void accept_transaction( const sophiatx::chain::signed_transaction& trx ) override;

   void check_time_in_block( const sophiatx::chain::signed_block& block );

   sophiatx::chain::signed_block generate_block( const fc::time_point_sec& when, const account_name_type& witness_owner,
                                                 const fc::ecc::private_key& block_signing_private_key,
                                                 uint32_t skip ) override;

   int16_t set_write_lock_hold_time( int16_t new_time ) override;

   void start_write_processing();
   void stop_write_processing();

private:
   bool                             replay = false;
   bool                             check_locks = false;
   bool                             validate_invariants = false;
   bool                             dump_memory_details = false;
   uint32_t                         stop_replay_at = 0;
   uint32_t                         benchmark_interval = 0;
   genesis_state_type               genesis;
   flat_map<uint32_t,block_id_type> loaded_checkpoints;

   int16_t                          write_lock_hold_time=500;

   std::shared_ptr< std::thread >   write_processor_thread;
   boost::lockfree::queue< write_context* > write_queue;

   // TODO: temporary solution. DELETE when proper solution is implemented -> shared config object, which will contain also initminer mining public key.
   public_key_type init_mining_pubkey;
};

} } } // sophiatx::plugins::chain

#endif //SOPHIATX_CHAIN_PLUGIN_FULL_HPP
