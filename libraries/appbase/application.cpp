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

namespace{
// TODO: delete after HF2
/**
 * @brief Fix which renames default data directory and also moves config into subdirectory configs, so users do not need to do it themselves
 *        Applies fixes only when users did not specify some custom paths to data-dir and config file
 *
 * @param actual_data_dir
 * @param cfg_data_dir
 * @param cfg_config
 */
/*void fix_deprecated_data_folder_structure(const bfs::path& actual_data_dir,
                                          const boost::program_options::variable_value& cfg_data_dir,
                                          const boost::program_options::variable_value& cfg_config)
{
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
}*/

/**
          * @brief Writes options_desc data into cfg_file file
          *
          * @param options_desc
          * @param cfg_file
          * @return created config file absolute path
          */
bfs::path write_default_config( const options_description& options_desc, const bfs::path& cfg_file, std::string suffix = "" ) {
   bfs::path _cfg_file = cfg_file;
   if( suffix != "" ){
      _cfg_file += std::string(".") + suffix;
   }

   bfs::path config_file_name = app_factory().data_dir / "configs" / _cfg_file ;

   if( _cfg_file.is_absolute() == true ) {
      config_file_name = _cfg_file;
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

const static string _public_net_chain_id = "1a058d1a89aff240ab203abe8a429d1a1699c339032a87e70e01022842a98324";

}


class application_impl {
   public:
      application_impl(){
      }
      const variables_map*    _options = nullptr;
      options_description&    _cfg_options = app_factory().app_options;
      variables_map           _args;

      bfs::path               _data_dir;
};

application::application(const string& _id)
:id(_id), my(new application_impl()) {
   io_serv = std::make_shared<boost::asio::io_service>();
}

application::~application() { }

void application::startup() {
   for (const auto& plugin : initialized_plugins)
      plugin->startup();
}

plugin_program_options application_factory::get_plugin_program_options(std::shared_ptr<abstract_plugin_factory> plugin_factory) {
   options_description plugin_cli_opts("Command Line Options for " + plugin_factory->get_name());
   options_description plugin_cfg_opts("Config Options for " + plugin_factory->get_name());
   plugin_factory->set_program_options(plugin_cli_opts, plugin_cfg_opts);

   return plugin_program_options(plugin_cli_opts, plugin_cfg_opts);
}


bool application::load_external_plugins() {
   // plugins-dir par (even if it is default) must be always present


   if(my->_args.count("external-plugin") > 0)
   {
      auto plugins = my->_args.at("external-plugin").as<std::vector<std::string>>();
      for(auto& arg : plugins)
      {
         vector<string> plugins_names;
         boost::split(plugins_names, arg, boost::is_any_of(" \t,"));

         for(const std::string& plugin_name : plugins_names) {
            bfs::path plugin_binary_path = app_factory().plugins_dir / std::string("lib" + plugin_name + "_plugin.so");
            if (bfs::exists(plugin_binary_path) == false) {
               std::cerr << "Missing binary: " << plugin_binary_path << " for plugin: \"" << plugin_name << "\"" << std::endl;
               return false;
            }

            std::shared_ptr<abstract_plugin> loaded_plugin;
            std::function<void(options_description&,options_description&)> options_setter;
            try {
               auto plugin_factory = boost::dll::import<std::shared_ptr<abstract_plugin>()>(plugin_binary_path, "plugin_factory", boost::dll::load_mode::append_decorations);
               options_setter = boost::dll::import<void(options_description&,options_description&)>(plugin_binary_path, "options_setter", boost::dll::load_mode::append_decorations);
               loaded_plugin = plugin_factory();
            }
            catch ( const boost::exception& e )
            {
               std::cerr << "Failed to load plugin. Error: " << boost::diagnostic_information(e) << std::endl;
               return false;
            }

            loaded_plugin->set_app( shared_from_this() );
            // Registers loaded plugin for application
            register_external_plugin(loaded_plugin);

            // Loads plugins config parameters
            load_external_plugin_config(options_setter, plugin_name);

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

void application::load_external_plugin_config(const std::function<void(options_description&,options_description&)> options_setter, const std::string& cfg_plugin_name) {
   options_description plugin_cli_opts("Command Line Options for " + cfg_plugin_name);
   options_description plugin_cfg_opts("Config Options for " + cfg_plugin_name);
   options_setter(plugin_cli_opts, plugin_cfg_opts);

   // Writes config if it does not already exists
   bfs::path config_file_path = write_default_config(plugin_cfg_opts,  bfs::path(cfg_plugin_name + "_plugin_config.ini"), id);

   // Copies parameters from config_file into my->_args
   bpo::store(bpo::parse_config_file< char >( config_file_path.make_preferred().string().c_str(),
                                              plugin_cfg_opts, true ), my->_args );
}


bool application::initialize( const variables_map& app_settings, const vector<string>& autostart_plugins )
{
   try
   {
      my->_args = app_settings;

      if(my->_args.count("plugin") > 0)
      {
         auto plugins = my->_args.at("plugin").as<std::vector<std::string>>();
         for(auto& arg : plugins)
         {
            vector<string> names;
            boost::split(names, arg, boost::is_any_of(" \t,"));
            for(const std::string& name : names)
               get_plugin(name).initialize(my->_args);
         }
      }

      // Loads external plugins specified in config
      if (load_external_plugins() == false) {
         return false;
      }

      for (const auto& plugin_name : autostart_plugins){
         auto plugin = find_plugin( plugin_name );
         if (plugin != nullptr && plugin->get_state() == abstract_plugin::registered)
            plugin->initialize(my->_args);
      }

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


const variables_map& application::get_args() const
{
   return my->_args;
}

void application_factory::add_program_options( const options_description& cli )
{
   app_options.add( cli );
}

application_factory& app_factory() { return application_factory::get_app_factory(); }


void application_factory::set_program_options()
{

   options_description app_cfg_opts( "Application Config Options" );
   options_description app_cli_opts( "Application Command Line Options" );
   app_cfg_opts.add_options()
         ("plugin", bpo::value< vector<string> >()->composing(), "Plugin(s) to enable, may be specified multiple times");

   app_cfg_opts.add_options()
         ("external-plugin", bpo::value< vector<string> >()->composing(), "External plugin(s) to enable, may be specified multiple times");

   app_cli_opts.add_options()
         ("startup-apps", bpo::value<vector<string>>()->composing(), "Applications to load at startup");

   app_cli_opts.add_options()
         ("external-plugins-dir", bpo::value<bfs::path>()->default_value( "external-plugins" ), "Directory containing external/runtime-loadable plugins binaries (absolute path or relative to the program option data-dir/)");

   app_cli_opts.add_options()
         ("help,h", "Print this help message and exit.")
         ("version,v", "Print version information.")
         ("data-dir,d", bpo::value<bfs::path>()->default_value( "sophia_app_data" ), "Directory containing configuration files, blockchain data and external plugins")
         ("config,c", bpo::value<bfs::path>()->default_value( "config.ini" ), "Main configuration file path (absolute path or relative to the data-dir/configs/)");

   app_options.add(app_cfg_opts);
   global_options.add(app_cli_opts);
}

map<string, std::shared_ptr<application>> application_factory::initialize( int argc, char** argv, vector< string > _autostart_plugins, bool start_apps )
{
   try
   {
      map<string, std::shared_ptr<application>> ret;
      bpo::store( bpo::parse_command_line( argc, argv, global_options ), global_args );
      std::copy(_autostart_plugins.begin(), _autostart_plugins.end(), std::back_inserter(autostart_plugins));

      if( global_args.count( "help" ) ) {
         cout << global_options << "\n";
         return ret;
      }

      if( global_args.count( "version" ) )
      {
         cout << version_info << "\n";
         return ret;
      }

      // data-dir par (even if it is default) must be always present
      assert(global_args.count( "data-dir" ));

      data_dir = global_args["data-dir"].as<bfs::path>();
      if( data_dir.is_relative() )
         data_dir = bfs::current_path() / data_dir;

      assert(global_args.count( "external-plugins-dir" ));

      boost::program_options::variable_value cfg_plugins_dir = global_args["external-plugins-dir"];
      plugins_dir = cfg_plugins_dir.as<bfs::path>();
      if (plugins_dir.is_relative() == true) {
         plugins_dir = data_dir / plugins_dir;

         if (bfs::exists(plugins_dir) == false) {
            bfs::create_directories(plugins_dir);
         }
      }



      // config par (even if it is default) must be always present
      assert(global_args.count( "config" ));

      // Writes config if it does not already exists
      bfs::path config_file_path = write_default_config(global_options, global_args["config"].as<bfs::path>());

      // Copies parameters from config_file into my->_args
      bpo::store(bpo::parse_config_file< char >( config_file_path.make_preferred().string().c_str(),
                                                 global_options, true ), global_args );

      //TODO: migrate the config here

      if(start_apps) {
         if( global_args.count("startup-apps") > 0 ) {
            auto chains = global_args.at("startup-apps").as<std::vector<string>>();
            for( auto &arg : chains ) {
               vector<string> names;
               boost::split(names, arg, boost::is_any_of(" \t,"));
               for( const std::string &name : names ) {
                  variables_map app_args = read_app_config(name);
                  auto new_app = new_application(name);
                  new_app->initialize(app_args, autostart_plugins);
                  ret[ name ] = new_app;
               }
            }
         } else {
            //load default (public) one.
            variables_map app_args = read_app_config(_public_net_chain_id);
            auto new_app = new_application(_public_net_chain_id);
            new_app->initialize(app_args, autostart_plugins);
            ret[ _public_net_chain_id ] = new_app;
         }
      }
      return ret;
   }
   catch (const boost::program_options::error& e)
   {
      map<string, std::shared_ptr<application>> ret;
      std::cerr << "Error parsing command line: " << e.what() << "\n";
      return ret;
   }
}

variables_map application_factory::read_app_config(std::string name)
{
   bfs::path config_file_path = write_default_config(app_options, global_args[ "config" ].as<bfs::path>(),
                                                     name);
   variables_map app_args;
   bpo::store(bpo::parse_config_file<char>(config_file_path.make_preferred().string().c_str(),
                                           app_options, true), app_args);
   return app_args;
}


} /// namespace appbase
