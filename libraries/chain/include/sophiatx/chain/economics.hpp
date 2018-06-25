#pragma once
#include <sophiatx/protocol/config.hpp>
#include <sophiatx/protocol/types.hpp>
#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <sophiatx/chain/database.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>




namespace sophiatx { namespace chain {
namespace bip = boost::interprocess;

using namespace sophiatx::protocol;

//technically, anything in economic_model_object pools do not exists yet; any transfers (by withdraw and add methods) must affect current_supply
class economic_model_object: public object< economic_model_object_type, economic_model_object> {
public:
   template< typename Constructor, typename Allocator >
   economic_model_object( Constructor&& c, allocator< Allocator > a ) : historic_supply( historic_supply_allocator_type( a.get_segment_manager() ) )
   {
      c( *this );
   }

   economic_model_object() =delete;

   id_type           id;

   share_type mining_pool_from_coinbase;
   share_type mining_pool_from_fees;
   share_type interest_pool_from_coinbase;
   share_type interest_pool_from_fees;
   share_type promotion_pool;
   share_type initial_promotion_pool;
   share_type init_supply;
   share_type total_supply;

   typedef bip::allocator< economic_model_object, bip::managed_mapped_file::segment_manager >  allocator_type;
   typedef bip::allocator< std::pair< uint32_t, share_type >, bip::managed_mapped_file::segment_manager > historic_supply_allocator_type;
   typedef bip::flat_map< uint32_t, share_type, std::less< uint32_t >, historic_supply_allocator_type > historic_supply_map;

   historic_supply_map historic_supply;
   share_type accumulated_supply;

   void init_economics(share_type init_supply, share_type total_supply);
   void record_block(uint32_t block, share_type current_supply);
   share_type get_mining_reward(uint32_t block_number) const;
   share_type withdraw_mining_reward(uint32_t block_number, uint32_t nominator, uint32_t denominator);
   share_type withdraw_interests(share_type holding, uint32_t period);
   share_type get_available_promotion_pool(uint32_t block_number) const;
   share_type withdraw_from_promotion_pool(share_type amount, uint32_t block_number);
   void add_fee(share_type fee);



};

typedef multi_index_container<
      economic_model_object,
      indexed_by<
            ordered_unique< tag< by_id >,
                  member< economic_model_object, economic_model_object::id_type, &economic_model_object::id > >
      >,
      allocator< economic_model_object >
> economic_model_index;

}}//namespace





FC_REFLECT(sophiatx::chain::economic_model_object, (id)
           (mining_pool_from_coinbase)
           (mining_pool_from_fees)
           (interest_pool_from_coinbase)
           (interest_pool_from_fees)
           (promotion_pool)
           (initial_promotion_pool)
           (init_supply)
           (total_supply)
      (historic_supply)
      (accumulated_supply)
)
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::economic_model_object, sophiatx::chain::economic_model_index )
