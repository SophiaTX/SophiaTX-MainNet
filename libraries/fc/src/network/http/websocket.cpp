#include <fc/network/http/websocket.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/logger/stub.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/asio.hpp>

namespace fc { namespace http {

   namespace detail {

      struct asio_with_stub_log : public websocketpp::config::asio {

          typedef asio_with_stub_log type;
          typedef asio base;

          typedef base::concurrency_type concurrency_type;

          typedef base::request_type request_type;
          typedef base::response_type response_type;

          typedef base::message_type message_type;
          typedef base::con_msg_manager_type con_msg_manager_type;
          typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

          /// Custom Logging policies
          /*typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::elevel> elog_type;
          typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::alevel> alog_type;
          */
          //typedef base::alog_type alog_type;
          //typedef base::elog_type elog_type;
          typedef websocketpp::log::stub elog_type;
          typedef websocketpp::log::stub alog_type;

          typedef base::rng_type rng_type;

          struct transport_config : public base::transport_config {
              typedef type::concurrency_type concurrency_type;
              typedef type::alog_type alog_type;
              typedef type::elog_type elog_type;
              typedef type::request_type request_type;
              typedef type::response_type response_type;
              typedef websocketpp::transport::asio::basic_socket::endpoint
                  socket_type;
          };

          typedef websocketpp::transport::asio::endpoint<transport_config>
              transport_type;
          
          static const long timeout_open_handshake = 0;
      };
      struct asio_tls_with_stub_log : public websocketpp::config::asio_tls {

          typedef asio_with_stub_log type;
          typedef asio_tls base;

          typedef base::concurrency_type concurrency_type;

          typedef base::request_type request_type;
          typedef base::response_type response_type;

          typedef base::message_type message_type;
          typedef base::con_msg_manager_type con_msg_manager_type;
          typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

          /// Custom Logging policies
          /*typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::elevel> elog_type;
          typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::alevel> alog_type;
          */
          //typedef base::alog_type alog_type;
          //typedef base::elog_type elog_type;
          typedef websocketpp::log::stub elog_type;
          typedef websocketpp::log::stub alog_type;

          typedef base::rng_type rng_type;

          struct transport_config : public base::transport_config {
              typedef type::concurrency_type concurrency_type;
              typedef type::alog_type alog_type;
              typedef type::elog_type elog_type;
              typedef type::request_type request_type;
              typedef type::response_type response_type;
              typedef websocketpp::transport::asio::tls_socket::endpoint socket_type;
          };

          typedef websocketpp::transport::asio::endpoint<transport_config>
              transport_type;
          
          static const long timeout_open_handshake = 0;
      };
      struct asio_tls_stub_log : public websocketpp::config::asio_tls {
         typedef asio_tls_stub_log type;
         typedef asio_tls base;

         typedef base::concurrency_type concurrency_type;

         typedef base::request_type request_type;
         typedef base::response_type response_type;

         typedef base::message_type message_type;
         typedef base::con_msg_manager_type con_msg_manager_type;
         typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

         //typedef base::alog_type alog_type;
         //typedef base::elog_type elog_type;
         typedef websocketpp::log::stub elog_type;
         typedef websocketpp::log::stub alog_type;

         typedef base::rng_type rng_type;

         struct transport_config : public base::transport_config {
         typedef type::concurrency_type concurrency_type;
         typedef type::alog_type alog_type;
         typedef type::elog_type elog_type;
         typedef type::request_type request_type;
         typedef type::response_type response_type;
         typedef websocketpp::transport::asio::tls_socket::endpoint socket_type;
         };

         typedef websocketpp::transport::asio::endpoint<transport_config>
         transport_type;
      };





      using websocketpp::connection_hdl;
      typedef websocketpp::server<asio_with_stub_log>  websocket_server_type;
      typedef websocketpp::server<asio_tls_stub_log>   websocket_tls_server_type;

      template<typename T>
      class websocket_connection_impl : public websocket_connection
      {
         public:
            websocket_connection_impl( T con )
            :_ws_connection(con){
               wdump((uint64_t(this)));
            }

            ~websocket_connection_impl()
            {
               wdump((uint64_t(this)));
            }

