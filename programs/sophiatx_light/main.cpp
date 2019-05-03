#include <appbase/application.hpp>
#include <sophiatx/manifest/plugins.hpp>

#include <sophiatx/protocol/types.hpp>
#include <sophiatx/protocol/version.hpp>

#include <sophiatx/utilities/logging_config.hpp>
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
      // Setup logging config
      bpo::options_description options;

      fc::ecc::public_key::init_cache(static_cast<uint32_t>(SOPHIATX_MAX_BLOCK_SIZE / SOPHIATX_MIN_TRANSACTION_SIZE_LIMIT), std::chrono::milliseconds(2000));

      sophiatx::utilities::set_logging_program_options( options );
      options.add_options()
            ("backtrace", bpo::value< string >()->default_value( "yes" ), "Whether to print backtrace on SIGSEGV" );

      appbase::app().add_program_options( bpo::options_description(), options );

      appbase::app().register_plugin<sophiatx::plugins::chain::chain_plugin_lite>();
      sophiatx::plugins::register_plugins();
      appbase::app().set_version_string( version_string() );

      bool initialized = appbase::app().initialize<
                     sophiatx::plugins::chain::chain_plugin_lite,
                     sophiatx::plugins::json_rpc::json_rpc_plugin,
                     sophiatx::plugins::webserver::webserver_plugin >( argc, argv );

      info();

      if( !initialized )
         return 0;

      auto& args = appbase::app().get_args();

      try
      {
         fc::optional< fc::logging_config > logging_config = sophiatx::utilities::load_logging_config( args, appbase::app().data_dir() );
         if( logging_config )
            fc::configure_logging( *logging_config );
      }
      catch( const fc::exception& )
      {
         wlog( "Error parsing logging config" );
      }

      if( args.at( "backtrace" ).as< string >() == "yes" )
      {
         fc::print_stacktrace_on_segfault();
         ilog( "Backtrace on segfault is enabled." );
      }

      appbase::app().startup();
      appbase::app().exec();
      std::cout << "exited cleanly\n";

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
