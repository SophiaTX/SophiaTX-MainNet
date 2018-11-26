#pragma once

#include <sophiatx/protocol/operations.hpp>

#include <sophiatx/chain/sophiatx_object_types.hpp>

namespace sophiatx { namespace chain {

struct operation_notification
{
   operation_notification( const operation& o ) : op(o) {}

   transaction_id_type trx_id;
   uint32_t            block = 0;
   int32_t             trx_in_block = 0;
   uint16_t            op_in_trx = 0;
   uint64_t            virtual_op = 0;
   const operation&    op;
   account_name_type   fee_payer;
};

} }
