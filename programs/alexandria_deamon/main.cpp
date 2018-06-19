/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/server.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/http_api.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/smart_ref_impl.hpp>

#include <steem/utilities/key_conversion.hpp>

#include <steem/protocol/protocol.hpp>
#include <steem/alexandria/remote_node_api.hpp>
#include <steem/alexandria/lib_alexandria.hpp>

#include <fc/interprocess/signals.hpp>
#include <boost/algorithm/string.hpp>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger_config.hpp>

#ifdef WIN32
# include <signal.h>
#endif


using namespace steem::utilities;
using namespace steem::chain;
using namespace steem::wallet;
using namespace std;
namespace bpo = boost::program_options;

int main( int argc, char** argv )
{
   try {

      boost::program_options::options_description opts;
         opts.add_options()
         ("help,h", "Print this help message and exit.")
         ("server-rpc-endpoint,s", bpo::value<string>()->implicit_value("ws://127.0.0.1:8090"), "Server websocket RPC endpoint")
         ("rpc-endpoint,r", bpo::value<string>()->implicit_value("127.0.0.1:8091"), "Endpoint for wallet websocket RPC to listen on")
         ("rpc-http-endpoint,H", bpo::value<string>()->implicit_value("127.0.0.1:8093"), "Endpoint for wallet HTTP RPC to listen on")
         ("daemon,d", "Run the wallet in daemon mode" );

      bpo::variables_map options;

      bpo::store( bpo::parse_command_line(argc, argv, opts), options );

      if( options.count("help") )
      {
         std::cout << opts << "\n";
         return 0;
      }
      steem::protocol::chain_id_type _steem_chain_id = STEEM_CHAIN_ID;

      fc::path data_dir;
      fc::logging_config cfg;
      fc::path log_dir = data_dir / "logs";

      fc::file_appender::config ac;
      ac.filename             = log_dir / "rpc" / "rpc.log";
      ac.flush                = true;
      ac.rotate               = true;
      ac.rotation_interval    = fc::hours( 1 );
      ac.rotation_limit       = fc::days( 1 );

      std::cout << "Logging RPC to file: " << (data_dir / ac.filename).preferred_string() << "\n";

      cfg.appenders.push_back(fc::appender_config( "default", "console", fc::variant(fc::console_appender::config())));
      cfg.appenders.push_back(fc::appender_config( "rpc", "file", fc::variant(ac)));

      cfg.loggers = { fc::logger_config("default"), fc::logger_config( "rpc") };
      cfg.loggers.front().level = fc::log_level::info;
      cfg.loggers.front().appenders = {"default"};
      cfg.loggers.back().level = fc::log_level::debug;
      cfg.loggers.back().appenders = {"rpc"};

      string ws_server = "ws://127.0.0.1:8090";
      // but allow CLI to override
      if( options.count("server-rpc-endpoint") )
         ws_server = options.at("server-rpc-endpoint").as<std::string>();

      fc::http::websocket_client client;
      auto con  = client.connect( ws_server );
      auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

      auto remote_api = apic->get_remote_api< steem::wallet::remote_node_api >( 0, "condenser_api" );

      auto alex_apiptr = std::make_shared<alexandria_api>( _steem_chain_id, remote_api );

      fc::api<alexandria_api> alex_api(alex_apiptr);

      auto alexandria_deamon = std::make_shared<fc::rpc::cli>();
      for( auto& name_formatter : alex_apiptr->get_result_formatters() )
         alexandria_deamon->format_result( name_formatter.first, name_formatter.second );

      boost::signals2::scoped_connection closed_connection(con->closed.connect([=]{
         cerr << "Server has disconnected us.\n";
         alexandria_deamon->stop();
      }));
      (void)(closed_connection);

      auto _websocket_server = std::make_shared<fc::http::websocket_server>();
      if( options.count("rpc-endpoint") )
        {
            _websocket_server->on_connection([&]( const fc::http::websocket_connection_ptr& c ){
            std::cout << "here... \n";
            wlog("." );
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
            wsc->register_api(alex_api);
            c->set_session_data( wsc );
            });
            ilog( "Listening for incoming RPC requests on ${p}", ("p", options.at("rpc-endpoint").as<string>() ));
            _websocket_server->listen( fc::ip::endpoint::from_string(options.at("rpc-endpoint").as<string>()) );
            _websocket_server->start_accept();
      }

      auto _http_server = std::make_shared<fc::http::server>();
      if( options.count("rpc-http-endpoint" ) )
      {
         ilog( "Listening for incoming HTTP RPC requests on ${p}", ("p", options.at("rpc-http-endpoint").as<string>() ) );

         _http_server->listen( fc::ip::endpoint::from_string( options.at( "rpc-http-endpoint" ).as<string>() ) );
         //
         // due to implementation, on_request() must come AFTER listen()
         //
         _http_server->on_request(
            [&]( const fc::http::request& req, const fc::http::server::response& resp )
            {
               std::shared_ptr< fc::rpc::http_api_connection > conn =
                  std::make_shared< fc::rpc::http_api_connection>();
               conn->register_api( alex_api );
               conn->on_request( req, resp );
            } );
      }

      if( !options.count( "daemon" ) )
      {
         alexandria_deamon->register_api( alex_api );
         alexandria_deamon->start();
         alexandria_deamon->wait();
      }
      else
      {
        fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");
        fc::set_signal_handler([&exit_promise](int signal) {
           exit_promise->set_value(signal);
        }, SIGINT);

        ilog( "Entering Daemon Mode, ^C to exit" );
        exit_promise->wait();
      }

      closed_connection.disconnect();
   }
   catch ( const fc::exception& e )
   {
      std::cout << e.to_detail_string() << "\n";
      return -1;
   }
   return 0;
}
