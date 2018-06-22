#pragma once

#include <sophiatx/protocol/authority.hpp>
#include <sophiatx/protocol/operations.hpp>
#include <sophiatx/protocol/sophiatx_operations.hpp>

#include <sophiatx/chain/buffer_type.hpp>
#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <sophiatx/chain/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace sophiatx { namespace chain {

   class operation_object : public object< operation_object_type, operation_object >
   {
      operation_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         operation_object( Constructor&& c, allocator< Allocator > a )
            :serialized_op( a.get_segment_manager() )
         {
            c( *this );
         }

         id_type              id;

         transaction_id_type  trx_id;
         uint32_t             block = 0;
         uint32_t             trx_in_block = 0;
         uint16_t             op_in_trx = 0;
         uint64_t             virtual_op = 0;
         time_point_sec       timestamp;
         buffer_type          serialized_op;
         account_name_type    fee_payer;
   };

   struct by_location;
   struct by_transaction_id;
   typedef multi_index_container<
      operation_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< operation_object, operation_id_type, &operation_object::id > >,
         ordered_unique< tag< by_location >,
            composite_key< operation_object,
               member< operation_object, uint32_t, &operation_object::block>,
               member< operation_object, uint32_t, &operation_object::trx_in_block>,
               member< operation_object, uint16_t, &operation_object::op_in_trx>,
               member< operation_object, uint64_t, &operation_object::virtual_op>,
               member< operation_object, operation_id_type, &operation_object::id>
            >
         >
#ifndef SKIP_BY_TX_ID
         ,
         ordered_unique< tag< by_transaction_id >,
            composite_key< operation_object,
               member< operation_object, transaction_id_type, &operation_object::trx_id>,
               member< operation_object, operation_id_type, &operation_object::id>
            >
         >
#endif
      >,
      allocator< operation_object >
   > operation_index;

   class account_history_object : public object< account_history_object_type, account_history_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         account_history_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account;
         uint32_t          sequence = 0;
         operation_id_type op;
   };

   struct by_account;
   struct by_account_rev;
   typedef multi_index_container<
      account_history_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< account_history_object, account_history_id_type, &account_history_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< account_history_object,
               member< account_history_object, account_name_type, &account_history_object::account>,
               member< account_history_object, uint32_t, &account_history_object::sequence>
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< uint32_t > >
         >
      >,
      allocator< account_history_object >
   > account_history_index;
} }

FC_REFLECT( sophiatx::chain::operation_object, (id)(trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(serialized_op)(fee_payer) )
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::operation_object, sophiatx::chain::operation_index )

FC_REFLECT( sophiatx::chain::account_history_object, (id)(account)(sequence)(op) )
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::account_history_object, sophiatx::chain::account_history_index )

namespace helpers
{
   template <>
   class index_statistic_provider<sophiatx::chain::operation_index>
   {
   public:
      typedef sophiatx::chain::operation_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);
         
         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
               info._item_additional_allocation +=
                  o.serialized_op.capacity()*sizeof(sophiatx::chain::buffer_type::value_type);
         }

         return info;
      }
   };

   template <>
   class index_statistic_provider<sophiatx::chain::account_history_index>
   {
   public:
      typedef sophiatx::chain::account_history_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            //for(const auto& o : index)
            //   info._item_additional_allocation += o.get_ops().capacity()*
            //      sizeof(sophiatx::chain::account_history_object::operation_container::value_type);
         }

         return info;
      }
   };

} /// namespace helpers

