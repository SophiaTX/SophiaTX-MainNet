#pragma once
#include <fc/uint128.hpp>

#include <sophiatx/chain/sophiatx_object_types.hpp>

#include <sophiatx/protocol/asset.hpp>
#include <sophiatx/chain/get_config.hpp>

namespace sophiatx { namespace chain {

   using sophiatx::protocol::asset;
   using sophiatx::protocol::price;

   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public object< dynamic_global_property_object_type, dynamic_global_property_object>
   {
      public:
         template< typename Constructor, typename Allocator >
         dynamic_global_property_object( Constructor&& c, allocator< Allocator > a ) :
                 witness_required_vesting(chain::sophiatx_config::get<uint64_t>("SOPHIATX_INITIAL_WITNESS_REQUIRED_VESTING_BALANCE"), SOPHIATX_SYMBOL )
         {
            c( *this );
         }

         dynamic_global_property_object(){}

         id_type           id;

         uint32_t          head_block_number = 0;
         block_id_type     head_block_id;
         time_point_sec    time;
         account_name_type current_witness;

         asset       current_supply             = asset( 0, SOPHIATX_SYMBOL );
         asset       total_vesting_shares       = asset( 0, VESTS_SYMBOL );
         asset       total_reward_fund    = asset( 0, SOPHIATX_SYMBOL );

         asset       witness_required_vesting;


         /**
          *  Maximum block size is decided by the set of active witnesses which change every round.
          *  Each witness posts what they think the maximum size should be as part of their witness
          *  properties, the median size is chosen to be the maximum block size for the round.
          *
          *  @note the minimum value for maximum_block_size is defined by the protocol to prevent the
          *  network from getting stuck by witnesses attempting to set this too low.
          */
         uint32_t     maximum_block_size = 0;

         /**
          * The current absolute slot number.  Equal to the total
          * number of slots since genesis.  Also equal to the total
          * number of missed slots plus head_block_number.
          */
         uint64_t      current_aslot = 0;

         /**
          * used to compute witness participation.
          */
         fc::uint128_t recent_slots_filled;
         uint8_t       participation_count = 0; ///< Divide by 128 to compute participation percentage

         uint32_t last_irreversible_block_num = 0;

         chain_id_type chain_id;
         time_point_sec genesis_time;
         bool private_net = false;

   };

   typedef multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< dynamic_global_property_object, dynamic_global_property_object::id_type, &dynamic_global_property_object::id > >
      >,
      allocator< dynamic_global_property_object >
   > dynamic_global_property_index;

} } // sophiatx::chain

FC_REFLECT( sophiatx::chain::dynamic_global_property_object,
             (id)
             (head_block_number)
             (head_block_id)
             (time)
             (current_witness)
             (current_supply)
             (total_vesting_shares)
             (total_reward_fund)
             (maximum_block_size)
             (current_aslot)
             (witness_required_vesting)
             (recent_slots_filled)
             (participation_count)
             (last_irreversible_block_num)
             (private_net)
)

CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::dynamic_global_property_object, sophiatx::chain::dynamic_global_property_index )
