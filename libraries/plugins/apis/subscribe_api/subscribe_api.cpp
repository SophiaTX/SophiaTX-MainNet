#include <sophiatx/plugins/subscribe_api/subscribe_api_plugin.hpp>
#include <sophiatx/plugins/subscribe_api/subscribe_api.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/protocol/operations.hpp>
#include <sophiatx/plugins/custom_api/custom_api_plugin.hpp>
#include <sophiatx/plugins/custom_api/custom_api.hpp>


namespace sophiatx { namespace plugins { namespace subscribe {

namespace detail {

struct custom_content_callback;


class subscribe_api_impl
{
public:
   subscribe_api_impl(subscribe_api_plugin&plugin) : _db( plugin.app()->get_plugin< sophiatx::plugins::chain::chain_plugin >().db() )  {
      post_apply_connection = _db->post_apply_operation.connect( 0, [&]( const chain::operation_notification& note ){ on_operation(note); } );
      json_api = plugin.app()->find_plugin< plugins::json_rpc::json_rpc_plugin >();
      auto custom = plugin.app()->find_plugin< custom::custom_api_plugin>();
      if( custom != nullptr )
         custom_api = custom->api;
   }

   DECLARE_API_IMPL(
         (custom_object_subscription)
   )

   void on_operation( const chain::operation_notification& note );

   std::shared_ptr<chain::database_interface>  _db;
   boost::signals2::connection      post_apply_connection;
   std::vector<custom_content_callback>  _content_subscriptions;
   plugins::json_rpc::json_rpc_plugin* json_api;
   std::shared_ptr< custom::custom_api > custom_api;

};

struct custom_content_callback{
   uint64_t last_position=0;
   uint64_t websocket_handle = 0;
   bool invalid = false;
   subscribe_api_impl* impl;
   custom_object_subscription_args args;

   custom_content_callback(custom_object_subscription_args _args, subscribe_api_impl* _impl, uint64_t _websocket_handle ){
      FC_ASSERT(_args.start > 0);
      args = _args;
      impl = _impl;
      websocket_handle = _websocket_handle;
      last_position = _args.start-1;
   }

   custom_content_callback(custom_content_callback&c){
      args=c.args; impl=c.impl;websocket_handle=c.websocket_handle;last_position=c.last_position;
   }

   custom_content_callback(const custom_content_callback&c){
      args=c.args; impl=c.impl;websocket_handle=c.websocket_handle;last_position=c.last_position;
   }

   void operator ()() {
      custom::list_received_documents_args cb_args;
      cb_args.app_id = args.app_id;
      cb_args.account_name = args.account_name;
      cb_args.search_type = args.search_type;
      cb_args.count = 1;
      cb_args.start = std::to_string(last_position+1);
      auto docs = impl->custom_api->list_received_documents(cb_args);
      while (docs.size()){
         const auto d_itr = docs.find(last_position+1);
         if(d_itr==docs.end())
            return;

         fc::variant v;
         fc::to_variant(*d_itr,v);
         impl->json_api->send_ws_notice(websocket_handle, args.return_id, v);
         last_position++;
         cb_args.start = std::to_string(last_position+1);
         docs = impl->custom_api->list_received_documents(cb_args);
      }
   }
};

void subscribe_api_impl::on_operation( const chain::operation_notification& note ){
   try {
      if( note.op.which() == sophiatx::protocol::operation::tag<sophiatx::protocol::custom_json_operation>::value ||
          note.op.which() == sophiatx::protocol::operation::tag<sophiatx::protocol::custom_binary_operation>::value ) {

         auto itr = _content_subscriptions.begin();
         while(itr!= _content_subscriptions.end() ) {
            try {
               if(!itr->invalid)
                  (*itr)();
            } catch ( fc::send_error_exception& e ) {
               itr->invalid = true;
            }
            itr++;
         }

      }
   } catch (fc::assert_exception&){}
}


DEFINE_API_IMPL( subscribe_api_impl, custom_object_subscription)
{

   FC_ASSERT( custom_api, "custom_api_plugin not enabled." );
   custom_content_callback cb(args, this, _content_subscriptions.size() );

   _content_subscriptions.push_back(cb);

   return _content_subscriptions.size()-1;
}





} // namespace detail

subscribe_api::subscribe_api(subscribe_api_plugin& plugin): my( new detail::subscribe_api_impl(plugin) )
{
   JSON_RPC_REGISTER_API( SOPHIATX_SUBSCRIBE_API_PLUGIN_NAME, plugin.app() );
   plugin.app()->get_plugin< sophiatx::plugins::json_rpc::json_rpc_plugin >().add_api_subscribe_method("subscribe_api", "custom_object_subscription" );
}

void subscribe_api::api_startup(){

}

subscribe_api::~subscribe_api() {}

DEFINE_READ_APIS( subscribe_api,
     (custom_object_subscription)
)

} } } // sophiatx::plugins::subscribe