            virtual void send_message( const std::string& message )override
            {
               idump((message));
               auto ec = _ws_connection->send( message );
               FC_ASSERT( !ec, "websocket send failed: ${msg}", ("msg",ec.message() ) );
            }
            virtual void close( int64_t code, const std::string& reason  )override
            {
               _ws_connection->close(code,reason);
            }

            T _ws_connection;
      };

      typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

      class websocket_server_impl
      {
         public:
            websocket_server_impl()
            :_server_thread( fc::thread::current() )
            {

               _server.clear_access_channels( websocketpp::log::alevel::all );
               _server.init_asio(&fc::asio::default_io_service());
               _server.set_reuse_addr(true);
               _server.set_open_handler( [&]( connection_hdl hdl ){
                    _server_thread.async( [&](){
                       auto new_con = std::make_shared<websocket_connection_impl<websocket_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       _on_connection( _connections[hdl] = new_con );
                    }).wait();
               });
               _server.set_message_handler( [&]( connection_hdl hdl, websocket_server_type::message_ptr msg ){
                    _server_thread.async( [&](){
                       auto current_con = _connections.find(hdl);
                       assert( current_con != _connections.end() );
                       wdump(("server")(msg->get_payload()));
                       current_con->second->on_message( msg->get_payload()  );
                    }).wait();
               });

               _server.set_http_handler( [&]( connection_hdl hdl ){
                    _server_thread.async( [&](){

                       auto current_con = std::make_shared<websocket_connection_impl<websocket_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       _on_connection( current_con );

                       auto con = _server.get_con_from_hdl(hdl);
                       wdump(("server")(con->get_request_body()));
                       auto response = current_con->on_http( con->get_request_body() );

                       con->set_body( response );
                       con->set_status( websocketpp::http::status_code::ok );
                       current_con->closed();

                    }).wait();
               });

               _server.set_close_handler( [&]( connection_hdl hdl ){
                    _server_thread.async( [&](){
                       if( _connections.find(hdl) != _connections.end() )
                       {
                          _connections[hdl]->closed();
                          _connections.erase( hdl );  
                       }
                       else
                       {
                            wlog( "unknown connection closed" );
                       }
                    }).wait();
               });

               _server.set_fail_handler( [&]( connection_hdl hdl ){
                    if( _server.is_listening() )
                    {
                       _server_thread.async( [&](){
                          if( _connections.find(hdl) != _connections.end() )
                          {
                             _connections[hdl]->closed();
                             _connections.erase( hdl );  
                          }
                          else
                          {
                            wlog( "unknown connection failed" );
                          }
                       }).wait();
                    }
               });
            }
            ~websocket_server_impl()
            {
               if( _server.is_listening() )
                  _server.stop_listening();
               auto cpy_con = _connections;
               for( auto item : cpy_con )
                  _server.close( item.first, 0, "server exit" );
            }

            typedef std::map<connection_hdl, websocket_connection_ptr,std::owner_less<connection_hdl> > con_map;

            con_map                  _connections;
            fc::thread&              _server_thread;
            websocket_server_type    _server;
            on_connection_handler    _on_connection;
            fc::promise<void>::ptr   _closed;
      };

      class websocket_tls_server_impl
      {
         public:
            websocket_tls_server_impl( const string& server_pem, const string& ssl_password )
            :_server_thread( fc::thread::current() )
            {
               //if( server_pem.size() )
               {
                  _server.set_tls_init_handler( [=]( websocketpp::connection_hdl hdl ) -> context_ptr {
                        context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);
                        try {
                           ctx->set_options(boost::asio::ssl::context::default_workarounds |
                           boost::asio::ssl::context::no_sslv2 |
                           boost::asio::ssl::context::no_sslv3 |
                           boost::asio::ssl::context::single_dh_use);
                           ctx->set_password_callback([=](std::size_t max_length, boost::asio::ssl::context::password_purpose){ return ssl_password;});
                           ctx->use_certificate_chain_file(server_pem);
                           ctx->use_private_key_file(server_pem, boost::asio::ssl::context::pem);
                        } catch (std::exception& e) {
                           std::cout << e.what() << std::endl;
                        }
                        return ctx;
                  });
               }

