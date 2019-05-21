#pragma once

#include <fc/container/flat.hpp>
#include <sophiatx/protocol/operations.hpp>
#include <sophiatx/protocol/transaction.hpp>

#include <fc/string_utils.hpp>

namespace sophiatx { namespace app {

using namespace fc;

void operation_get_impacted_accounts(
   const sophiatx::protocol::operation& op,
   fc::flat_set<protocol::account_name_type>& result );

void transaction_get_impacted_accounts(
   const sophiatx::protocol::transaction& tx,
   fc::flat_set<protocol::account_name_type>& result
   );

} } // sophiatx::app
