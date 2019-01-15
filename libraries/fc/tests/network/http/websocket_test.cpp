#include <boost/test/unit_test.hpp>

#include <fc/network/http/websocket.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(fc_network)

BOOST_AUTO_TEST_CASE(websocket_test)
{ 
    fc::http::websocket_client client;
    fc::http::websocket_connection_ptr s_conn, c_conn;
    {
        fc::http::websocket_server server;
        server.on_connection([&]( const fc::http::websocket_connection_ptr& c ){
                s_conn = c;
                c->on_message_handler([&](const std::string& s){
                    std::cout <<"server received message s: " <<s <<"\n";
                    c->send_message("echo: " + s);
                });
            });

        server.listen( 54321 );
        server.start_accept();

        std::string echo;
        c_conn = client.connect( "ws://localhost:54321" );
        c_conn->on_message_handler([&](const std::string& s){
                    std::cout <<"client received message s: " <<s <<"\n";
                    echo = s;
                });
        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: hello world", echo);
        c_conn->send_message( "again" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: again", echo);

        s_conn->close(0, "test");
        fc::usleep( fc::seconds(1) );
        try {
            c_conn->send_message( "again" );
            BOOST_FAIL("expected assertion failure");
        } catch (const fc::assert_exception& e) {
            //std::cerr << e.to_string() << "\n";
        }

        c_conn = client.connect( "ws://localhost:54321" );
        c_conn->on_message_handler([&](const std::string& s){
                    std::cout <<"client2 received message s: " <<s <<"\n";
                    echo = s;
                });
        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: hello world", echo);
    }

    try {
        c_conn->send_message( "again" );
        BOOST_FAIL("expected assertion failure");
    } catch (const fc::assert_exception& e) {
        std::cerr << "Expected assertion failure : " << e.to_string() << "\n";
    } catch (const fc::exception& e) {
        BOOST_FAIL("Unexpected exception : " + e.to_string());
    } catch (const std::exception& e) {
        BOOST_FAIL("Unexpected exception : " + std::string(e.what()));
    }

    try {
        c_conn = client.connect( "ws://localhost:54321" );
        BOOST_FAIL("expected fc::exception (fail to connect)");
    } catch (const fc::exception& e) {
        std::cerr << "Excepted fc::exception : " << e.to_string() << "\n";
    } catch (const std::exception& e) {
        BOOST_FAIL("Unexpected exception : " + std::string(e.what()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
