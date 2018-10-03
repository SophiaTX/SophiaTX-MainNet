#pragma once

#include <sophiatx/plugins/account_history_api/account_history_objects.hpp>
#include <sophiatx/protocol/types.hpp>
#include <sophiatx/protocol/transaction.hpp>

namespace sophiatx { namespace plugins { namespace account_history {

struct get_ops_in_block_args
{
   uint32_t block_num;
   bool     only_virtual;
};

struct get_ops_in_block_return
{
   vector< api_operation_object > ops;
};


struct get_transaction_args
{
   sophiatx::protocol::transaction_id_type id;
};

typedef sophiatx::protocol::annotated_signed_transaction get_transaction_return;


struct get_account_history_args
{
   sophiatx::protocol::account_name_type  account;
   int64_t                                start = -1;
   uint32_t                               limit = 1000;
   bool                                   reverse_order = false;
};

struct get_account_history_return
{
   std::map< uint32_t, api_operation_object > history;
};


} } } // sophiatx::plugins::account_history

FC_REFLECT( sophiatx::plugins::account_history::get_ops_in_block_args,
   (block_num)(only_virtual) )

FC_REFLECT( sophiatx::plugins::account_history::get_ops_in_block_return,
   (ops) )

FC_REFLECT( sophiatx::plugins::account_history::get_transaction_args,
   (id) )

FC_REFLECT( sophiatx::plugins::account_history::get_account_history_args,
   (account)(start)(limit)(reverse_order) )

FC_REFLECT( sophiatx::plugins::account_history::get_account_history_return,
   (history) )
