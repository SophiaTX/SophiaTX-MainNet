#include <boost/test/unit_test.hpp>
#include <fc/network/tcp_socket.hpp>
BOOST_AUTO_TEST_SUITE(tcp_tests)
/***
 * Running this test through valgrind will show
 * a memory leak due to lack of logic in destructor.
 */
BOOST_AUTO_TEST_CASE(tcpconstructor_test)
{
   fc::tcp_socket socket;
}
BOOST_AUTO_TEST_SUITE_END()
