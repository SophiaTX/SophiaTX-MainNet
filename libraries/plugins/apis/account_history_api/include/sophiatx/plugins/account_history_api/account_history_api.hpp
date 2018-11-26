#pragma once
#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/account_history_api/account_history_args.hpp>

namespace sophiatx { namespace plugins { namespace account_history {


namespace detail { class account_history_api_impl; }

class account_history_api
{
   public:
      account_history_api();
      ~account_history_api();

      DECLARE_API(
         (get_ops_in_block)
         (get_transaction)
         (get_account_history)
      )

   private:
      std::unique_ptr< detail::account_history_api_impl > my;
};

} } } // sophiatx::plugins::account_history
