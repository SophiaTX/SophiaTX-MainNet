#include <steem/plugins/custom_api/custom_api_plugin.hpp>
#include <steem/plugins/custom_api/custom_api.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace custom {

namespace detail {

class custom_api_impl
{
public:
   custom_api_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() )  {}

   DECLARE_API_IMPL(
         (get_received)
   )

   chain::database& _db;
};

DEFINE_API_IMPL( custom_api_impl, get_received )
{
   FC_ASSERT( args.count <= 10000, "limit of ${l} is greater than maxmimum allowed", ("l",args.count) );
   if(args.search_type == "by_sender"){
      uint64_t start = std::stoull(args.start);
      FC_ASSERT( start >= args.count, "start must be greater than limit" );
      const auto& idx = _db.get_index< chain::custom_content_index, chain::by_sender >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, std::max( int64_t(0), int64_t(itr->sender_sequence) - args.count ) ) );

      get_received_return result;
      while( itr != end )
      {
         result.history[ itr->sender_sequence ] = *itr;
         ++itr;
      }

      return result;
   }else if(args.search_type == "by_recipient"){
      uint64_t start = std::stoull(args.start);
      FC_ASSERT( start >= args.count, "start must be greater than limit" );
      const auto& idx = _db.get_index< chain::custom_content_index, chain::by_recipient >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, std::max( int64_t(0), int64_t(itr->recipient_sequence) - args.count ) ) );

      get_received_return result;
      while( itr != end )
      {
         result.history[ itr->recipient_sequence ] = *itr;
         ++itr;
      }

      return result;
   }else if(args.search_type == "by_sender_datetime"){
      fc::time_point_sec start = fc::time_point_sec::from_iso_string(args.start);
      const auto& idx = _db.get_index< chain::custom_content_index, chain::by_sender_time >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );

      get_received_return result;
      while( itr != idx.end() && itr->sender == args.account_name && itr->app_id == args.app_id && result.history.size() < args.count)
      {
         result.history[ itr->sender_sequence ] = *itr;
         ++itr;
      }

      return result;

   }else if(args.search_type == "by_recipient_datetime"){
      fc::time_point_sec start = fc::time_point_sec::from_iso_string(args.start);
      const auto& idx = _db.get_index< chain::custom_content_index, chain::by_recipient_time >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );

      get_received_return result;
      while( itr != idx.end() && itr->recipient == args.account_name && itr->app_id == args.app_id && result.history.size() < args.count)
      {
         result.history[ itr->recipient_sequence ] = *itr;
         ++itr;
      }

      return result;

   }else{
      FC_ASSERT(false, "Unknown search type argument");
   }

}


} // detail

custom_api::custom_api(): my( new detail::custom_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_CUSTOM_API_PLUGIN_NAME );
}

custom_api::~custom_api() {}

DEFINE_READ_APIS( custom_api,
      (get_received)
)

} } } // steem::plugins::custom
