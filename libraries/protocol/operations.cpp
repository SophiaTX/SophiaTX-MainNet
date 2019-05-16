#include <sophiatx/protocol/operations.hpp>

#include <sophiatx/protocol/operation_util_impl.hpp>

namespace sophiatx { namespace protocol {

struct is_ffop_visitor
{
   typedef bool result_type;
   bool operator()( const base_operation& v )const { return v.fee.amount == 0; }
};
bool is_fee_free_operation(const operation& op) {
   return op.visit( is_ffop_visitor() );
};


struct is_vop_visitor
{
   typedef bool result_type;

   template< typename T >
   bool operator()( const T& v )const { return v.is_virtual(); }
};

bool is_virtual_operation( const operation& op )
{
   return op.visit( is_vop_visitor() );
}

} } // sophiatx::protocol

SOPHIATX_DEFINE_OPERATION_TYPE( sophiatx::protocol::operation )
