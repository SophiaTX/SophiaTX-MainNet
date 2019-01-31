#include <appbase/plugin.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <fstream>

namespace appbase {

template< typename Impl >
void plugin<Impl>::initialize(const variables_map& options)
{
   if( _state == registered )
   {
      _state = initialized;
      this->plugin_for_each_dependency( [&]( abstract_plugin& plug ){ plug.initialize( options ); } );
      this->plugin_initialize( options );
      // std::cout << "Initializing plugin " << Impl::name() << std::endl;
      app()->plugin_initialized( *this );
   }
   if (_state != initialized)
      BOOST_THROW_EXCEPTION( std::runtime_error("Initial state was not registered, so final state cannot be initialized.") );
}

template< typename Impl >
void plugin<Impl>::startup()
{
   if( _state == initialized )
   {
      _state = started;
      this->plugin_for_each_dependency( [&]( abstract_plugin& plug ){ plug.startup(); } );
      this->plugin_startup();
      app()->plugin_started( *this );
   }
   if (_state != started )
      BOOST_THROW_EXCEPTION( std::runtime_error("Initial state was not initialized, so final state cannot be started.") );
}

template< typename Impl >
void plugin<Impl>::shutdown()
{
   if( _state == started )
   {
      _state = stopped;
      //ilog( "shutting down plugin ${name}", ("name",name()) );
      this->plugin_shutdown();
   }
}


}