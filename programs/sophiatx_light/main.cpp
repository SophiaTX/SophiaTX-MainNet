#include <appbase/application.hpp>
#include <sophiatx/manifest/plugins.hpp>

#include <sophiatx/protocol/types.hpp>
#include <sophiatx/protocol/version.hpp>

#include <sophiatx/utilities/key_conversion.hpp>
#include <sophiatx/utilities/git_revision.hpp>

#include <sophiatx/plugins/chain/chain_plugin.hpp>
#include <sophiatx/plugins/webserver/webserver_plugin.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/git_revision.hpp>
#include <fc/stacktrace.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <csignal>
#include <vector>

#include <fc/crypto/rand.hpp>
#include <sophiatx/plugins/chain/chain_plugin_lite.hpp>

namespace bpo = boost::program_options;
using sophiatx::protocol::version;
using std::string;
using std::vector;

string& version_string()
{
   static string v_str =
         "sophiatx_blockchain_version: " + fc::string( SOPHIATX_BLOCKCHAIN_VERSION ) + "\n" +
         "sophiatx_git_revision:       " + fc::string( sophiatx::utilities::git_revision_sha ) + "\n" +
         "fc_git_revision:          " + fc::string( fc::git_revision_sha ) + "\n";
   return v_str;
}

void info()
{
   std::cerr << "------------------------------------------------------\n\n";
   std::cerr << "            STARTING SOPHIATX LIGHT CLIENT\n\n";
   std::cerr << "------------------------------------------------------\n";
}

int main( int argc, char** argv )
{
   try
   {
      appbase::app().register_plugin<sophiatx::plugins::chain::chain_plugin_lite>();
      sophiatx::plugins::register_plugins();

      // Reads main application config file
      appbase::app().load_config(argc, argv);
      auto& args = appbase::app().get_args();

      // Initializes logger
      fc::Logger::init("sophiatx_light"/* Do not change this parameter as syslog config depends on it !!! */, args.at("log-level").as< std::string >());

      fc::ecc::public_key::init_cache(static_cast<uint32_t>(SOPHIATX_MAX_BLOCK_SIZE / SOPHIATX_MIN_TRANSACTION_SIZE_LIMIT), std::chrono::milliseconds(2000));
      appbase::app().set_version_string( version_string() );

      bool initialized = appbase::app().initialize<
                     sophiatx::plugins::chain::chain_plugin_lite,
                     sophiatx::plugins::json_rpc::json_rpc_plugin,
                     sophiatx::plugins::webserver::webserver_plugin >( argc, argv );

      info();

      if( !initialized ) {
         return 0;
      }

      if( args.at( "backtrace" ).as< string >() == "yes" )
      {
         fc::print_stacktrace_on_segfault();
         ilog( "Backtrace on segfault is enabled." );
      }

      appbase::app().startup();
      appbase::app().exec();
      ilog("exited cleanly");

      return 0;
   }
   catch ( const boost::exception& e )
   {
      std::cerr << boost::diagnostic_information(e) << "\n";
   }
   catch ( const fc::exception& e )
   {
      std::cerr << e.to_detail_string() << "\n";
   }
   catch ( const std::exception& e )
   {
      std::cerr << e.what() << "\n";
   }
   catch ( ... )
   {
      std::cerr << "unknown exception\n";
   }

   return -1;
}
