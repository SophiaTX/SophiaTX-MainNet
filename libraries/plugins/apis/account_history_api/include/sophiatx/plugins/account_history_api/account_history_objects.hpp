#pragma once

#include <sophiatx/chain/history_object.hpp>
#include <sophiatx/protocol/operations.hpp>

namespace sophiatx { namespace plugins { namespace account_history {

struct api_operation_object
{
   api_operation_object() {}
   api_operation_object( const sophiatx::chain::operation_object& op_obj ) :
      trx_id( op_obj.trx_id ),
      block( op_obj.block ),
      trx_in_block( op_obj.trx_in_block ),
      virtual_op( op_obj.virtual_op ),
      timestamp( op_obj.timestamp ),
      fee_payer( op_obj.fee_payer )
   {
      op = fc::raw::unpack_from_buffer< sophiatx::protocol::operation >( op_obj.serialized_op, 0 );
   }

   sophiatx::protocol::transaction_id_type trx_id;
   uint32_t                               block = 0;
   uint32_t                               trx_in_block = 0;
   uint16_t                               op_in_trx = 0;
   uint64_t                               virtual_op = 0;
   fc::time_point_sec                     timestamp;
   sophiatx::protocol::operation             op;
   string                                 fee_payer;
};

} } } // sophiatx::plugins::account_history

FC_REFLECT( sophiatx::plugins::account_history::api_operation_object,
   (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op)(fee_payer) )
