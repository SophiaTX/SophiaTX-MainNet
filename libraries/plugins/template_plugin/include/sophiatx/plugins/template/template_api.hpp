#pragma once
#include <sophiatx/plugins/json_rpc/utility.hpp>

#include <sophiatx/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>

namespace sophiatx { namespace plugins { namespace template_plugin {

namespace detail
{
   class template_api_impl;
}

struct example_call_args
{
   uint64_t arg1;
   uint64_t arg2;
};

struct example_call_return
{
   uint64_t sum;
   uint64_t mul;
};

class template_api
{
   public:
      template_api();
      ~template_api();

      DECLARE_API( (example_call) )

   private:
      std::unique_ptr< detail::template_api_impl > my;
};

} } } // sophiatx::plugins::template_plugin

FC_REFLECT( sophiatx::plugins::template_plugin::example_call_args,
   (arg1)(arg2) )

FC_REFLECT( sophiatx::plugins::template_plugin::example_call_return,
   (sum)(mul) )
