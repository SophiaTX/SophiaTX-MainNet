#include <sophiatx/plugins/subscribe_api/subscribe_api_plugin.hpp>
#include <sophiatx/plugins/subscribe_api/subscribe_api.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/protocol/operations.hpp>

namespace sophiatx { namespace plugins { namespace subscribe {

namespace detail {

class subscribe_api_impl
{
public:
   subscribe_api_impl() : _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() )  {
      post_apply_connection = _db.post_apply_operation.connect( 0, [&]( const chain::operation_notification& note ){ on_operation(note); } );
   }

   DECLARE_API_IMPL(
         (custom_object_subscription)
   )

   void on_operation( const chain::operation_notification& note );

   chain::database&                 _db;
   boost::signals2::connection      post_apply_connection;
   std::vector<std::uint64_t>            _subscriptions;
   plugins::json_rpc::json_rpc_plugin* json_api;
};

void subscribe_api_impl::on_operation( const chain::operation_notification& note ){
   try {
      if( note.op.which() == sophiatx::protocol::operation::tag<sophiatx::protocol::custom_json_operation>::value ||
          note.op.which() == sophiatx::protocol::operation::tag<sophiatx::protocol::custom_binary_operation>::value ) {
         uint64_t co_id = _db.count<chain::custom_content_object>();
         const auto& idx = _db.get_index< chain::custom_content_index, chain::by_id >();
         auto res = idx.find(co_id);

         string text = "my_notification";
         fc::variant notify_body;
         fc::to_variant(text, notify_body);
         for(uint64_t i = 0; i< _subscriptions.size(); i++)
            json_api->send_ws_notice( i, _subscriptions[i], notify_body);

      }
   }catch(fc::assert_exception){}
}

DEFINE_API_IMPL( subscribe_api_impl, custom_object_subscription)
{
   _subscriptions.push_back(args);
   return _subscriptions.size()-1;
}





} // namespace detail

subscribe_api::subscribe_api(): my( new detail::subscribe_api_impl() )
{
   JSON_RPC_REGISTER_SUBSCRIBE_API( SOPHIATX_SUBSCRIBE_API_PLUGIN_NAME );
}

void subscribe_api::api_startup(){
   my->json_api = appbase::app().find_plugin< plugins::json_rpc::json_rpc_plugin >();
}

subscribe_api::~subscribe_api() {}

DEFINE_READ_APIS( subscribe_api,
     (custom_object_subscription)
)

} } } // sophiatx::plugins::subscribe