               _server.clear_access_channels( websocketpp::log::alevel::all );
               _server.init_asio(&fc::asio::default_io_service());
               _server.set_reuse_addr(true);
               _server.set_open_handler( [&]( connection_hdl hdl ){
                    _server_thread.async( [&](){
                       auto new_con = std::make_shared<websocket_connection_impl<websocket_tls_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       _on_connection( _connections[hdl] = new_con );
                    }).wait();
               });
               _server.set_message_handler( [&]( connection_hdl hdl, websocket_server_type::message_ptr msg ){
                    _server_thread.async( [&](){
                       auto current_con = _connections.find(hdl);
                       assert( current_con != _connections.end() );
                       wdump(("server")(msg->get_payload()));
                       current_con->second->on_message( msg->get_payload()  );
                    }).wait();
               });

               _server.set_http_handler( [&]( connection_hdl hdl ){
                    _server_thread.async( [&](){

                       auto current_con = std::make_shared<websocket_connection_impl<websocket_tls_server_type::connection_ptr>>( _server.get_con_from_hdl(hdl) );
                       try{
                          _on_connection( current_con );

                          auto con = _server.get_con_from_hdl(hdl);
                          wdump(("server")(con->get_request_body()));
                          auto response = current_con->on_http( con->get_request_body() );

                          con->set_body( response );
                          con->set_status( websocketpp::http::status_code::ok );
                       } catch ( const fc::exception& e )
                       {                   
                         edump((e.to_detail_string()));
                       }
                       current_con->closed();

                    }).wait();
               });

               _server.set_close_handler( [&]( connection_hdl hdl ){
                    _server_thread.async( [&](){
                       _connections[hdl]->closed();
                       _connections.erase( hdl );  
                    }).wait();
               });

               _server.set_fail_handler( [&]( connection_hdl hdl ){
                    if( _server.is_listening() )
                    {
                       _server_thread.async( [&](){
                          if( _connections.find(hdl) != _connections.end() )
                          {
                             _connections[hdl]->closed();
                             _connections.erase( hdl );  
                          }
                       }).wait();
                    }
               });
            }
            ~websocket_tls_server_impl()
            {
               if( _server.is_listening() )
                  _server.stop_listening();
               auto cpy_con = _connections;
               for( auto item : cpy_con )
                  _server.close( item.first, 0, "server exit" );
            }

            typedef std::map<connection_hdl, websocket_connection_ptr,std::owner_less<connection_hdl> > con_map;

            con_map                     _connections;
            fc::thread&                 _server_thread;
            websocket_tls_server_type   _server;
            on_connection_handler       _on_connection;
            fc::promise<void>::ptr      _closed;
      };













      typedef websocketpp::client<asio_with_stub_log> websocket_client_type; 
      typedef websocketpp::client<asio_tls_stub_log> websocket_tls_client_type; 

      typedef websocket_client_type::connection_ptr  websocket_client_connection_type; 
      typedef websocket_tls_client_type::connection_ptr  websocket_tls_client_connection_type; 

      class websocket_client_impl 
      {
         public:
            typedef websocket_client_type::message_ptr message_ptr;

            websocket_client_impl()
            :_client_thread( fc::thread::current() )
            {
                _client.clear_access_channels( websocketpp::log::alevel::all );
                _client.set_message_handler( [&]( connection_hdl hdl, message_ptr msg ){
                   _client_thread.async( [&](){
                        wdump((msg->get_payload()));
                      _connection->on_message( msg->get_payload() );
                   }).wait();
                });
                _client.set_close_handler( [=]( connection_hdl hdl ){
                   if( _connection )
                      _client_thread.async( [&](){ if( _connection ) _connection->closed(); _connection.reset(); } ).wait();
                   if( _closed ) _closed->set_value();
                });
                _client.set_fail_handler( [=]( connection_hdl hdl ){
                   auto con = _client.get_con_from_hdl(hdl);
                   auto message = con->get_ec().message();
                   if( _connection )
                      _client_thread.async( [&](){ if( _connection ) _connection->closed(); _connection.reset(); } ).wait();
                   if( _connected && !_connected->ready() ) 
                       _connected->set_exception( exception_ptr( new FC_EXCEPTION( exception, "${message}", ("message",message)) ) );
                   if( _closed ) 
                       _closed->set_value();
                });

                _client.init_asio( &fc::asio::default_io_service() );
            }
            ~websocket_client_impl()
            {
               if(_connection )
               {
                  _connection->close(0, "client closed");
                  _closed->wait();
               }
            }
            fc::promise<void>::ptr             _connected;
            fc::promise<void>::ptr             _closed;
            fc::thread&                        _client_thread;
            websocket_client_type              _client;
            websocket_connection_ptr           _connection;
      };



