#include <sophiatx/plugins/template/template_plugin.hpp>
#include <sophiatx/plugins/template/template_objects.hpp>
#include <sophiatx/plugins/template/template_api.hpp>

#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/operation_notification.hpp>

namespace sophiatx { namespace plugins { namespace template_plugin {

namespace detail {

class template_plugin_impl
{
   public:
     template_plugin_impl( template_plugin& _plugin ) :
         _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void pre_operation( const operation_notification& op_obj );
      void post_operation( const operation_notification& op_obj );

      std::shared_ptr<database_interface>    _db;
      template_plugin&              _self;
      boost::signals2::connection   pre_apply_connection;
      boost::signals2::connection   post_apply_connection;
};

struct pre_operation_visitor
{
   template_plugin_impl& _plugin;

   pre_operation_visitor( template_plugin_impl& plugin ) : _plugin( plugin ) {}

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
   template_plugin_impl& _plugin;

   post_operation_visitor( template_plugin_impl& plugin ) : _plugin( plugin ) {}

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

void template_plugin_impl::pre_operation( const operation_notification& note )
{
   note.op.visit( pre_operation_visitor( *this ) );
}

void template_plugin_impl::post_operation( const operation_notification& note )
{
   note.op.visit( post_operation_visitor( *this ) );
}

} // detail

template_plugin::template_plugin() {}
template_plugin::~template_plugin() {}

void template_plugin::set_program_options( options_description& cli, options_description& cfg ){}

void template_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::template_plugin_impl >( *this );
   api = std::make_shared< template_api >();
   try
   {
      ilog( "Initializing template_plugin_impl plugin" );
      auto& db = appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db();

      my->pre_apply_connection = db->pre_apply_operation.connect( 0, [&]( const operation_notification& o ){ my->pre_operation( o ); } );
      my->post_apply_connection = db->post_apply_operation.connect( 0, [&]( const operation_notification& o ){ my->post_operation( o ); } );

      add_plugin_index< template_lookup_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void template_plugin::plugin_startup() {}

void template_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->pre_apply_connection );
   chain::util::disconnect_signal( my->post_apply_connection );
}

} } } // sophiatx::plugins::template_plugin
