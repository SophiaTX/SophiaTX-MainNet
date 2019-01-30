#pragma once
#include <boost/program_options.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <functional>
#include <string>
#include <vector>
#include <map>

#define APPBASE_PLUGIN_REQUIRES_VISIT( r, visitor, elem ) \
  visitor( ( app()->get_or_create_plugin<elem>() ) );

#define APPBASE_PLUGIN_REQUIRES( PLUGINS )                               \
   virtual void plugin_for_each_dependency( plugin_processor&& l ) override {  \
      BOOST_PP_SEQ_FOR_EACH( APPBASE_PLUGIN_REQUIRES_VISIT, l, PLUGINS ) \
   }

namespace appbase {

using boost::program_options::options_description;
using boost::program_options::variables_map;
using std::string;
using std::vector;
using std::map;

class application;

class abstract_plugin : std::enable_shared_from_this<abstract_plugin>
{
public:
   enum state {
      registered, ///< the plugin is constructed but doesn't do anything
      initialized, ///< the plugin has initlaized any state required but is idle
      started, ///< the plugin is actively running
      stopped ///< the plugin is no longer running
   };

   virtual ~abstract_plugin(){}

   virtual state get_state()const = 0;
   virtual const std::string& get_name()const  = 0;
   static void set_program_options( options_description& cli, options_description& cfg ){};
   virtual void register_dependencies() = 0;
   virtual void initialize(const variables_map& options) = 0;
   virtual void startup() = 0;
   virtual void shutdown() = 0;
   void set_app(std::shared_ptr<application> my_app) { _app = my_app; }
   std::shared_ptr<application> app() { return _app; }

protected:
   typedef std::function<void(abstract_plugin&)> plugin_processor;

   /** Abstract method to be reimplemented in final plugin implementation.
       It is a part of initialization/startup process triggerred by main application.
       Allows to process all plugins, this one depends on.
   */
   virtual void plugin_for_each_dependency(plugin_processor&& processor) = 0;

   /** Abstract method to be reimplemented in final plugin implementation.
       It is a part of initialization process triggerred by main application.
   */
   virtual void plugin_initialize( const variables_map& options ) = 0;
   /** Abstract method to be reimplemented in final plugin implementation.
       It is a part of startup process triggerred by main application.
   */
   virtual void plugin_startup() = 0;
   /** Abstract method to be reimplemented in final plugin implementation.
       It is a part of shutdown process triggerred by main application.
   */
   virtual void plugin_shutdown() = 0;

private:
   std::shared_ptr<application> _app;
};

template< typename Impl >
class plugin : public abstract_plugin
{
public:
   virtual ~plugin() {}

   virtual state get_state() const override { return _state; }
   virtual const std::string& get_name()const override final { return Impl::name(); }

   virtual void register_dependencies() override
   {
      //this->plugin_for_each_dependency( [&]( abstract_plugin& plug ){} );
   }

   virtual void initialize(const variables_map& options) override final
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

   virtual void startup() override final
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

   virtual void shutdown() override final
   {
      if( _state == started )
      {
         _state = stopped;
         //ilog( "shutting down plugin ${name}", ("name",name()) );
         this->plugin_shutdown();
      }
   }

protected:
   plugin() = default;

private:
   state _state = abstract_plugin::registered;
};

class abstract_plugin_factory
{
public:
   virtual ~abstract_plugin_factory(){}
   virtual std::shared_ptr<abstract_plugin> new_plugin( std::shared_ptr<application> new_app ) const = 0;
   virtual void set_program_options( options_description& cli, options_description& cfg ) = 0;
   virtual std::string get_name() = 0;
};


template <typename Plugin>
class plugin_factory : public abstract_plugin_factory
{
public:
   virtual ~plugin_factory(){}
   virtual std::shared_ptr<abstract_plugin> new_plugin( std::shared_ptr<application> new_app ) const final {
      std::shared_ptr<abstract_plugin> new_plg = std::make_shared<Plugin>();
      new_plg->set_app( new_app );
      return new_plg;
   }

   virtual void set_program_options( options_description& cli, options_description& cfg ) {
      Plugin::set_program_options(cli, cfg);
   };

   virtual std::string get_name(){
      return Plugin::name();
   }

};

template<typename Impl> class plugin;
};

