#include <sophiatx/plugins/webserver/webserver_plugin.hpp>

#include <sophiatx/plugins/chain/chain_plugin.hpp>

#include <fc/network/ip.hpp>
#include <fc/io/json.hpp>
#include <fc/network/resolve.hpp>
#include <fc/crypto/openssl.hpp>

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/logger/stub.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>


#include <thread>
#include <memory>
#include <iostream>

namespace fc {
   SSL_TYPE(ec_key, EC_KEY, EC_KEY_free)
}

namespace sophiatx { namespace plugins { namespace webserver {

namespace asio = boost::asio;

using std::map;
using std::string;
using boost::optional;
using boost::asio::ip::tcp;
using std::shared_ptr;
using websocketpp::connection_hdl;

typedef uint32_t thread_pool_size_t;

namespace detail {

   template<class T>
   struct asio_with_stub_log : public websocketpp::config::asio
   {
         typedef asio_with_stub_log type;
         typedef asio base;

         typedef base::concurrency_type concurrency_type;

         typedef base::request_type request_type;
         typedef base::response_type response_type;

         typedef base::message_type message_type;
         typedef base::con_msg_manager_type con_msg_manager_type;
         typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

         typedef base::alog_type alog_type;
         typedef base::elog_type elog_type;
         //typedef websocketpp::log::stub elog_type;
         //typedef websocketpp::log::stub alog_type;

         typedef base::rng_type rng_type;

         struct transport_config : public base::transport_config
         {
            typedef type::concurrency_type concurrency_type;
            typedef type::alog_type alog_type;
            typedef type::elog_type elog_type;
            typedef type::request_type request_type;
            typedef type::response_type response_type;
            typedef T socket_type;
         };

         typedef websocketpp::transport::asio::endpoint< transport_config >
            transport_type;

         static const long timeout_open_handshake = 0;

         /// permessage_compress extension
         struct permessage_deflate_config {};

         typedef websocketpp::extensions::permessage_deflate::enabled
               <permessage_deflate_config> permessage_deflate_type;
   };

using websocket_server_type      = websocketpp::server< detail::asio_with_stub_log<websocketpp::transport::asio::basic_socket::endpoint> >;
using websocket_server_tls_type  = websocketpp::server< detail::asio_with_stub_log<websocketpp::transport::asio::tls_socket::endpoint> >;
using ssl_context_ptr            = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;

std::vector<fc::ip::endpoint> resolve_string_to_ip_endpoints( const std::string& endpoint_string )
{
   try
   {
      string::size_type colon_pos = endpoint_string.find( ':' );
      if( colon_pos == std::string::npos )
         FC_THROW( "Missing required port number in endpoint string \"${endpoint_string}\"",
                  ("endpoint_string", endpoint_string) );

      std::string port_string = endpoint_string.substr( colon_pos + 1 );

      try
      {
         uint16_t port = boost::lexical_cast< uint16_t >( port_string );
         std::string hostname = endpoint_string.substr( 0, colon_pos );
         std::vector< fc::ip::endpoint > endpoints = fc::resolve( hostname, port );

         if( endpoints.empty() )
            FC_THROW_EXCEPTION( fc::unknown_host_exception, "The host name can not be resolved: ${hostname}", ("hostname", hostname) );

         return endpoints;
      }
      catch( const boost::bad_lexical_cast& )
      {
         FC_THROW("Bad port: ${port}", ("port", port_string) );
      }
   }
   FC_CAPTURE_AND_RETHROW( (endpoint_string) )
}

class webserver_plugin_impl
{
   public:
      webserver_plugin_impl(thread_pool_size_t thread_pool_size) :
         thread_pool_work( this->thread_pool_ios )
      {
         for( uint32_t i = 0; i < thread_pool_size; ++i )
            thread_pool.create_thread( boost::bind( &asio::io_service::run, &thread_pool_ios ) );
      }

      void start_webserver();
      void stop_webserver();

      template<class T>
      void handle_http_message(typename T::connection_ptr con);

      void handle_ws_message( websocket_server_type::connection_ptr con, detail::websocket_server_type::message_ptr );
      void send_ws_notice( websocket_server_type::connection_ptr con, const string& message );

      ssl_context_ptr on_tls_init(websocketpp::connection_hdl hdl);

      shared_ptr< std::thread >  http_thread;
      asio::io_service           http_ios;
      optional< tcp::endpoint >  http_endpoint;
      websocket_server_type      http_server;
      string                     http_cors;

      shared_ptr< std::thread >  https_thread;
      asio::io_service           https_ios;
      optional<tcp::endpoint>    https_endpoint;
      websocket_server_tls_type  https_server;
      string                     https_cert_chain;
      string                     https_key;