      class websocket_tls_client_impl 
      {
         public:
            typedef websocket_tls_client_type::message_ptr message_ptr;

            websocket_tls_client_impl()
            :_client_thread( fc::thread::current() )
            {
                _client.clear_access_channels( websocketpp::log::alevel::all );
                _client.set_message_handler( [&]( connection_hdl hdl, message_ptr msg ){
                   _client_thread.async( [&](){
                        wdump((msg->get_payload()));
                      _connection->on_message( msg->get_payload() );
                   }).wait();
                });
                _client.set_close_handler( [=]( connection_hdl hdl ){
                   if( _connection )
                   {
                      try {
                         _client_thread.async( [&](){ 
                                 wlog(". ${p}", ("p",uint64_t(_connection.get()))); 
                                 if( !_shutting_down && !_closed && _connection ) 
                                    _connection->closed(); 
                                 _connection.reset(); 
                         } ).wait();
                      } catch ( const fc::exception& e )
                      {
                          if( _closed ) _closed->set_exception( e.dynamic_copy_exception() );
                      }
                      if( _closed ) _closed->set_value();
                   }
                });
                _client.set_fail_handler( [=]( connection_hdl hdl ){
                   elog( "." );
                   auto con = _client.get_con_from_hdl(hdl);
                   auto message = con->get_ec().message();
                   if( _connection )
                      _client_thread.async( [&](){ if( _connection ) _connection->closed(); _connection.reset(); } ).wait();
                   if( _connected && !_connected->ready() ) 
                       _connected->set_exception( exception_ptr( new FC_EXCEPTION( exception, "${message}", ("message",message)) ) );
                   if( _closed ) 
                       _closed->set_value();
                });

                _client.set_tls_init_handler( [=](websocketpp::connection_hdl) {
                   context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);
                   try {
                      ctx->set_options(boost::asio::ssl::context::default_workarounds |
                      boost::asio::ssl::context::no_sslv2 |
                      boost::asio::ssl::context::no_sslv3 |
                      boost::asio::ssl::context::single_dh_use);
                   } catch (std::exception& e) {
                      edump((e.what()));
                      std::cout << e.what() << std::endl;
                   }
                   return ctx;
                });

