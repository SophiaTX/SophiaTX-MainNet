#pragma once
#include <sophiatx/plugins/json_rpc/utility.hpp>

#include <sophiatx/chain/custom_content_object.hpp>

#include <sophiatx/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace sophiatx { namespace plugins { namespace subscribe {


namespace detail { class subscribe_api_impl; }
using plugins::json_rpc::void_type;



typedef uint64_t custom_object_subscription_args;

typedef uint64_t custom_object_subscription_return;

class subscribe_api
{
public:
   subscribe_api();
   ~subscribe_api();
   void api_startup();

   DECLARE_API(
   (custom_object_subscription)
   )

private:
   std::unique_ptr< detail::subscribe_api_impl > my;
};

} } } // sophiatx::plugins::subscribe


