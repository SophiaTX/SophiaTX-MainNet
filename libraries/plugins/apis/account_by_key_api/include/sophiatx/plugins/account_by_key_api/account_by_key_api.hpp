#pragma once
#include <sophiatx/plugins/json_rpc/utility.hpp>

#include <sophiatx/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>

namespace sophiatx { namespace plugins { namespace account_by_key {

namespace detail
{
   class account_by_key_api_impl;
}

struct get_key_references_args
{
   std::vector< sophiatx::protocol::public_key_type > keys;
};

struct get_key_references_return
{
   std::vector< std::vector< sophiatx::protocol::account_name_type > > accounts;
};

class account_by_key_api
{
   public:
      account_by_key_api();
      ~account_by_key_api();

      DECLARE_API( (get_key_references) )

   private:
      std::unique_ptr< detail::account_by_key_api_impl > my;
};

} } } // sophiatx::plugins::account_by_key

FC_REFLECT( sophiatx::plugins::account_by_key::get_key_references_args,
   (keys) )

FC_REFLECT( sophiatx::plugins::account_by_key::get_key_references_return,
   (accounts) )
