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
//#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/database/database_interface.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/variant.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>

using namespace sophiatx;
using namespace sophiatx::chain;
using namespace sophiatx::protocol;

BOOST_FIXTURE_TEST_SUITE( serialization_tests, clean_database_fixture )

   /*
BOOST_AUTO_TEST_CASE( account_name_type_test )
{

   auto test = []( const string& data ) {
      fixed_string<> a(data);
      std::string    b(data);

      auto ap = fc::raw::pack( empty );
      auto bp = fc::raw::pack( emptystr );
      FC_ASSERT( ap.size() == bp.size() );
      FC_ASSERT( std::equal( ap.begin(), ap.end(), bp.begin() ) );

      auto sfa = fc::raw::unpack<std::string>( ap );
      auto afs = fc::raw::unpack<fixed_string<>>( bp );
   }
   test( std::string() );
   test( "helloworld" );
   test( "1234567890123456" );

   auto packed_long_string = fc::raw::pack( std::string( "12345678901234567890" ) );
   auto unpacked = fc::raw::unpack<fixed_string<>>( packed_long_string );
   idump( (unpacked) );
}*/


BOOST_AUTO_TEST_CASE( serialization_raw_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = AN("alice");
      op.to = AN("bob");
      op.amount = asset(100,SOPHIATX_SYMBOL);

      trx.operations.push_back( op );
      auto packed = fc::raw::pack_to_vector( trx );
      signed_transaction unpacked = fc::raw::unpack_from_vector<signed_transaction>(packed, 0);
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}
BOOST_AUTO_TEST_CASE( serialization_json_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = AN("alice");
      op.to = AN("bob");
      op.amount = asset(100,SOPHIATX_SYMBOL);

      fc::variant test(op.amount);
      auto tmp = test.as<asset>();
      BOOST_REQUIRE( tmp == op.amount );

      trx.operations.push_back( op );
      fc::variant packed(trx);
      signed_transaction unpacked = packed.as<signed_transaction>();
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( legacy_asset_test )
{
   try
   {
      using sophiatx::protocol::asset;

      BOOST_CHECK_EQUAL( asset().symbol.decimals(), 6 );
      BOOST_CHECK_EQUAL( asset().to_string(), "0.000000 SPHTX" );

      BOOST_TEST_MESSAGE( "Asset Test" );
      asset sophiatx = asset::from_string( "123.456000 SPHTX" );
      asset sbd = asset::from_string( "654.321000 USD" );
      asset tmp = asset::from_string( "0.456000 SPHTX" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 456000 );
      tmp = asset::from_string( "0.056000 SPHTX" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 56000 );

      BOOST_CHECK_EQUAL( sophiatx.amount.value, 123456000 );
      BOOST_CHECK_EQUAL( sophiatx.symbol.decimals(), 6 );
      BOOST_CHECK_EQUAL( sophiatx.to_string(), "123.456000 SPHTX" );
      BOOST_CHECK( sophiatx.symbol == SOPHIATX_SYMBOL );
      BOOST_CHECK_EQUAL( asset( asset( 50000, SOPHIATX_SYMBOL ) ).to_string(), "0.050000 SPHTX" );
      BOOST_CHECK_EQUAL( asset( asset(50000000, SOPHIATX_SYMBOL ) ) .to_string(), "50.000000 SPHTX" );

      BOOST_CHECK_EQUAL( sbd.amount.value, 654321000 );
      BOOST_CHECK_EQUAL( sbd.symbol.decimals(), 6 );
      BOOST_CHECK_EQUAL( sbd.to_string(), "654.321000 USD" );
      BOOST_CHECK( sbd.symbol == SBD1_SYMBOL );
      BOOST_CHECK_EQUAL( asset( asset(50000, SBD1_SYMBOL ) ).to_string(), "0.050000 USD" );
      BOOST_CHECK_EQUAL( asset( asset(50000000, SBD1_SYMBOL ) ).to_string(), "50.000000 USD" );

      BOOST_CHECK_THROW( asset::from_string( "1.00000000000000000000 SPHTX" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1.000SPHTX" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1. 333 SPHTX" ), fc::exception ); // Fails because symbol is '333 SPHTX', which is too long
      BOOST_CHECK_THROW( asset::from_string( "1 .333 SPHTX" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1 .333 X" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1 .333" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1 1.1" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "11111111111111111111111111111111111111111111111 SPHTX" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1.1.1 SPHTX" ), std::exception );
      BOOST_CHECK_THROW( asset::from_string( "1.abc SPHTX" ), std::exception );
      BOOST_CHECK_THROW( asset::from_string( " SPHTX" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "SPHTX" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1.333" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "1.333 " ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "" ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( " " ), fc::exception );
      BOOST_CHECK_THROW( asset::from_string( "  " ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

/*BOOST_AUTO_TEST_CASE( asset_test )
{
   try
   {
      std::string s;

      BOOST_CHECK_EQUAL( asset().symbol.decimals(), 6 );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset() ), "[\"0\",3,\"@@000000021\"]" );

      asset sophiatx = fc::json::from_string( "[\"123456\",    3, \"@@000000021\"]" ).as< asset >();
      asset sbd =   fc::json::from_string( "[\"654321\",    3, \"@@000000013\"]" ).as< asset >();
      asset vests = fc::json::from_string( "[\"123456789\", 6, \"@@000000037\"]" ).as< asset >();
      asset tmp =   fc::json::from_string( "[\"456\",       3, \"@@000000021\"]" ).as< asset >();
      BOOST_CHECK_EQUAL( tmp.amount.value, 456 );
      tmp = fc::json::from_string( "[\"56\", 3, \"@@000000021\"]" ).as< asset >();
      BOOST_CHECK_EQUAL( tmp.amount.value, 56 );

      BOOST_CHECK_EQUAL( sophiatx.amount.value, 123456 );
      BOOST_CHECK_EQUAL( sophiatx.symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( fc::json::to_string( sophiatx ), "[\"123456\",3,\"@@000000021\"]" );
      BOOST_CHECK( sophiatx.symbol == SOPHIATX_SYMBOL );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, SOPHIATX_SYMBOL ) ), "[\"50\",3,\"@@000000021\"]" );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, SOPHIATX_SYMBOL ) ), "[\"50000\",3,\"@@000000021\"]" );

      BOOST_CHECK_EQUAL( sbd.amount.value, 654321 );
      BOOST_CHECK_EQUAL( sbd.symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( fc::json::to_string( sbd ), "[\"654321\",3,\"@@000000013\"]" );
      BOOST_CHECK( sbd.symbol == SBD1_SYMBOL );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, SBD1_SYMBOL ) ), "[\"50\",3,\"@@000000013\"]" );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, SBD1_SYMBOL ) ), "[\"50000\",3,\"@@000000013\"]" );

      BOOST_CHECK_EQUAL( vests.amount.value, 123456789 );
      BOOST_CHECK_EQUAL( vests.symbol.decimals(), 6 );
      BOOST_CHECK_EQUAL( fc::json::to_string( vests ), "[\"123456789\",6,\"@@000000037\"]" );
      BOOST_CHECK( vests.symbol == VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, VESTS_SYMBOL ) ), "[\"50\",6,\"@@000000037\"]" );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, VESTS_SYMBOL ) ), "[\"50000\",6,\"@@000000037\"]" );

      // amount overflow
      BOOST_CHECK_THROW( fc::json::from_string( "[\"9223372036854775808\",3,\"@@000000021\"]" ).as< asset >(), fc::exception );
      // amount underflow
      BOOST_CHECK_THROW( fc::json::from_string( "[\"-1\",3,\"@@000000021\"]" ).as< asset >(), fc::exception );

      // precision overflow
      BOOST_CHECK_THROW( fc::json::from_string( "[\"10\",256,\"@@000000021\"]" ).as< asset >(), fc::exception );
      // precision underflow
      BOOST_CHECK_THROW( fc::json::from_string( "[\"10\",-1,\"@@000000021\"]" ).as< asset >(), fc::exception );

      // Check wrong size tuple
      BOOST_CHECK_THROW( fc::json::from_string( "[\"0\",3]" ).as< asset >(), fc::exception );
      BOOST_CHECK_THROW( fc::json::from_string( "[\"0\",3,\"@@000000021\",1]" ).as< asset >(), fc::exception );

      // Check non-numeric characters in amount
      BOOST_CHECK_THROW( fc::json::from_string( "[\"foobar\",3,\"@@000000021\"]" ).as< asset >(), fc::exception );
      BOOST_CHECK_THROW( fc::json::from_string( "[\"10a\",3,\"@@000000021\"]" ).as< asset >(), fc::exception );
      BOOST_CHECK_THROW( fc::json::from_string( "[\"10a00\",3,\"@@000000021\"]" ).as< asset >(), fc::exception );

      // Check hex value
      BOOST_CHECK_THROW( fc::json::from_string( "[\"0x8000\",3,\"@@000000021\"]" ).as< asset >(), fc::exception );

      // Check octal value
      BOOST_CHECK_EQUAL( fc::json::from_string( "[\"08000\",3,\"@@000000021\"]" ).as< asset >().amount.value, 8000 );
   }
   FC_LOG_AND_RETHROW()
}*/

template< typename T >
std::string hex_bytes( const T& obj )
{
   std::vector<char> data = fc::raw::pack_to_vector( obj );
   std::ostringstream ss;
   static const char hexdigits[] = "0123456789abcdef";

   for( char c : data )
   {
      ss << hexdigits[((c >> 4) & 0x0F)] << hexdigits[c & 0x0F] << ' ';
   }
   return ss.str();
}

void old_pack_symbol(vector<char>& v, asset_symbol_type sym)
{
   if( sym == SOPHIATX_SYMBOL )
   {
      v.push_back('S' ); v.push_back('P' ); v.push_back('H' );
      v.push_back('T'   ); v.push_back('X' ); v.push_back('\0'); v.push_back('\0'); v.push_back('\0');
      // 03 54 45 53 54 53 00 00
   }
   else if( sym == SBD1_SYMBOL )
   {
      v.push_back('U' ); v.push_back('S' ); v.push_back('D' ); v.push_back('\0'  );
      v.push_back('\0'  ); v.push_back('\0'); v.push_back('\0'); v.push_back('\0');
      // 03 54 42 44 00 00 00 00
   }
   else if( sym == VESTS_SYMBOL )
   {
      v.push_back('V' ); v.push_back('E' ); v.push_back('S' );
      v.push_back('T'   ); v.push_back('S' ); v.push_back('\0'); v.push_back('\0'); v.push_back('\0');
      // 06 56 45 53 54 53 00 00
   }
   else
   {
      FC_ASSERT( false, "This method cannot serialize this symbol" );
   }
   return;
}

void old_pack_asset( vector<char>& v, const asset& a )
{
   uint64_t x = uint64_t( a.amount.value );
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );
   old_pack_symbol( v, a.symbol );
   return;
}

std::string old_json_asset( const asset& a )
{
   size_t decimal_places = 0;
   if( (a.symbol == SOPHIATX_SYMBOL) || (a.symbol == SBD1_SYMBOL) )
      decimal_places = 3;
   else if( a.symbol == VESTS_SYMBOL )
      decimal_places = 6;
   std::ostringstream ss;
   ss << std::setfill('0') << std::setw(decimal_places+1) << a.amount.value;
   std::string result = ss.str();
   result.insert( result.length() - decimal_places, 1, '.' );
   if( a.symbol == SOPHIATX_SYMBOL )
      result += " SPHTX";
   else if( a.symbol == SBD1_SYMBOL )
      result += " USD";
   else if( a.symbol == VESTS_SYMBOL )
      result += " VESTS";
   result.insert(0, 1, '"');
   result += '"';
   return result;
}

BOOST_AUTO_TEST_CASE( asset_raw_test )
{
   try
   {
      // get a bunch of random bits
      fc::sha256 h = fc::sha256::hash("");

      std::vector< share_type > amounts;

      for( int i=0; i<64; i++ )
      {
         uint64_t s = (uint64_t(1) << i);
         uint64_t x = (h._hash[0] & (s-1)) | s;
         if( x >= SOPHIATX_MAX_SHARE_SUPPLY )
            break;
         amounts.push_back( share_type( x ) );
      }
      // ilog( "h0:${h0}", ("h0", h._hash[0]) );

/*      asset sophiatx = asset::from_string( "0.001 SPHTX" );
#define VESTS_SYMBOL  (uint64_t(6) | (uint64_t('V') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< VESTS with 6 digits of precision
#define SOPHIATX_SYMBOL  (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< SOPHIATX with 3 digits of precision
#define SBD1_SYMBOL    (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('B') << 16) | (uint64_t('D') << 24) ) ///< Test Backed Dollars with 3 digits of precision
*/
      std::vector< asset_symbol_type > symbols;

      symbols.push_back( SOPHIATX_SYMBOL );
      symbols.push_back( SBD1_SYMBOL );
      symbols.push_back( VESTS_SYMBOL );

      for( const share_type& amount : amounts )
      {
         for( const asset_symbol_type& symbol : symbols )
         {
            // check raw::pack() works
            asset a = asset( amount, symbol );
            vector<char> v_old;
            old_pack_asset( v_old, a );
            vector<char> v_cur = fc::raw::pack_to_vector(a);
            // ilog( "${a} : ${d}", ("a", a)("d", hex_bytes( v_old )) );
            // ilog( "${a} : ${d}", ("a", a)("d", hex_bytes( v_cur )) );
            BOOST_CHECK( v_cur == v_old );

            // check raw::unpack() works
            std::istringstream ss( string(v_cur.begin(), v_cur.end()) );
            asset a2;
            fc::raw::unpack( ss, a2, 0 );
            BOOST_CHECK( a == a2 );

            // check conversion to JSON works
            //std::string json_old = old_json_asset(a);
            //std::string json_cur = fc::json::to_string(a);
            // ilog( "json_old: ${j}", ("j", json_old) );
            // ilog( "json_cur: ${j}", ("j", json_cur) );
            //BOOST_CHECK( json_cur == json_old );

            // check JSON serialization is symmetric
            std::string json_cur = fc::json::to_string(a);
            a2 = fc::json::from_string(json_cur).as< asset >();
            BOOST_CHECK( a == a2 );
         }
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( json_tests )
{
   try {
   auto var = fc::json::variants_from_string( "10.6 " );
   var = fc::json::variants_from_string( "10.5" );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_private_key_type_test )
{
   try
   {
     fc::ecc::extended_private_key key = fc::ecc::extended_private_key( fc::ecc::private_key::generate(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_private_key_type type = extended_private_key_type( key );
      std::string packed = std::string( type );
      extended_private_key_type unpacked = extended_private_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_public_key_type_test )
{
   try
   {
      fc::ecc::extended_public_key key = fc::ecc::extended_public_key( fc::ecc::private_key::generate().get_public_key(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_public_key_type type = extended_public_key_type( key );
      std::string packed = std::string( type );
      extended_public_key_type unpacked = extended_public_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( version_test )
{
   try
   {
      BOOST_REQUIRE_EQUAL( string( version( 1, 2, 3) ), "1.2.3" );

      fc::variant ver_str( "3.0.0" );
      version ver;
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 3, 0 , 0 ) );

      ver_str = fc::variant( "0.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version() );

      ver_str = fc::variant( "1.0.1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 1, 0, 1 ) );

      ver_str = fc::variant( "1_0_1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 1, 0, 1 ) );

      ver_str = fc::variant( "12.34.56" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 12, 34, 56 ) );

      ver_str = fc::variant( "256.0.0" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.256.0" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.0.65536" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "1.0" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "1.0.0.1" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( hardfork_version_test )
{
   try
   {
      BOOST_REQUIRE_EQUAL( string( hardfork_version( 1, 2 ) ), "1.2.0" );

      fc::variant ver_str( "3.0.0" );
      hardfork_version ver;
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 3, 0 ) );

      ver_str = fc::variant( "0.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version() );

      ver_str = fc::variant( "1.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );

      ver_str = fc::variant( "1_0_0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );

      ver_str = fc::variant( "12.34.00" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 12, 34 ) );

      ver_str = fc::variant( "256.0.0" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.256.0" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.0.1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 0, 0 ) );

      ver_str = fc::variant( "1.0" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "1.0.0.1" );
      SOPHIATX_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( min_block_size )
{
   signed_block b;
   while( b.witness.length() < SOPHIATX_MIN_ACCOUNT_NAME_LENGTH )
      b.witness += 'a';
   size_t min_size = fc::raw::pack_size( b );
   BOOST_CHECK( min_size == SOPHIATX_MIN_BLOCK_SIZE );
}

BOOST_AUTO_TEST_CASE( legacy_signed_transaction )
{
   using sophiatx::protocol::signed_transaction;

   signed_transaction tx;
   transfer_operation op;
   op.from = AN("alice");
   op.to = AN("bob");
   op.amount = asset( 50, SOPHIATX_SYMBOL );
   tx.ref_block_num = 4000;
   tx.ref_block_prefix = 4000000000;
   tx.expiration = fc::time_point_sec( 1514764800 );
   tx.operations.push_back( op );

   signed_transaction tx2 = signed_transaction( fc::json::from_string( "{\"ref_block_num\":4000,\"ref_block_prefix\":4000000000,\"expiration\":\"2018-01-01T00:00:00\",\"operations\":[[\"transfer\",{\"from\":\"nbyckkPfkdV4OQtmdTh93QjB5CIA\",\"to\":\"8I1Pt6T2ovPJgEhTaMPP3Qv4uCoA\",\"amount\":\"0.000050 SPHTX\"}]],\"extensions\":[],\"signatures\":[\"\"]}" ).as< signed_transaction >() );

   BOOST_REQUIRE( tx.id() == tx2.id() );
}

BOOST_AUTO_TEST_SUITE_END()
//#endif
