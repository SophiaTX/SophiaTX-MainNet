#ifndef SOPHIATX_HYBRID_DB_OBJECT_HPP
#define SOPHIATX_HYBRID_DB_OBJECT_HPP

#include <fc/uint128.hpp>

#include <sophiatx/chain/sophiatx_object_types.hpp>

namespace sophiatx { namespace chain {

/**
 * @class hybrid_db_property_object
 * @brief Maintains global state information
 */

class hybrid_db_property_object : public object< dynamic_global_property_object_type, hybrid_db_property_object>
{
public:
   template< typename Constructor, typename Allocator >
   hybrid_db_property_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   hybrid_db_property_object(){}

   id_type           id;

   uint64_t          head_op_number = 0;
};

typedef multi_index_container<
      hybrid_db_property_object,
      indexed_by<
            ordered_unique< tag< by_id >,
                  member< hybrid_db_property_object, hybrid_db_property_object::id_type, &hybrid_db_property_object::id > >
      >,
      allocator< hybrid_db_property_object >
>  hybrid_db_property_index;

} } // sophiatx::chain

FC_REFLECT( sophiatx::chain::hybrid_db_property_object,
            (id)
            (head_op_number)
)

CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::hybrid_db_property_object, sophiatx::chain::hybrid_db_property_index )


#endif //SOPHIATX_HYBRID_DB_OBJECT_HPP