      shared_ptr< std::thread >  ws_thread;
      asio::io_service           ws_ios;
      optional< tcp::endpoint >  ws_endpoint;
      websocket_server_type      ws_server;

      boost::thread_group        thread_pool;
      asio::io_service           thread_pool_ios;
      asio::io_service::work     thread_pool_work;

      plugins::json_rpc::json_rpc_plugin* api;
      boost::signals2::connection         chain_sync_con;
};

void webserver_plugin_impl::start_webserver()
{
   if( ws_endpoint )
   {
      ws_thread = std::make_shared<std::thread>( [&]()
      {
         ilog( "start processing ws thread" );
         try
         {
            ws_server.clear_access_channels( websocketpp::log::alevel::all );
            ws_server.clear_error_channels( websocketpp::log::elevel::all );
            ws_server.init_asio( &ws_ios );
            ws_server.set_reuse_addr( true );

            ws_server.set_message_handler( [&](connection_hdl hdl, detail::websocket_server_type::message_ptr msg) { handle_ws_message(ws_server.get_con_from_hdl(hdl), msg); } );


            if( http_endpoint && http_endpoint == ws_endpoint )
            {
               ws_server.set_http_handler( [&](connection_hdl hdl) { handle_http_message<websocket_server_type>(ws_server.get_con_from_hdl(hdl) ); } );
               ilog( "start listending for http requests" );
            }

            ilog( "start listening for ws requests" );
            ws_server.listen( *ws_endpoint );
            ws_server.start_accept();

            ws_ios.run();
            ilog( "ws io service exit" );
         }
         catch ( const fc::exception& e )
         {
            elog( "ws service failed to start: ${e}", ("e",e.to_detail_string()));
         }
         catch ( const std::exception& e )
         {
            elog( "ws service failed to start: ${e}", ("e", e.what()));
         }
         catch( ... )
         {
            elog( "error thrown from ws io service" );
         }
      });
   }

   if( http_endpoint && ( ( ws_endpoint && ws_endpoint != http_endpoint ) || !ws_endpoint ) )
   {
      http_thread = std::make_shared<std::thread>( [&]()
      {
         ilog( "start processing http thread" );
         try
         {
            http_server.clear_access_channels( websocketpp::log::alevel::all );
            http_server.clear_error_channels( websocketpp::log::elevel::all );
            http_server.init_asio( &http_ios );
            http_server.set_reuse_addr( true );

            http_server.set_http_handler( [&](connection_hdl hdl) { handle_http_message<websocket_server_type>(http_server.get_con_from_hdl(hdl) ); } );

            ilog( "start listening for http requests" );
            http_server.listen( *http_endpoint );
            http_server.start_accept();

            http_ios.run();
            ilog( "http io service exit" );
         }
         catch ( const fc::exception& e )
         {
            elog( "http service failed to start: ${e}", ("e",e.to_detail_string()));
         }
         catch ( const std::exception& e )
         {
            elog( "http service failed to start: ${e}", ("e", e.what()));
         }
         catch( ... )
         {
            elog( "error thrown from http io service" );
         }
      });
   }

   if( https_endpoint )
   {
      https_thread = std::make_shared<std::thread>( [&]()
      {
          ilog( "start processing https thread" );
          try
          {
             https_server.clear_access_channels( websocketpp::log::alevel::all );
             https_server.clear_error_channels( websocketpp::log::elevel::all );
             https_server.init_asio( &http_ios );
             https_server.set_reuse_addr( true );
             https_server.set_tls_init_handler( [this]( websocketpp::connection_hdl hdl ) -> ssl_context_ptr { return on_tls_init(hdl); } );

             https_server.set_http_handler( [&](connection_hdl hdl) { handle_http_message<websocket_server_tls_type>(https_server.get_con_from_hdl(hdl) ); } );

             ilog( "start listening for https requests" );
             https_server.listen( *https_endpoint );
             https_server.start_accept();

             https_ios.run();
             ilog( "https io service exit" );
          }
          catch ( const fc::exception& e )
          {
             elog( "https service failed to start: ${e}", ("e",e.to_detail_string()));
          }
          catch ( const std::exception& e )
          {
             elog( "https service failed to start: ${e}", ("e", e.what()));
          }
          catch( ... )
          {
             elog( "error thrown from https io service" );
          }
      });
   }
}

void webserver_plugin_impl::stop_webserver()
{
   if( ws_server.is_listening() )
      ws_server.stop_listening();

   if( http_server.is_listening() )
      http_server.stop_listening();

   if( https_server.is_listening() )
      https_server.stop_listening();

   thread_pool_ios.stop();
   thread_pool.join_all();

   if( ws_thread )
   {
      ws_ios.stop();
      ws_thread->join();
      ws_thread.reset();
   }

   if( http_thread )
   {
      http_ios.stop();
      http_thread->join();
      http_thread.reset();
   }

   if( https_thread )
   {
      https_ios.stop();
      https_thread->join();
      https_thread.reset();
   }
}

ssl_context_ptr webserver_plugin_impl::on_tls_init(websocketpp::connection_hdl hdl) {
   ssl_context_ptr ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(asio::ssl::context::sslv23_server);

   try {
      ctx->set_options(asio::ssl::context::default_workarounds |
                       asio::ssl::context::no_sslv2 |
                       asio::ssl::context::no_sslv3 |
                       asio::ssl::context::no_tlsv1 |
                       asio::ssl::context::no_tlsv1_1 |
                       asio::ssl::context::single_dh_use);

      ctx->use_certificate_chain_file(https_cert_chain);
      ctx->use_private_key_file(https_key, asio::ssl::context::pem);

      fc::ec_key ecdh = EC_KEY_new_by_curve_name(NID_secp384r1);
      if (!ecdh)
         elog("Failed to set NID_secp384r1");
      if(SSL_CTX_set_tmp_ecdh(ctx->native_handle(), (EC_KEY*)ecdh) != 1)
         elog("Failed to set ECDH PFS");

      std::string ciphers = "EECDH+ECDSA+AESGCM:EECDH+aRSA+AESGCM:EECDH+ECDSA+SHA384:EECDH+ECDSA+SHA256:AES256:!DHE:!RSA:!AES128:!RC4:!DES:!3DES:!DSS:!SRP:!PSK:!EXP:!MD5:!LOW:!aNULL:!eNULL";
      if(SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1)
         elog("Error setting https cipher list");

   } catch (const fc::exception& e) {
      elog("https server initialization error: ${w}", ("w", e.to_detail_string()));
   } catch(std::exception& e) {
      elog("https server initialization error: ${w}", ("w", e.what()));
   }

   return ctx;
}

template<class T>
void webserver_plugin_impl::handle_http_message(typename T::connection_ptr con) {
   con->defer_http_response();

   thread_pool_ios.post( [con, this]()
   {
      auto body = con->get_request_body();

      if(!http_cors.empty())
         con->append_header("Access-Control-Allow-Origin", http_cors);

      try
      {
       bool is_error = false;
       con->set_body( api->call( body, is_error ) );

       if(is_error) {
          con->set_status( websocketpp::http::status_code::internal_server_error );
       } else {
          con->set_status( websocketpp::http::status_code::ok );
       }
      }
      catch( fc::exception& e )
      {
       edump( (e) );
       con->set_body( "Could not call API" );
       con->set_status( websocketpp::http::status_code::not_found );
      }
      catch( ... )
      {
       auto eptr = std::current_exception();

       try
       {
          if( eptr )
             std::rethrow_exception( eptr );

          con->set_body( "unknown error occurred" );
          con->set_status( websocketpp::http::status_code::internal_server_error );
       }
       catch( const std::exception& e )
       {
          std::stringstream s;
          s << "unknown exception: " << e.what();
          con->set_body( s.str() );
          con->set_status( websocketpp::http::status_code::internal_server_error );
       }
      }

      con->send_http_response();
   });
}

void webserver_plugin_impl::send_ws_notice( websocket_server_type::connection_ptr con, const string& message )
{
   try{
      thread_pool_ios.post( [con, message]() {
         try{
            con->send(message);
         }catch( ... )
         {
            fc::send_error_exception e;
            throw e;
            //we shall notify the caller and delete the callback association
         }
      });
   }catch(...){
      fc::send_error_exception e;
      throw e;
   }
}

void webserver_plugin_impl::handle_ws_message( websocket_server_type::connection_ptr con, detail::websocket_server_type::message_ptr msg )
{
   thread_pool_ios.post( [con, msg, this]()
   {
      try
      {
         if( msg->get_opcode() == websocketpp::frame::opcode::text )
            con->send( api->call( msg->get_payload(), [this, con](const string& message) { send_ws_notice(con, message); } ));
         else
            con->send( "error: string payload expected" );
      }
      catch( fc::exception& e )
      {
         con->send( "error calling API " + e.to_string() );
      }
      catch( ... )
      {
         auto eptr = std::current_exception();

         try
         {
            if( eptr )
               std::rethrow_exception( eptr );

            con->send( "unknown error occurred" );
         }
         catch( const std::exception& e )
         {
            std::stringstream s;
            s << "unknown exception: " << e.what();
            con->send( s.str() );
         }
      }
   });
}

} // detail

webserver_plugin::webserver_plugin() {}
webserver_plugin::~webserver_plugin() {}

void webserver_plugin::set_program_options( options_description&, options_description& cfg )
{
   cfg.add_options()
      ("webserver-ws-endpoint", bpo::value< string >(), "Local websocket endpoint for webserver requests.")
      ("webserver-http-endpoint", bpo::value< string >(), "Local http endpoint for webserver requests.")
      ("webserver-https-endpoint", bpo::value< string >(), "Local https endpoint for webserver requests.")
      ("https-certificate-chain-file", bpo::value< string >(), "File with certificate chain to present on https connections. Required for https.")
      ("https-private-key-file", bpo::value< string >(), "File with https private key in PEM format. Required for https.")
      ("http-cors", bpo::value<string>()->default_value("*"), "Access-Control-Allow-Origin response header")
      ("webserver-thread-pool-size", bpo::value<thread_pool_size_t>()->default_value(16), "Number of threads used to handle queries. Default: 16.");
}

void webserver_plugin::plugin_initialize( const variables_map& options )
{
   auto thread_pool_size = options.at("webserver-thread-pool-size").as<thread_pool_size_t>();
   FC_ASSERT(thread_pool_size > 0, "webserver-thread-pool-size must be greater than 0");
   ilog("configured with ${tps} thread pool size", ("tps", thread_pool_size));
   my.reset(new detail::webserver_plugin_impl(thread_pool_size));

   if( options.count( "webserver-ws-endpoint" ) )
   {
      auto ws_endpoint = options.at( "webserver-ws-endpoint" ).as< string >();
      auto endpoints = detail::resolve_string_to_ip_endpoints( ws_endpoint );
      FC_ASSERT( endpoints.size(), "ws-server-endpoint ${hostname} did not resolve", ("hostname", ws_endpoint) );
      my->ws_endpoint = tcp::endpoint( boost::asio::ip::address_v4::from_string( ( string )endpoints[0].get_address() ), endpoints[0].port() );
      ilog( "configured ws to listen on ${ep}", ("ep", endpoints[0]) );
   }

   if(options.count( "webserver-http-endpoint" ) || options.count("webserver-https-endpoint")) {
      my->http_cors = options.at( "http-cors" ).as< string >();
   }

   if( options.count( "webserver-http-endpoint" ) )
   {
      auto http_endpoint = options.at( "webserver-http-endpoint" ).as< string >();
      auto endpoints = detail::resolve_string_to_ip_endpoints( http_endpoint );
      FC_ASSERT( endpoints.size(), "webserver-http-endpoint ${hostname} did not resolve", ("hostname", http_endpoint) );
      my->http_endpoint = tcp::endpoint( boost::asio::ip::address_v4::from_string( ( string )endpoints[0].get_address() ), endpoints[0].port() );
      ilog( "configured http to listen on ${ep}", ("ep", endpoints[0]) );
   }

   if (options.count("webserver-https-endpoint"))
   {
      FC_ASSERT(options.count("https-certificate-chain-file") > 0 , "https-certificate-chain-file is required for https.");
      FC_ASSERT(options.count("https-private-key-file") > 0, "https-private-key-file is required for https.");

      my->https_cert_chain = options.at("https-certificate-chain-file").as<string>();
      FC_ASSERT(my->https_cert_chain.empty() == false, "https-certificate-chain-file is required for https.");
      my->https_key = options.at("https-private-key-file").as<string>();
      FC_ASSERT(my->https_key.empty() == false, "https-private-key-file is required for https.");

      auto https_endpoint = options.at( "webserver-https-endpoint" ).as< string >();
      auto endpoints = detail::resolve_string_to_ip_endpoints( https_endpoint );
      FC_ASSERT( endpoints.size(), "webserver-https-endpoint ${hostname} did not resolve", ("hostname", https_endpoint) );
      my->https_endpoint = tcp::endpoint( boost::asio::ip::address_v4::from_string( ( string )endpoints[0].get_address() ), endpoints[0].port() );
      FC_ASSERT(my->https_endpoint != my->http_endpoint, "webserver-https-endpoint must be different than webserver-http-endpoint");
      FC_ASSERT(my->https_endpoint != my->ws_endpoint, "webserver-https-endpoint must be different than webserver-ws-endpoint");
      ilog( "configured https to listen on ${ep}", ("ep", endpoints[0]) );
   }
}

void webserver_plugin::plugin_startup()
{
   my->api = appbase::app().find_plugin< plugins::json_rpc::json_rpc_plugin >();
   FC_ASSERT( my->api != nullptr, "Could not find API Register Plugin" );

   plugins::chain::chain_plugin* chain = appbase::app().find_plugin< plugins::chain::chain_plugin >();
   if( chain != nullptr && chain->get_state() != appbase::abstract_plugin::started )
   {
      ilog( "Waiting for chain plugin to start" );
      my->chain_sync_con = chain->on_sync.connect( 0, [this]()
      {
         my->start_webserver();
      });
   }
   else
   {
      my->start_webserver();
   }
}

void webserver_plugin::plugin_shutdown()
{
   my->stop_webserver();
}

} } } // sophiatx::plugins::webserver
