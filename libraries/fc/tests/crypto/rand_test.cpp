#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fc/crypto/rand.hpp>

#include <cmath>

static void check_randomness( const char* buffer, size_t len ) {
    if (len == 0) { return; }
    // count bit runs and 0's / 1's
    unsigned int zc = 0, oc = 0, rc = 0, last = 2;
    for (size_t k = len; k; k--) {
        char c = *buffer++;
        for (int i = 0; i < 8; i++) {
            unsigned int bit = c & 1;
            c >>= 1;
            if (bit) { oc++; } else { zc++; }
            if (bit != last) { rc++; last = bit; }
        }
    }
    BOOST_CHECK_EQUAL( 8*len, zc + oc );
    double E = 1 + (zc + oc) / 2.0;
    double variance = (E - 1) * (E - 2) / (oc + zc - 1);
    double sigma = sqrt(variance);
    std::cout << "rc :"<< rc <<"; E: "<< E <<"; sigma: " <<sigma <<"\n";
    BOOST_CHECK( rc > E - 2* sigma && rc < E + 2* sigma);
}

BOOST_AUTO_TEST_SUITE(fc_crypto)

BOOST_AUTO_TEST_CASE(rand_test)
{
    char buffer[81920];
    fc::rand_bytes( buffer, sizeof(buffer) );
    std::cout <<"\n";
    check_randomness( buffer, sizeof(buffer) );
}

BOOST_AUTO_TEST_SUITE_END()
