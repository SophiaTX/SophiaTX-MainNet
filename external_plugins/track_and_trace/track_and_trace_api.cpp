#include <sophiatx/plugins/track_and_trace/track_and_trace_plugin.hpp>
#include <sophiatx/plugins/track_and_trace/track_and_trace_api.hpp>
#include <sophiatx/plugins/track_and_trace/track_and_trace_objects.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

namespace detail {

class track_and_trace_api_impl
{
   public:
   track_and_trace_api_impl() : _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ) {}

   DECLARE_API_IMPL((get_current_holder)(get_holdings)(get_tracked_object_history)(get_transfer_requests)(get_item_details));

   std::shared_ptr<database_interface> _db;
};

DEFINE_API_IMPL(track_and_trace_api_impl, get_current_holder)
{
   get_current_holder_return final_result;
   const auto& holder_idx = _db->get_index< posession_index >().indices().get< by_serial >();
   const auto& holder_itr = holder_idx.find(args.serial);
   if(holder_itr != holder_idx.end())
      final_result.holder = holder_itr->holder;
   else
      final_result.holder = account_name_type();
   return final_result;
}

DEFINE_API_IMPL(track_and_trace_api_impl, get_holdings)
{
   get_holdings_return final_result;
   const auto& holder_idx = _db->get_index< posession_index >().indices().get< by_holder >();
   auto itr = holder_idx.lower_bound( args.holder );
   while (itr!=holder_idx.end() && itr->holder == args.holder ) {
      final_result.serials.push_back(itr->serial);
      itr++;
   }
   return final_result;
}

DEFINE_API_IMPL(track_and_trace_api_impl, get_tracked_object_history)
{
   get_tracked_object_history_return final_result;
   const auto& history_idx = _db->get_index< transfer_history_index >().indices().get< by_serial_date >();
   auto itr = history_idx.lower_bound( std::make_tuple(args.serial, fc::time_point_sec::min() ) );
   while (itr!=history_idx.end() && itr->serial == args.serial ) {
      final_result.history_items.push_back(*itr);
      itr++;
   }
   return final_result;
}

DEFINE_API_IMPL(track_and_trace_api_impl, get_transfer_requests)
{
   get_transfer_requests_return final_result;
   const auto& holder_idx = _db->get_index< posession_index >().indices().get< by_new_holder >();
   auto itr = holder_idx.lower_bound( args.new_holder );
   while (itr!=holder_idx.end() && itr->holder == args.new_holder ) {
      final_result.serials.push_back(itr->serial);
      itr++;
   }
   return final_result;
}

DEFINE_API_IMPL(track_and_trace_api_impl, get_item_details)
{
   const auto& holder_idx = _db->get_index< posession_index >().indices().get< by_serial >();
   const auto& holder_itr = holder_idx.find(args.serial);
   if(holder_itr != holder_idx.end())
      return *holder_itr;
   return get_item_details_return();
}

} // detail

track_and_trace_api::track_and_trace_api(): my( new detail::track_and_trace_api_impl() )
{
   JSON_RPC_REGISTER_API( SOPHIATX_TAT_PLUGIN_NAME );
}

track_and_trace_api::~track_and_trace_api() {}

DEFINE_READ_APIS( track_and_trace_api, (get_current_holder)(get_holdings)(get_tracked_object_history)(get_transfer_requests)(get_item_details) )

} } } // sophiatx::plugins::track_and_trace_plugin
