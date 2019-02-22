#ifndef ACCOUNT_BANDWIDTH_OBJECT_HPP
#define ACCOUNT_BANDWIDTH_OBJECT_HPP

#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace sophiatx { namespace chain {

using namespace sophiatx::chain;

class account_bandwidth_object : public object< account_bandwidth_object_type, account_bandwidth_object >
{
public:
   template< typename Constructor, typename Allocator >
   account_bandwidth_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   account_bandwidth_object() {}

   id_type           id;
   /**
    * account
    */
   account_name_type account;
   /**
    * Total (lifetime) transactions bandwidth [Bytes]
    */
   uint64_t          total_bandwidth;
   /**
    * Total (lifetime) transactions count
    */
   uint64_t          total_tx_count;
   /**
    * Actual(during last time frame <1,n> blocks - defined in config) fee free transactions bandwidth [Bytes]
    */
   uint64_t          act_fee_free_bandwidth;
   /**
    * Actual(during last time frame <1,n> blocks - defined in config) fee free transactions count
    */
   uint64_t          act_fee_free_tx_count;
   /**
    * last block_num since which act_fee_free_bandwidth and act_fee_free_tx_count are counted.
    * As soon as actual head_block_num - last_block_num_reset >= time_frame(from config), act_fee_free_bandwidth and act_fee_free_tx_count are
    * reset to 0
    */
   uint32_t          last_block_num_reset;
};

typedef oid< account_bandwidth_object > account_bandwidth_id_type;


struct by_account;
using account_bandwidth_index = multi_index_container <
      account_bandwidth_object,
      indexed_by <
            ordered_unique< tag< by_id >, member< account_bandwidth_object, account_bandwidth_id_type, &account_bandwidth_object::id > >,
            ordered_unique< tag< by_account >, member< account_bandwidth_object, account_name_type, &account_bandwidth_object::account > >
      >,
      allocator< account_bandwidth_object >
>;



} }  // sophiatx::chain

FC_REFLECT( sophiatx::chain::account_bandwidth_object,
            (id)(account)(total_bandwidth)(total_tx_count)(total_bandwidth)(act_fee_free_bandwidth)(act_fee_free_tx_count)(last_block_num_reset) )

CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::account_bandwidth_object, sophiatx::chain::account_bandwidth_index )



#endif // ACCOUNT_BANDWIDTH_OBJECT_HPP
