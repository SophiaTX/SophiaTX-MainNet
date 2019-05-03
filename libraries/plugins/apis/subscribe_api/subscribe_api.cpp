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
   subscribe_api_impl() : _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() )  {
      post_apply_connection = _db->post_apply_operation.connect( 0, [&]( const chain::operation_notification& note ){ on_operation(note); } );
   }

   DECLARE_API_IMPL(
         (custom_object_subscription)
   )

   void on_operation( const chain::operation_notification& note );

   std::shared_ptr<chain::database_interface>  _db;
   boost::signals2::connection      post_apply_connection;
   std::vector<custom_content_callback>  _content_subscriptions;
   plugins::json_rpc::json_rpc_plugin* _json_api;

};

struct custom_content_callback{
   uint64_t last_position=0;
   std::function<void(fc::variant&)> notify;
   bool invalid = false;
   subscribe_api_impl& impl;
   custom_object_subscription_args args;

   custom_content_callback(const custom_object_subscription_args& _args, subscribe_api_impl& _impl, std::function<void(fc::variant&)>& _notify ):notify(_notify), impl(_impl), args(_args) {
      FC_ASSERT(_args.start > 0);
      last_position = _args.start-1;
   }

   void operator ()() {
      custom::list_received_documents_args cb_args;
      cb_args.app_id = args.app_id;
      cb_args.account_name = args.account_name;
      cb_args.search_type = args.search_type;
      cb_args.count = 1;
      cb_args.start = std::to_string(last_position+1);
      auto vdocs = impl._json_api->call_api_method("custom_api", "list_received_documents", fc::variant(cb_args), [](fc::variant& v, uint64_t i){ FC_UNUSED(v) FC_UNUSED(i)} );
      if(!vdocs)
         return;
      custom::list_received_documents_return docs;
      fc::from_variant( *vdocs, docs );
      while (docs.size()){
         const auto d_itr = docs.find(last_position+1);
         if(d_itr==docs.end())
            return;

         fc::variant v;
         fc::to_variant(*d_itr,v);
         notify(v);
         last_position++;
         cb_args.start = std::to_string(last_position+1);
         vdocs = impl._json_api->call_api_method("custom_api", "list_received_documents", fc::variant(cb_args), [](fc::variant& v, uint64_t i){ FC_UNUSED(v) FC_UNUSED(i)} );
         if(!vdocs)
            break;
         fc::from_variant( *vdocs, docs );
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


DEFINE_API_IMPL( subscribe_api_impl, custom_object_subscription )
{
   std::function<void(fc::variant&)> notify = [ notify_callback, args ](fc::variant& v )->void{ notify_callback(v, args.return_id);};

   custom_content_callback cb(args, *this, notify );

   cb();

   _content_subscriptions.push_back(cb);

   return args.return_id;
}

} // namespace detail

subscribe_api::subscribe_api(): my( new detail::subscribe_api_impl() )
{
   JSON_RPC_REGISTER_API( SOPHIATX_SUBSCRIBE_API_PLUGIN_NAME );
}

void subscribe_api::api_startup(){
}

subscribe_api::~subscribe_api() {}

DEFINE_READ_APIS( subscribe_api,
     (custom_object_subscription)
)

} } } // sophiatx::plugins::subscribe
