#include <sophiatx/plugins/track_and_trace/track_and_trace_plugin.hpp>
#include <sophiatx/plugins/track_and_trace/track_and_trace_objects.hpp>
#include <sophiatx/plugins/track_and_trace/track_and_trace_api.hpp>

#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/custom_operation_interpreter.hpp>
#include <sophiatx/chain/operation_notification.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

namespace detail {
class tat_interpreter : public custom_operation_interpreter
{
public:
   tat_interpreter(const std::shared_ptr<database_interface>& db):_db(db){};
   virtual ~tat_interpreter() = default;
   std::shared_ptr<database_interface> _db;
   virtual void apply( const protocol::custom_json_operation& op );
   virtual void apply( const protocol::custom_binary_operation & op ){ FC_ASSERT(false, "Track and trace handles only json operations");};
   //virtual std::shared_ptr< sophiatx::schema::abstract_schema > get_operation_schema();
};

class track_and_trace_plugin_impl
{
   public:
     track_and_trace_plugin_impl( track_and_trace_plugin& _plugin ) :
         _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ),
         app_id(_plugin.app_id) {
        interpreter = std::make_shared<tat_interpreter>(_db);
     }

      std::shared_ptr<database_interface>  _db;
      track_and_trace_plugin&              _self;
      uint64_t                      app_id;
      std::shared_ptr< tat_interpreter > interpreter;
};



void tat_interpreter::apply( const protocol::custom_json_operation& op ) {
   account_name_type sender = op.sender;
   FC_ASSERT(op.recipients.size());
   account_name_type serial = *op.recipients.begin();
   FC_ASSERT(op.json.size());
   variant tmp = fc::json::from_string(&op.json[ 0 ]);
   FC_ASSERT( (std::string)serial == tmp[ "serial" ].as<string>() );

   if( tmp[ "action" ].as<string>() == std::string("register")) {
      _db->create<possession_object>([&](possession_object& po){
         po.holder="";
         po.new_holder = "";
         po.serial = serial;
         from_string(po.meta, tmp["meta"].as<string>());
         from_string(po.claim_key, tmp["claimKey"].as<string>());
      });//*/
   } else {
      //read the db entry here
      const auto& holder_idx = _db->get_index< posession_index >().indices().get< by_serial >();
      const auto& holder_itr = holder_idx.find(serial);
      FC_ASSERT(holder_itr != holder_idx.end(), "Item with given number not found");
      if( tmp[ "action" ].as<string>() == std::string("claim")) {
         FC_ASSERT(tmp["claimKey"].as<string>() == to_string(holder_itr->claim_key), "incorrect claim key");
         _db->modify(*holder_itr,[&](possession_object& po){
              po.holder = sender;
              from_string(po.info, tmp["info"].as<string>());
         });
      } else if( tmp[ "action" ].as<string>() == std::string("updateInfo")) {
         FC_ASSERT(sender == holder_itr->holder);
         _db->modify(*holder_itr,[&](possession_object& po){
              from_string(po.info, tmp["info"].as<string>());
         });
      } else if( tmp[ "action" ].as<string>() == std::string("handoverRequest")) {
         FC_ASSERT(sender == holder_itr->holder);
         _db->modify(*holder_itr,[&](possession_object& po){
              po.new_holder = tmp["newOwner"].as<string>();
         });
      } else if( tmp[ "action" ].as<string>() == std::string("handoverAck")) {
         FC_ASSERT(sender == holder_itr->new_holder);
         _db->modify(*holder_itr,[&](possession_object& po){
              po.new_holder = "";
              po.holder = sender;
         });
         _db->create<transfer_history_object>([&](transfer_history_object& o){
              o.new_holder = sender;
              o.serial = serial;
              o.change_date = fc::time_point::now();
         });
      } else if( tmp[ "action" ].as<string>() == std::string("handoverReject")) {
         FC_ASSERT(sender == holder_itr->new_holder);
         _db->modify(*holder_itr,[&](possession_object& po){
              po.new_holder = "";
         });

      }
   }
}


} // detail

track_and_trace_plugin::track_and_trace_plugin() {}
track_and_trace_plugin::~track_and_trace_plugin() {}

void track_and_trace_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()("track-and-trace-app-id", boost::program_options::value< long long >()->default_value( 100 ), "App id used by the track and trace" );
}

void track_and_trace_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   if( options.count( "track-and-trace-app-id" ) ){
      app_id = options[ "track-and-trace-app-id" ].as< long long >();
   }else{
      ilog("App ID not given, track and trace is disabled");
      return;
   }
   my = std::make_unique< detail::track_and_trace_plugin_impl >( *this );
   api = std::make_shared< track_and_trace_api >();

   try
   {
      ilog( "Initializing track_and_trace_plugin_impl plugin" );
      auto& db = appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db();

      db->set_custom_operation_interpreter(app_id, dynamic_pointer_cast<custom_operation_interpreter, detail::tat_interpreter>(my->interpreter));
      add_plugin_index< posession_index >(db);
      add_plugin_index< transfer_history_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void track_and_trace_plugin::plugin_startup() {}
void track_and_trace_plugin::plugin_shutdown() {}

} } } // sophiatx::plugins::track_and_trace_plugin
