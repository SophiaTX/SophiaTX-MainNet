#pragma once
#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/witness/witness_objects.hpp>

#include <sophiatx/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace sophiatx { namespace plugins { namespace witness {

namespace detail
{
   class witness_api_impl;
}

struct get_account_bandwidth_args
{
   protocol::account_name_type   account;
   bandwidth_type                type;
};

typedef account_bandwidth_object api_account_bandwidth_object;

struct get_account_bandwidth_return
{
   optional< api_account_bandwidth_object > bandwidth;
};

typedef json_rpc::void_type get_reserve_ratio_args;
typedef reserve_ratio_object get_reserve_ratio_return;

class witness_api
{
   public:
      witness_api();
      ~witness_api();

      DECLARE_API(
         (get_account_bandwidth)
         (get_reserve_ratio)
      )

   private:
      std::unique_ptr< detail::witness_api_impl > my;
};

} } } // sophiatx::plugins::witness

FC_REFLECT( sophiatx::plugins::witness::get_account_bandwidth_args,
            (account)(type) )

FC_REFLECT( sophiatx::plugins::witness::get_account_bandwidth_return,
            (bandwidth) )
