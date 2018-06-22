#include <sophiatx/account_statistics/account_statistics_api.hpp>

namespace sophiatx { namespace account_statistics {

namespace detail
{
   class account_statistics_api_impl
   {
      public:
         account_statistics_api_impl( sophiatx::app::application& app )
            :_app( app ) {}

         sophiatx::app::application& _app;
   };
} // detail

account_statistics_api::account_statistics_api( const sophiatx::app::api_context& ctx )
{
   _my= std::make_shared< detail::account_statistics_api_impl >( ctx.app );
}

void account_statistics_api::on_api_startup() {}

} } // sophiatx::account_statistics
