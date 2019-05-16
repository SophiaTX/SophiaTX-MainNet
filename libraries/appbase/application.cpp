#include <appbase/application.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <fstream>

#include <boost/dll/import.hpp>
#include <boost/exception/diagnostic_information.hpp>


namespace appbase {

namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;
using std::cout;


class application_impl {
   public:
      application_impl():_app_options("Application Options"){
      }
      const variables_map*    _options = nullptr;
      options_description     _app_options;
      options_description     _cfg_options;
      variables_map           _args;

      bfs::path               _data_dir;
};

application::application()
:my(new application_impl()){
   io_serv = std::make_shared<boost::asio::io_service>();
}

application::~application() { }

void application::startup() {
   for (const auto& plugin : initialized_plugins)
      plugin->startup();
}

application& application::instance( bool reset ) {
   static application* _app = new application();
   if( reset )
   {
      delete _app;
      _app = new application();
   }
   return *_app;
}
application& app() { return application::instance(); }
application& reset() { return application::instance( true ); }


void application::set_program_options()
{
   options_description app_cfg_opts( "Application Config Options" );
   options_description app_cli_opts( "Application Command Line Options" );
   app_cfg_opts.add_options()
         ("plugin", bpo::value< vector<string> >()->composing(), "Plugin(s) to enable, may be specified multiple times");
   app_cfg_opts.add_options()
         ("backtrace", bpo::value< string >()->default_value( "yes" ), "Whether to print backtrace on SIGSEGV" );
   app_cfg_opts.add_options()
         ("log-level", bpo::value< string >()->default_value( "info" ), "Log level. Possible values: debug, info, notice, warning, error, critical, alert, emergency. For monitoring to work, min. level is notice !" );


   app_cfg_opts.add_options()
         ("external-plugins-dir", bpo::value<bfs::path>()->default_value( "external-plugins" ), "Directory containing external/runtime-loadable plugins binaries (absolute path or relative to the program option data-dir/)");
   app_cfg_opts.add_options()
         ("external-plugin", bpo::value< vector<string> >()->composing(), "External plugin(s) to enable, may be specified multiple times");

   app_cli_opts.add_options()
         ("help,h", "Print this help message and exit.")
         ("version,v", "Print version information.")
         ("data-dir,d", bpo::value<bfs::path>()->default_value( "sophia_app_data" ), "Directory containing configuration files, blockchain data and external plugins")
         ("config,c", bpo::value<bfs::path>()->default_value( "config.ini" ), "Main configuration file path (absolute path or relative to the data-dir/configs/)");

   my->_cfg_options.add(app_cfg_opts);
   my->_app_options.add(app_cfg_opts);
   my->_app_options.add(app_cli_opts);

   for(auto& plug : plugins) {
      const plugin_program_options& plugin_options = get_plugin_program_options(plug.second);

      const options_description& plugin_cfg_options = plugin_options.get_cfg_options();
      if (plugin_cfg_options.options().size()) {
         my->_app_options.add(plugin_cfg_options);
         my->_cfg_options.add(plugin_cfg_options);
      }

      const options_description& plugin_cli_options = plugin_options.get_cli_options();
      if (plugin_cli_options.options().size()) {
         my->_app_options.add(plugin_cli_options);
      }
   }
}

plugin_program_options application::get_plugin_program_options(const std::shared_ptr< abstract_plugin >& plugin) {
   options_description plugin_cli_opts("Command Line Options for " + plugin->get_name());
   options_description plugin_cfg_opts("Config Options for " + plugin->get_name());
   plugin->set_program_options(plugin_cli_opts, plugin_cfg_opts);

   return plugin_program_options(plugin_cli_opts, plugin_cfg_opts);
}


bool application::load_external_plugins() {
   // plugins-dir par (even if it is default) must be always present
   assert(my->_args.count( "external-plugins-dir" ));

   boost::program_options::variable_value cfg_plugins_dir = my->_args["external-plugins-dir"];
   bfs::path plugins_dir = cfg_plugins_dir.as<bfs::path>();
   if (plugins_dir.is_relative() == true) {
      plugins_dir = my->_data_dir / plugins_dir;

      if (bfs::exists(plugins_dir) == false) {
         bfs::create_directory(plugins_dir);
      }
   }

   if(my->_args.count("external-plugin") > 0)
   {
      auto plugins = my->_args.at("external-plugin").as<std::vector<std::string>>();
      for(auto& arg : plugins)
      {
         vector<string> plugins_names;
         boost::split(plugins_names, arg, boost::is_any_of(" \t,"));

         for(const std::string& plugin_name : plugins_names) {
            bfs::path plugin_binary_path = plugins_dir / std::string("lib" + plugin_name + "_plugin.so");
            if (bfs::exists(plugin_binary_path) == false) {
               std::cerr << "Missing binary: " << plugin_binary_path << " for plugin: \"" << plugin_name << "\"" << std::endl;
               return false;
            }

            std::shared_ptr<abstract_plugin> loaded_plugin;
            try {
               auto plugin_creator = boost::dll::import<std::shared_ptr<abstract_plugin>()>(plugin_binary_path, "get_plugin", boost::dll::load_mode::append_decorations);
               loaded_plugin = plugin_creator();
            }
            catch ( const boost::exception& e )
            {
               std::cerr << "Failed to load plugin. Error: " << boost::diagnostic_information(e) << std::endl;
               return false;
            }

            // Registers loaded plugin for application
            register_external_plugin(loaded_plugin);

            // Loads plugins config parameters
            load_external_plugin_config(loaded_plugin, plugin_name);

            // Initializes plugin
            loaded_plugin->initialize(my->_args);
         }
      }
   }

   return true;
}

void application::register_external_plugin(const std::shared_ptr<abstract_plugin>& plugin) {
   plugins[plugin->get_name()] = plugin;
   plugin->register_dependencies();
}

void application::load_external_plugin_config(const std::shared_ptr<abstract_plugin>& plugin, const std::string& cfg_plugin_name) {
   const plugin_program_options& plugin_options = get_plugin_program_options(plugin);

   // Writes config if it does not already exists
   bfs::path config_file_path = write_default_config(plugin_options.get_cfg_options(),  bfs::path(cfg_plugin_name + "_plugin_config.ini"));

   // Copies parameters from config_file into my->_args
   bpo::store(bpo::parse_config_file< char >( config_file_path.make_preferred().string().c_str(),
                                              plugin_options.get_cfg_options(), true ), my->_args );
}

// TODO: delete after HF2
/**
 * @brief Fix which renames default data directory and also moves config into subdirectory configs, so users do not need to do it themselves
 *        Applies fixes only when users did not specify some custom paths to data-dir and config file
 *
 * @param actual_data_dir
 * @param cfg_data_dir
 * @param cfg_config
 */
void fix_deprecated_data_folder_structure(const bfs::path& actual_data_dir,
                                          const boost::program_options::variable_value& cfg_data_dir,
                                          const boost::program_options::variable_value& cfg_config) {
   // If data_dir has default(new) name and there is existing directory with old name -> rename it
   if (cfg_data_dir.defaulted() == true) {
      bfs::path deprecated_default_data_dir = bfs::current_path() / "witness_node_data_dir";
      if (bfs::exists(deprecated_default_data_dir) == true) {
         bfs::rename(deprecated_default_data_dir, actual_data_dir);
      }
   }

   // If config with default name exists and is not in subdirectory configs -> move it there
   if (cfg_config.defaulted() == true) {
      bfs::path deprecated_default_config_path = actual_data_dir / "config.ini";
      if (bfs::exists(deprecated_default_config_path) == true) {
         if (bfs::exists(actual_data_dir) == false) {
            bfs::create_directory(actual_data_dir);
         }

         bfs::path configs_dir = actual_data_dir / "configs/";
         if (bfs::exists(configs_dir) == false) {
            bfs::create_directory(configs_dir);
         }

         bfs::rename(deprecated_default_config_path, configs_dir / cfg_config.as<bfs::path>());
      }
   }
}


bool application::load_config( int argc, char** argv ) {
   try
   {
      set_program_options();
      bpo::store( bpo::parse_command_line( argc, argv, my->_app_options ), my->_args );

      if( my->_args.count( "help" ) ) {
         cout << my->_app_options << "\n";
         return false;
      }

      if( my->_args.count( "version" ) )
      {
         cout << version_info << "\n";
         return false;
      }


      // data-dir par (even if it is default) must be always present
      assert(my->_args.count( "data-dir" ));

      my->_data_dir = my->_args["data-dir"].as<bfs::path>();
      if( my->_data_dir.is_relative() )
         my->_data_dir = bfs::current_path() / my->_data_dir;


      // config par (even if it is default) must be always present
      assert(my->_args.count( "config" ));


      // TODO: delete after HF2
      // Fix which renames default data directory and also moves config into subdirectory configs, so users do not need to do it themselves
      fix_deprecated_data_folder_structure(my->_data_dir, my->_args["data-dir"], my->_args["config"]);


      // Writes config if it does not already exists
      bfs::path config_file_path = write_default_config(my->_cfg_options, my->_args["config"].as<bfs::path>());

      // Copies parameters from config_file into my->_args
      bpo::store(bpo::parse_config_file< char >( config_file_path.make_preferred().string().c_str(),
                                                 my->_cfg_options, true ), my->_args );

      return true;
   }
   catch (const boost::program_options::error& e)
   {
      std::cerr << "Error parsing command line: " << e.what() << "\n";
      return false;
   }
}

bool application::initialize_impl(int argc, char** argv, vector<abstract_plugin*> autostart_plugins)
{
   try
   {
      if(my->_args.count("plugin") > 0)
      {
         auto plugins = my->_args.at("plugin").as<std::vector<std::string>>();
         for(auto& arg : plugins)
         {
            vector<string> names;
            boost::split(names, arg, boost::is_any_of(" \t,"));
            for(const std::string& name : names) {
               get_plugin(name).initialize(my->_args);
            }
         }
      }

      // Loads external plugins specified in config
      if (load_external_plugins() == false) {
         return false;
      }

      for (const auto& plugin : autostart_plugins) {
         if (plugin != nullptr && plugin->get_state() == abstract_plugin::registered)
            plugin->initialize(my->_args);
      }

      bpo::notify(my->_args);

      return true;
   }
   catch (const boost::program_options::error& e)
   {
      std::cerr << "Error parsing command line: " << e.what() << "\n";
      return false;
   }
}

void application::shutdown() {
   for(auto ritr = running_plugins.rbegin();
       ritr != running_plugins.rend(); ++ritr) {
      (*ritr)->shutdown();
   }
   for(auto ritr = running_plugins.rbegin();
       ritr != running_plugins.rend(); ++ritr) {
      plugins.erase((*ritr)->get_name());
   }
   running_plugins.clear();
   initialized_plugins.clear();
   plugins.clear();
}

void application::quit() {
   io_serv->stop();
}

void application::exec() {
   std::shared_ptr<boost::asio::signal_set> sigint_set(new boost::asio::signal_set(*io_serv, SIGINT));
   sigint_set->async_wait([sigint_set,this](const boost::system::error_code& err, int num) {
     quit();
     sigint_set->cancel();
   });

   std::shared_ptr<boost::asio::signal_set> sigterm_set(new boost::asio::signal_set(*io_serv, SIGTERM));
   sigterm_set->async_wait([sigterm_set,this](const boost::system::error_code& err, int num) {
     quit();
     sigterm_set->cancel();
   });

   io_serv->run();

   shutdown(); /// perform synchronous shutdown
}

bfs::path application::write_default_config( const options_description& options_desc, const bfs::path& cfg_file ) {
   bfs::path config_file_name = my->_data_dir / "configs" / cfg_file;
   if( cfg_file.is_absolute() == true ) {
      config_file_name = cfg_file;
   }

   if(bfs::exists(config_file_name) == true) {
      return config_file_name;
   }

   if(bfs::exists(config_file_name.parent_path()) == false) {
      bfs::create_directories(config_file_name.parent_path());
   }

   std::ofstream out_cfg( bfs::path(config_file_name).make_preferred().string());
   for(const boost::shared_ptr<bpo::option_description> od : options_desc.options())
   {
      if(!od->description().empty())
         out_cfg << "# " << od->description() << "\n";
      boost::any store;
      if(!od->semantic()->apply_default(store))
         out_cfg << "# " << od->long_name() << " = \n";
      else {
         auto example = od->format_parameter();
         if(example.empty())
            // This is a boolean switch
            out_cfg << od->long_name() << " = " << "false\n";
         else {
            // The string is formatted "arg (=<interesting part>)"
            example.erase(0, 6);
            example.erase(example.length()-1);
            out_cfg << od->long_name() << " = " << example << "\n";
         }
      }
      out_cfg << "\n";
   }
   out_cfg.close();

   return config_file_name;
}

abstract_plugin* application::find_plugin( const string& name )const
{
   auto itr = plugins.find( name );

   if( itr == plugins.end() )
   {
      return nullptr;
   }

   return itr->second.get();
}

abstract_plugin& application::get_plugin(const string& name)const {
   auto ptr = find_plugin(name);
   if(!ptr)
      BOOST_THROW_EXCEPTION(std::runtime_error("unable to find plugin: " + name));
   return *ptr;
}

bfs::path application::data_dir()const
{
   return my->_data_dir;
}

void application::add_program_options( const options_description& cli, const options_description& cfg )
{
   my->_app_options.add( cli );
   my->_app_options.add( cfg );
   my->_cfg_options.add( cfg );
}

const variables_map& application::get_args() const
{
   return my->_args;
}

} /// namespace appbase
