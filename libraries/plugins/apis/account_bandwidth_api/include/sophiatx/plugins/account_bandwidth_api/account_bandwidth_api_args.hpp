#ifndef SOPHIATX_ACCOUNT_BANDWIDTH_API_ARGS_HPP
#define SOPHIATX_ACCOUNT_BANDWIDTH_API_ARGS_HPP

#include <sophiatx/protocol/types.hpp>
#include <sophiatx/chain/account_bandwidth_object.hpp>

namespace sophiatx { namespace plugins { namespace account_bandwidth_api {

struct get_account_bandwidth_args
{
   protocol::account_name_type account;
};

struct get_account_bandwidth_return
{
   optional<chain::account_bandwidth_object> bandwidth;
};

} } } // sophiatx::plugins::account_bandwidth_api

FC_REFLECT( sophiatx::plugins::account_bandwidth_api::get_account_bandwidth_args,
            (account) )

FC_REFLECT( sophiatx::plugins::account_bandwidth_api::get_account_bandwidth_return,
            (bandwidth) )

#endif //SOPHIATX_ACCOUNT_BANDWIDTH_API_ARGS_HPP
