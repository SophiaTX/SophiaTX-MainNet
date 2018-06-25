#pragma once

#include <sophiatx/protocol/authority.hpp>
#include <sophiatx/protocol/sophiatx_operations.hpp>
#include <sophiatx/protocol/misc_utilities.hpp>

#include <sophiatx/chain/sophiatx_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multiprecision/cpp_int.hpp>


namespace sophiatx { namespace chain {

   using sophiatx::protocol::asset;
   using sophiatx::protocol::price;
   using sophiatx::protocol::asset_symbol_type;

   typedef protocol::fixed_string< 16 > reward_fund_name_type;


   class escrow_object : public object< escrow_object_type, escrow_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         escrow_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         escrow_object(){}

         id_type           id;

         uint32_t          escrow_id = 20;
         account_name_type from;
         account_name_type to;
         account_name_type agent;
         time_point_sec    ratification_deadline;
         time_point_sec    escrow_expiration;
         asset             sophiatx_balance;
         asset             pending_fee;
         bool              to_approved = false;
         bool              agent_approved = false;
         bool              disputed = false;

         bool              is_approved()const { return to_approved && agent_approved; }
   };


   /**
    *  This object gets updated once per hour, on the hour
    */
   class feed_history_object  : public object< feed_history_object_type, feed_history_object >
   {
      feed_history_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         feed_history_object( Constructor&& c, allocator< Allocator > a )
            :price_history( a.get_segment_manager() )
         {
            c( *this );
         }

         id_type                                   id;
         asset_symbol_type                         symbol;

         price                                     current_median_history; ///< the current median of the price history, used as the base for convert operations
         bip::deque< price, allocator< price > >   price_history; ///< tracks this last week of median_feed one per hour
   };




   struct by_symbol;

   typedef multi_index_container<
      feed_history_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< feed_history_object, feed_history_id_type, &feed_history_object::id > >,
         ordered_unique< tag< by_symbol >, member< feed_history_object, asset_symbol_type, &feed_history_object::symbol> >
      >,
      allocator< feed_history_object >
   > feed_history_index;

   struct by_from_id;
   struct by_ratification_deadline;
   typedef multi_index_container<
      escrow_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< escrow_object, escrow_id_type, &escrow_object::id > >,
         ordered_unique< tag< by_from_id >,
            composite_key< escrow_object,
               member< escrow_object, account_name_type,  &escrow_object::from >,
               member< escrow_object, uint32_t, &escrow_object::escrow_id >
            >
         >,
         ordered_unique< tag< by_ratification_deadline >,
            composite_key< escrow_object,
               const_mem_fun< escrow_object, bool, &escrow_object::is_approved >,
               member< escrow_object, time_point_sec, &escrow_object::ratification_deadline >,
               member< escrow_object, escrow_id_type, &escrow_object::id >
            >,
            composite_key_compare< std::less< bool >, std::less< time_point_sec >, std::less< escrow_id_type > >
         >
      >,
      allocator< escrow_object >
   > escrow_index;


} } // sophiatx::chain

#include <sophiatx/chain/account_object.hpp>

FC_REFLECT( sophiatx::chain::feed_history_object,
             (id)(current_median_history)(price_history)(symbol) )
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::feed_history_object, sophiatx::chain::feed_history_index )

FC_REFLECT( sophiatx::chain::escrow_object,
             (id)(escrow_id)(from)(to)(agent)
             (ratification_deadline)(escrow_expiration)
             (sophiatx_balance)(pending_fee)
             (to_approved)(agent_approved)(disputed) )
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::escrow_object, sophiatx::chain::escrow_index )

