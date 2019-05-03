#include <sophiatx/plugins/custom_api/custom_api_plugin.hpp>
#include <sophiatx/plugins/custom_api/custom_api.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>

namespace sophiatx { namespace plugins { namespace custom {

namespace detail {

class custom_api_impl
{
public:
   custom_api_impl() : _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() )  {
   }

   DECLARE_API_IMPL(
         (list_received_documents)
         (get_received_document)
         (get_app_custom_messages)
   )


   std::shared_ptr<chain::database_interface>  _db;
};


DEFINE_API_IMPL( custom_api_impl, get_received_document )
{
   const auto& idx = _db->get_index< chain::custom_content_index, chain::by_id >();
   auto res = idx.find(args.id);
   return *res;
}

DEFINE_API_IMPL( custom_api_impl, get_app_custom_messages)
{
   FC_ASSERT( args.limit <= CUSTOM_API_SINGLE_QUERY_LIMIT, "limit of ${l} is greater than maxmimum allowed", ("l",args.limit) );
   FC_ASSERT( args.start >= args.limit, "start must be greater than limit" );

   const auto& idx = _db->get_index< chain::custom_content_index, chain::by_app_id >();
   auto itr = idx.lower_bound( boost::make_tuple( args.app_id, args.start ) );
   auto end = idx.upper_bound( boost::make_tuple( args.app_id, std::max( int64_t(0), int64_t(itr->app_message_sequence) - args.limit ) ) );

   get_app_custom_messages_return result;
   result.clear();

   while( itr != end && result.size() < args.limit )
   {
      result[ itr->app_message_sequence ] = *itr;
      ++itr;
   }
   return result;
}


DEFINE_API_IMPL( custom_api_impl, list_received_documents )
{
   FC_ASSERT( args.count <= CUSTOM_API_SINGLE_QUERY_LIMIT, "limit of ${l} is greater than maxmimum allowed", ("l",args.count) );
   if(args.search_type == "by_sender"){
      uint64_t start = std::stoull(args.start);
      FC_ASSERT( start >= args.count, "start must be greater than limit" );
      const auto& idx = _db->get_index< chain::custom_content_index, chain::by_sender >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, std::max( int64_t(0), int64_t(itr->sender_sequence) - args.count ) ) );

      list_received_documents_return result; result.clear();
      while( itr != end && result.size() < args.count )
      {
         result[ itr->sender_sequence ] = *itr;
         ++itr;
      }
      return result;
   }else if(args.search_type == "by_recipient"){
      uint64_t start = std::stoull(args.start);
      FC_ASSERT( start >= args.count, "start must be greater than limit" );
      const auto& idx = _db->get_index< chain::custom_content_index, chain::by_recipient >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, std::max( int64_t(0), int64_t(itr->recipient_sequence) - args.count ) ) );

      list_received_documents_return result; result.clear();
      while( itr != end && result.size() < args.count)
      {
         result[ itr->recipient_sequence ] = *itr;
         ++itr;
      }

      return result;
   }else if(args.search_type == "by_sender_datetime"){
      fc::time_point_sec start = fc::time_point_sec::from_iso_string(args.start);
      const auto& idx = _db->get_index< chain::custom_content_index, chain::by_sender_time >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, fc::time_point_sec::min() ) );

      list_received_documents_return result; result.clear();
      while( itr != end && result.size() < args.count)
      {
         result[ itr->sender_sequence ] = *itr;
         ++itr;
      }

      return result;
   }else if(args.search_type == "by_recipient_datetime"){
      fc::time_point_sec start = fc::time_point_sec::from_iso_string(args.start);
      const auto& idx = _db->get_index< chain::custom_content_index, chain::by_recipient_time >();
      auto itr = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, fc::time_point_sec::min() ) );

      list_received_documents_return result; result.clear();
      while( itr != end && result.size() < args.count)
      {
         result[ itr->recipient_sequence ] = *itr;
         ++itr;
      }

      return result;

   }else if(args.search_type == "by_sender_reverse"){
      uint64_t start = std::stoull(args.start);
      //FC_ASSERT( start >= args.count, "start must be greater than limit" );
      const auto& idx = _db->get_index< chain::custom_content_index, chain::by_sender >();
      auto itr = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, int64_t(itr->sender_sequence) + args.count  ) );

      list_received_documents_return result; result.clear();
      while( itr != end && result.size() < args.count )
      {
         --itr;
         result[ itr->sender_sequence ] = *itr;
      }
      return result;
   }else if(args.search_type == "by_recipient_reverse"){
      uint64_t start = std::stoull(args.start);
      //FC_ASSERT( start >= args.count, "start must be greater than limit" );
      const auto& idx = _db->get_index< chain::custom_content_index, chain::by_recipient >();
      auto itr = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, int64_t(itr->recipient_sequence) + args.count ) ) ;

      list_received_documents_return result; result.clear();
      while( itr != end && result.size() < args.count)
      {
         --itr;
         result[ itr->recipient_sequence ] = *itr;
      }

      return result;
   }else if(args.search_type == "by_sender_datetime_reverse"){
      fc::time_point_sec start = fc::time_point_sec::from_iso_string(args.start);
      const auto& idx = _db->get_index< chain::custom_content_index, chain::by_sender_time >();
      auto itr = idx.upper_bound( boost::make_tuple( args.account_name, args.app_id, start ) );
      auto end = idx.lower_bound( boost::make_tuple( args.account_name, args.app_id, fc::time_point_sec::max() ) );

      list_received_documents_return result; result.clear();
      while( itr != end && result.size() < args.count)
      {
         --itr;
         result[ itr->sender_sequence ] = *itr;
      }

      return result;
   }else if(args.search_type == "by_recipient_datetime_reverse") {
      fc::time_point_sec start = fc::time_point_sec::from_iso_string(args.start);
      const auto &idx = _db->get_index<chain::custom_content_index, chain::by_recipient_time>();
      auto itr = idx.upper_bound(boost::make_tuple(args.account_name, args.app_id, start ));
      auto end = idx.lower_bound(boost::make_tuple(args.account_name, args.app_id, fc::time_point_sec::max()));

      list_received_documents_return result;
      result.clear();
      while( itr != end && result.size() < args.count ) {
         --itr;
         result[ itr->recipient_sequence ] = *itr;
      }
      return result;
   }else{
      FC_ASSERT(false, "Unknown search type argument");
   }

}


} // detail

custom_api::custom_api(): my( new detail::custom_api_impl() )
{
   JSON_RPC_REGISTER_API( SOPHIATX_CUSTOM_API_PLUGIN_NAME );
}

custom_api::~custom_api() {}

DEFINE_READ_APIS( custom_api,
      (list_received_documents)
      (get_received_document)
      (get_app_custom_messages)
)

} } } // sophiatx::plugins::custom
