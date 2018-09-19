#include <sophiatx/plugins/track_and_trace/track_and_trace_plugin.hpp>
#include <sophiatx/plugins/track_and_trace/track_and_trace_objects.hpp>
#include <sophiatx/plugins/track_and_trace/track_and_trace_api.hpp>

#include <sophiatx/chain/database.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/operation_notification.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

namespace detail {

class track_and_trace_plugin_impl
{
   public:
     track_and_trace_plugin_impl( track_and_trace_plugin& _plugin ) :
         _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void pre_operation( const operation_notification& op_obj );
      void post_operation( const operation_notification& op_obj );

      database&                     _db;
      track_and_trace_plugin&              _self;
      boost::signals2::connection   pre_apply_connection;
      boost::signals2::connection   post_apply_connection;
};

struct pre_operation_visitor
{
   track_and_trace_plugin_impl& _plugin;

   pre_operation_visitor( track_and_trace_plugin_impl& plugin ) : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const custom_json_operation& op )const
   {
      //do something here
   }

   void operator()( const custom_binary_operation& op )const
   {
      //do something here
   }

};


struct post_operation_visitor
{
   track_and_trace_plugin_impl& _plugin;

   post_operation_visitor( track_and_trace_plugin_impl& plugin ) : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const custom_json_operation& op )const
   {

   }

   void operator()( const custom_binary_operation& op )const
   {

   }

};

void track_and_trace_plugin_impl::pre_operation( const operation_notification& note )
{
   note.op.visit( pre_operation_visitor( *this ) );
}

void track_and_trace_plugin_impl::post_operation( const operation_notification& note )
{
   note.op.visit( post_operation_visitor( *this ) );
}

} // detail

track_and_trace_plugin::track_and_trace_plugin() {}
track_and_trace_plugin::~track_and_trace_plugin() {}

void track_and_trace_plugin::set_program_options( options_description& cli, options_description& cfg ){}

void track_and_trace_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::track_and_trace_plugin_impl >( *this );
   api = std::make_shared< track_and_trace_api >();
   try
   {
      ilog( "Initializing track_and_trace_plugin_impl plugin" );
      chain::database& db = appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db();

      my->pre_apply_connection = db.pre_apply_operation.connect( 0, [&]( const operation_notification& o ){ my->pre_operation( o ); } );
      my->post_apply_connection = db.post_apply_operation.connect( 0, [&]( const operation_notification& o ){ my->post_operation( o ); } );

      add_plugin_index< posession_index >(db);
      add_plugin_index< transfer_history_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void track_and_trace_plugin::plugin_startup() {}

void track_and_trace_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->pre_apply_connection );
   chain::util::disconnect_signal( my->post_apply_connection );
}

} } } // sophiatx::plugins::track_and_trace_plugin
