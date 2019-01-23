#pragma once

#if defined(_MSC_VER) && _MSC_FULL_VER >= 190023918
#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#endif

#include <appbase/application.hpp>
#include <sophiatx/chain/database.hpp>

#include <boost/signals2.hpp>

#define SOPHIATX_CHAIN_PLUGIN_NAME "chain"

namespace sophiatx { namespace plugins { namespace chain {

namespace detail { class chain_plugin_impl; }

using std::unique_ptr;
using namespace appbase;
using namespace sophiatx::chain;

class chain_plugin : public plugin< chain_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES()

   chain_plugin();
   virtual ~chain_plugin();

   static const std::string& name() { static std::string name = SOPHIATX_CHAIN_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;
   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   bool accept_block( const sophiatx::chain::signed_block& block, bool currently_syncing, uint32_t skip );
   void accept_transaction( const sophiatx::chain::signed_transaction& trx );

   bool block_is_on_preferred_chain( const sophiatx::chain::block_id_type& block_id );

   void check_time_in_block( const sophiatx::chain::signed_block& block );

   template< typename MultiIndexType >
   bool has_index() const
   {
      return db().has_index< MultiIndexType >();
   }

   template< typename MultiIndexType >
   const chainbase::generic_index< MultiIndexType >& get_index() const
   {
      return db().get_index< MultiIndexType >();
   }

   template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
   const ObjectType* find( CompatibleKey&& key ) const
   {
      return db().find< ObjectType, IndexedByType, CompatibleKey >( key );
   }

   template< typename ObjectType >
   const ObjectType* find( chainbase::oid< ObjectType > key = chainbase::oid< ObjectType >() )
   {
      return db().find< ObjectType >( key );
   }

   template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
   const ObjectType& get( CompatibleKey&& key ) const
   {
      return db().get< ObjectType, IndexedByType, CompatibleKey >( key );
   }

   template< typename ObjectType >
   const ObjectType& get( const chainbase::oid< ObjectType >& key = chainbase::oid< ObjectType >() )
   {
      return db().get< ObjectType >( key );
   }

   // Exposed for backwards compatibility. In the future, plugins should manage their own internal database
   database& db();
   const database& db() const;

   // Emitted when the blockchain is syncing/live.
   // This is to synchronize plugins that have the chain plugin as an optional dependency.
   boost::signals2::signal<void()> on_sync;

   sophiatx::chain::signed_block generate_block( const fc::time_point_sec when, const account_name_type& witness_owner,
                                                            const fc::ecc::private_key& block_signing_private_key, uint32_t skip );
   int16_t set_write_lock_hold_time( int16_t new_time );

private:
   std::unique_ptr< detail::chain_plugin_impl > my;
};

} } } // sophiatx::plugins::chain