                _client.init_asio( &fc::asio::default_io_service() );
            }
            ~websocket_tls_client_impl()
            {
               if(_connection )
               {
                  wlog(".");
                  _shutting_down = true;
                  _connection->close(0, "client closed");
                  _closed->wait();
               }
            }
            bool                               _shutting_down = false;
            fc::promise<void>::ptr             _connected;
            fc::promise<void>::ptr             _closed;
            fc::thread&                        _client_thread;
            websocket_tls_client_type              _client;
            websocket_connection_ptr           _connection;
      };


   } // namespace detail

   websocket_server::websocket_server():my( new detail::websocket_server_impl() ) {}
   websocket_server::~websocket_server(){}

   void websocket_server::on_connection( const on_connection_handler& handler )
   {
      my->_on_connection = handler;
   }

   void websocket_server::listen( uint16_t port )
   {
      my->_server.listen(port);
   }
   void websocket_server::listen( const fc::ip::endpoint& ep )
   {
      my->_server.listen( boost::asio::ip::tcp::endpoint( boost::asio::ip::address_v4(uint32_t(ep.get_address())),ep.port()) );
   }

   void websocket_server::start_accept() { 
      my->_server.start_accept(); 
   }




   websocket_tls_server::websocket_tls_server( const string& server_pem, const string& ssl_password ):my( new detail::websocket_tls_server_impl(server_pem, ssl_password) ) {}
   websocket_tls_server::~websocket_tls_server(){}

   void websocket_tls_server::on_connection( const on_connection_handler& handler )
   {
      my->_on_connection = handler;
   }

   void websocket_tls_server::listen( uint16_t port )
   {
      my->_server.listen(port);
   }
   void websocket_tls_server::listen( const fc::ip::endpoint& ep )
   {
      my->_server.listen( boost::asio::ip::tcp::endpoint( boost::asio::ip::address_v4(uint32_t(ep.get_address())),ep.port()) );
   }

   void websocket_tls_server::start_accept() { 
      my->_server.start_accept(); 
   }


   websocket_tls_client::websocket_tls_client():my( new detail::websocket_tls_client_impl() ) {}
   websocket_tls_client::~websocket_tls_client(){ }



   websocket_client::websocket_client():my( new detail::websocket_client_impl() ),smy(new detail::websocket_tls_client_impl()) {}
   websocket_client::~websocket_client(){ }

   websocket_connection_ptr websocket_client::connect( const std::string& uri )
   { try {
       if( uri.substr(0,4) == "wss:" ) 
          return secure_connect(uri);
       FC_ASSERT( uri.substr(0,3) == "ws:" );

       // wlog( "connecting to ${uri}", ("uri",uri));
       websocketpp::lib::error_code ec;

       my->_connected = fc::promise<void>::ptr( new fc::promise<void>("websocket::connect") );

       my->_client.set_open_handler( [=]( websocketpp::connection_hdl hdl ){
          auto con =  my->_client.get_con_from_hdl(hdl);
          my->_connection = std::make_shared<detail::websocket_connection_impl<detail::websocket_client_connection_type>>( con );
          my->_closed = fc::promise<void>::ptr( new fc::promise<void>("websocket::closed") );
          my->_connected->set_value();
       });

       auto con = my->_client.get_connection( uri, ec );

       if( ec ) FC_ASSERT( !ec, "error: ${e}", ("e",ec.message()) );

       my->_client.connect(con);
       my->_connected->wait();
       return my->_connection;
   } FC_CAPTURE_AND_RETHROW( (uri) ) }

   websocket_connection_ptr websocket_client::secure_connect( const std::string& uri )
   { try {
       if( uri.substr(0,3) == "ws:" ) 
          return connect(uri);
       FC_ASSERT( uri.substr(0,4) == "wss:" );
       // wlog( "connecting to ${uri}", ("uri",uri));
       websocketpp::lib::error_code ec;

       smy->_connected = fc::promise<void>::ptr( new fc::promise<void>("websocket::connect") );

       smy->_client.set_open_handler( [=]( websocketpp::connection_hdl hdl ){
          auto con =  smy->_client.get_con_from_hdl(hdl);
          smy->_connection = std::make_shared<detail::websocket_connection_impl<detail::websocket_tls_client_connection_type>>( con );
          smy->_closed = fc::promise<void>::ptr( new fc::promise<void>("websocket::closed") );
          smy->_connected->set_value();
       });

       auto con = smy->_client.get_connection( uri, ec );
       if( ec )
          FC_ASSERT( !ec, "error: ${e}", ("e",ec.message()) );
       smy->_client.connect(con);
       smy->_connected->wait();
       return smy->_connection;
   } FC_CAPTURE_AND_RETHROW( (uri) ) }

   websocket_connection_ptr websocket_tls_client::connect( const std::string& uri )
   { try {
       // wlog( "connecting to ${uri}", ("uri",uri));
       websocketpp::lib::error_code ec;

       my->_connected = fc::promise<void>::ptr( new fc::promise<void>("websocket::connect") );

       my->_client.set_open_handler( [=]( websocketpp::connection_hdl hdl ){
          auto con =  my->_client.get_con_from_hdl(hdl);
          my->_connection = std::make_shared<detail::websocket_connection_impl<detail::websocket_tls_client_connection_type>>( con );
          my->_closed = fc::promise<void>::ptr( new fc::promise<void>("websocket::closed") );
          my->_connected->set_value();
       });

       auto con = my->_client.get_connection( uri, ec );
       if( ec )
       {
          FC_ASSERT( !ec, "error: ${e}", ("e",ec.message()) );
       }
       my->_client.connect(con);
       my->_connected->wait();
       return my->_connection;
   } FC_CAPTURE_AND_RETHROW( (uri) ) }

} } // fc::http
