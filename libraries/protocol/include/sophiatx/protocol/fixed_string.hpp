#pragma once

#include <fc/uint128.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/base64.hpp>

#include <fc/crypto/ripemd160.hpp>

#include <boost/endian/conversion.hpp>

#include <sophiatx/protocol/types_fwd.hpp>

// These overloads need to be defined before the implementation in fixed_string
namespace fc
{
   /**
    * Endian-reversible pair class.
    */

   template< typename A, typename B >
   struct erpair
   {
      erpair() {}
      erpair(const A& a, const B& b)
         : first(a), second(b) {}
      friend bool operator <  ( const erpair& a, const erpair& b )
      { return std::tie( a.first, a.second ) <  std::tie( b.first, b.second ); }
      friend bool operator <= ( const erpair& a, const erpair& b )
      { return std::tie( a.first, a.second ) <= std::tie( b.first, b.second ); }
      friend bool operator >  ( const erpair& a, const erpair& b )
      { return std::tie( a.first, a.second ) >  std::tie( b.first, b.second ); }
      friend bool operator >= ( const erpair& a, const erpair& b )
      { return std::tie( a.first, a.second ) >= std::tie( b.first, b.second ); }
      friend bool operator == ( const erpair& a, const erpair& b )
      { return std::tie( a.first, a.second ) == std::tie( b.first, b.second ); }
      friend bool operator != ( const erpair& a, const erpair& b )
      { return std::tie( a.first, a.second ) != std::tie( b.first, b.second ); }

      A first{};
      B second{};
   };

   template< typename A, typename B >
   erpair<A, B> make_erpair( const A& a, const B& b )
   {  return erpair<A, B>(a, b); }

   template< typename T >
   T endian_reverse( const T& x )
   {  return boost::endian::endian_reverse(x);  }

   template<>
   inline uint128 endian_reverse( const uint128& u )
   {  return uint128( boost::endian::endian_reverse( u.hi ), boost::endian::endian_reverse( u.lo ) );  }

   template<typename A, typename B>
   erpair< A, B > endian_reverse( const erpair< A, B >& p )
   {
      return make_erpair( endian_reverse( p.first ), endian_reverse( p.second ) );
   }
}


namespace sophiatx { namespace protocol {

/**
 * This class is an in-place memory allocation of a fixed length character string.
 *
 * The string will serialize the same way as std::string for variant and raw formats.
 */
template< typename Storage >
class fixed_string_impl
{
   public:
      fixed_string_impl(){}
      fixed_string_impl( const fixed_string_impl& c ) : data( c.data ), _size (c._size){}
      fixed_string_impl( const char* str ) : fixed_string_impl( std::string( str ) ) {}
      fixed_string_impl( const std::string& str )
      {
         Storage d;

         /*char bytes[fc::ripemd160::data_size()];
         auto count = fc::from_base58(str, bytes, fc::ripemd160::data_size());

         if( count <= sizeof(d) )
            _size = count;
         else
            _size = sizeof(d);
         memcpy( (char*)&d, bytes, _size );

         data = boost::endian::big_to_native( d );*/


         std::string s = fc::base64_decode(str);
         int count = s.size();
         if( count <= sizeof(d) )
            _size = count;
         else
            _size = sizeof(d);
         memcpy( (char*)&d, s.c_str(), _size );
         data = boost::endian::big_to_native( d );

      }

      operator std::string()const
      {
         Storage d = boost::endian::native_to_big( data );

         /*std::vector<char> _self ((const char*)&d, (const char*)&d +  _size);

         std::string ret = fc::to_base58(_self);
         return ret;*/


         std::string s = fc::base64_encode((char*)&d, _size);
         return s;
      }

      uint32_t size()const
      {
         return _size;

      }

      uint32_t length()const { return size(); }

      fixed_string_impl& operator = ( const fixed_string_impl& str )
      {
         data = str.data;
         _size = str._size;
         return *this;
      }

      fixed_string_impl& operator = ( const char* str )
      {
         *this = fixed_string_impl( str );
         return *this;
      }

      fixed_string_impl& operator = ( const std::string& str )
      {
         *this = fixed_string_impl( str );
         return *this;
      }

      friend std::string operator + ( const fixed_string_impl& a, const std::string& b ) { return std::string( a ) + b; }
      friend std::string operator + ( const std::string& a, const fixed_string_impl& b ){ return a + std::string( b ); }
      friend bool operator < ( const fixed_string_impl& a, const fixed_string_impl& b ) { return a.data < b.data; }
      friend bool operator <= ( const fixed_string_impl& a, const fixed_string_impl& b ) { return a.data <= b.data; }
      friend bool operator > ( const fixed_string_impl& a, const fixed_string_impl& b ) { return a.data > b.data; }
      friend bool operator >= ( const fixed_string_impl& a, const fixed_string_impl& b ) { return a.data >= b.data; }
      friend bool operator == ( const fixed_string_impl& a, const fixed_string_impl& b ) { return a.data == b.data; }
      friend bool operator != ( const fixed_string_impl& a, const fixed_string_impl& b ) { return a.data != b.data; }

      Storage data;
      uint32_t _size=0;

};

// These storage types work with memory layout and should be used instead of a custom template.
template< size_t N >
struct fixed_string_impl_for_size;

template<>
struct fixed_string_impl_for_size< 16 >
{
   typedef fixed_string_impl< fc::uint128_t >                               t;
};

template<>
struct fixed_string_impl_for_size< 24 >
{
   typedef fixed_string_impl< fc::erpair< fc::uint128_t, uint64_t > >       t;
};

template<>
struct fixed_string_impl_for_size< 32 >
{
   typedef fixed_string_impl< fc::erpair< fc::uint128_t, fc::uint128_t > >  t;
};

template< size_t N >
using fixed_string = typename fixed_string_impl_for_size<N>::t;

} } // sophiatx::protocol

namespace fc { namespace raw {

   template< typename Stream, typename Storage >
   inline void pack( Stream& s, const sophiatx::protocol::fixed_string_impl< Storage >& u )
   {
      pack( s, std::string( u ) );
   }

   template< typename Stream, typename Storage >
   inline void unpack( Stream& s, sophiatx::protocol::fixed_string_impl< Storage >& u )
   {
      std::string str;
      unpack( s, str );
      u = str;
   }

} // raw
   template< typename Storage >
   void to_variant(   const sophiatx::protocol::fixed_string_impl< Storage >& s, variant& v ) { v = std::string( s ); }

   template< typename Storage >
   void from_variant( const variant& v, sophiatx::protocol::fixed_string_impl< Storage >& s ) { s = v.as_string(); }
} // fc
