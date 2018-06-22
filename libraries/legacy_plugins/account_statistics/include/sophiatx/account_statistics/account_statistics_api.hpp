#pragma once

#include <sophiatx/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace sophiatx { namespace app {
   struct api_context;
} }

namespace sophiatx { namespace account_statistics {

namespace detail
{
   class account_statistics_api_impl;
}

class account_statistics_api
{
   public:
      account_statistics_api( const sophiatx::app::api_context& ctx );

      void on_api_startup();

   private:
      std::shared_ptr< detail::account_statistics_api_impl > _my;
};

} } // sophiatx::account_statistics

FC_API( sophiatx::account_statistics::account_statistics_api, )
