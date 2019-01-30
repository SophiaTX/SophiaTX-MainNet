#pragma once
#include <appbase/plugin.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/core/demangle.hpp>
#include <boost/asio.hpp>
#include <boost/throw_exception.hpp>

#include <iostream>

#define APPBASE_VERSION_STRING ("appbase 1.1")

namespace appbase {

   namespace bpo = boost::program_options;
   namespace bfs = boost::filesystem;

   class application;

   /**
    * @brief Simple data holder for plugin program options
    */
   class plugin_program_options {
   public:
      plugin_program_options(const options_description& cli_options, const options_description& cfg_options) :
            _cli_options(cli_options),
            _cfg_options(cfg_options)
      {}

      const options_description& get_cli_options() const { return _cli_options; }
      const options_description& get_cfg_options() const { return _cfg_options; }

   private:
      // command line options
      options_description     _cli_options;
      // configuration file options
      options_description     _cfg_options;
   };


   class application : std::enable_shared_from_this<application>
   {
      public:
         ~application();
         application(const string& _id);

         /**
          * @brief Looks for the --plugin commandline / config option and calls initialize on those plugins
          *
          * @tparam Plugin List of plugins to initalize even if not mentioned by configuration. For plugins started by
          * configuration settings or dependency resolution, this template has no effect.
          * @return true if the application and plugins were initialized, false or exception on error
          */
         bool initialize( const variables_map& app_settings, const vector<string>& autostart_plugins );

         void startup();
         void shutdown();

         /**
          *  Wait until quit(), SIGINT or SIGTERM and then shutdown
          */
         void exec();
         void quit();

         void register_plugin( const std::shared_ptr<abstract_plugin>& plugin ){
            auto existing = find_plugin( plugin->get_name() );
            if(existing)
               return;

            plugins[plugin->get_name()] = plugin;
            plugin->register_dependencies();
         }

         /**
          * @brief Loads external plugins specified in config
          *
          * @return
          */
         bool load_external_plugins();

         /**
          * @brief Registers external plugin into the list of all app plugins
          *
          * @param plugin
          */
         void register_external_plugin(const std::shared_ptr<abstract_plugin>& plugin);

         /**
          * @brief Loads external plugin config
          *
          * @param plugin
          */
         void load_external_plugin_config(const std::function<void(options_description&,options_description&)> options_setter, const std::string& cfg_plugin_name);

         template< typename Plugin >
         Plugin* find_plugin()const
         {
            Plugin* plugin = dynamic_cast< Plugin* >( find_plugin( Plugin::name() ) );

            return plugin;
         }

         template< typename Plugin >
         Plugin& get_plugin()const
         {
            auto ptr = find_plugin< Plugin >();
            if( ptr == nullptr )
               BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find plugin: " + Plugin::name() ) );
            return *ptr;
         }

         template< typename Plugin >
         Plugin& get_or_create_plugin()
         {
            Plugin* plugin = dynamic_cast< Plugin* >( find_plugin( Plugin::name() ) );
            if( plugin != nullptr )
               return *plugin;
            auto new_plg = std::make_shared<Plugin>();
            new_plg->set_app( shared_from_this() );
            plugins[ Plugin::name() ] = new_plg;
            return *new_plg;
         }

         void reset(){
            plugins.clear();
         }

         bfs::path data_dir()const;

         const bpo::variables_map& get_args() const;
         string id;

         boost::asio::io_service& get_io_service() { return *io_serv; }

      protected:
         template< typename Impl >
         friend class plugin;

         abstract_plugin* find_plugin( const string& name )const;
         abstract_plugin& get_plugin( const string& name )const;

         /** these notifications get called from the plugin when their state changes so that
          * the application can call shutdown in the reverse order.
          */
         ///@{
         void plugin_initialized( abstract_plugin& plug ) { initialized_plugins.push_back( &plug ); }
         void plugin_started( abstract_plugin& plug ) { running_plugins.push_back( &plug ); }
         ///@}

      private:

         map< string, std::shared_ptr< abstract_plugin > >  plugins; ///< all registered plugins
         vector< abstract_plugin* >                         initialized_plugins; ///< stored in the order they were started running
         vector< abstract_plugin* >                         running_plugins; ///< stored in the order they were started running
         std::shared_ptr< boost::asio::io_service >         io_serv;

         std::unique_ptr< class application_impl > my;

   };

   class application_factory
   {
   public:
      static application_factory &get_app_factory() {
         static application_factory instance;
         return instance;
      }

      application_factory(application_factory const &) = delete;
      void operator=(application_factory const &) = delete;

      template<typename P> void register_plugin_factory(){
         if( plugin_factories.find(P::name()) != plugin_factories.end() )
            return;
         std::shared_ptr<plugin_factory<P>> new_plugin_factory = std::make_shared<plugin_factory<P>>();
         plugin_factories[P::name()] = new_plugin_factory;
         const plugin_program_options& plugin_options = get_plugin_program_options(new_plugin_factory);
         const options_description& plugin_cfg_options = plugin_options.get_cfg_options();
         if (plugin_cfg_options.options().size()) {
            app_options.add(plugin_cfg_options);
         }

         const options_description& plugin_cli_options = plugin_options.get_cli_options();
         if (plugin_cli_options.options().size()) {
            app_options.add(plugin_cli_options);
         }
      }

      std::shared_ptr<application> new_application( const string& id){
         std::shared_ptr<application> new_app = std::make_shared<application>(id );
         apps[id] = new_app;
         for( const auto& pf : plugin_factories){
            auto new_plugin = pf.second->new_plugin(new_app);
            new_app->register_plugin(new_plugin);
         }
         return new_app;
      }

      void add_program_options( const bpo::options_description& cli );
      variables_map read_app_config(string name);

      map<string, std::shared_ptr<application>> initialize( int argc, char** argv, vector< string > autostart_plugins, bool start_apps = true );
      void set_version_string( const string& version ) { version_info = version; }

      options_description    app_options;
      options_description    global_options;
      bfs::path              data_dir;
      bfs::path              plugins_dir;
      variables_map          global_args;

      void startup(){
         for(auto& app : apps)
            app.second->startup();
      };

      void exec(){
         for(auto& app : apps)
            app.second->exec();
      };
      void shutdown(){
         for(auto& app : apps)
            app.second->shutdown();
      }
      void quit(){
         for(auto& app : apps)
            app.second->quit();
      }

   private:
      void set_program_options();
      std::map<string, std::shared_ptr<abstract_plugin_factory>> plugin_factories;
      vector<string>                               autostart_plugins;
      std::map<string, std::shared_ptr<application>> apps;
      string                                       version_info;

      application_factory():global_options("Application Options") {
         set_program_options();
      };


      plugin_program_options get_plugin_program_options(std::shared_ptr<abstract_plugin_factory>  plugin_factory);
   };

   application_factory& app_factory();
}
