#pragma once

#include <fc/variant.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <fc/io/raw_fwd.hpp>


namespace fc {

   namespace bip = boost::interprocess;

    template<typename... T >
    void to_variant( const bip::deque< T... >& t, fc::variant& v ) {
      std::vector<variant> vars(t.size());
      for( size_t i = 0; i < t.size(); ++i ) {
         vars[i] = t[i];
      }
      v = std::move(vars);
    }

    template<typename T, typename... A>
    void from_variant( const fc::variant& v, bip::deque< T, A... >& d ) {
      const variants& vars = v.get_array();
      d.clear();
      d.resize( vars.size() );
      for( uint32_t i = 0; i < vars.size(); ++i ) {
         from_variant( vars[i], d[i] );
      }
    }

    /*  bip::flat_map == boost::flat_map
    template<typename K, typename V, typename... T >
    void to_variant( const bip::flat_map< K, V, T... >& var, fc::variant& vo ) {
       std::vector< variant > vars(var.size());
       size_t i = 0;
       for( auto itr = var.begin(); itr != var.end(); ++itr, ++i )
          vars[i] = fc::variant(*itr);
       vo = vars;
    }

    template<typename K, typename V, typename... A>
    void from_variant( const variant& var,  bip::flat_map<K, V, A...>& vo )
    {
       const variants& vars = var.get_array();
       vo.clear();
       for( auto itr = vars.begin(); itr != vars.end(); ++itr )
          vo.insert( itr->as< std::pair<K,V> >() );
    }
    */

    template<typename... T >
    void to_variant( const bip::vector< T... >& t, fc::variant& v ) {
      std::vector<variant> vars(t.size());
      for( size_t i = 0; i < t.size(); ++i ) {
         vars[i] = t[i];
      }
      v = std::move(vars);
    }

    template<typename T, typename... A>
    void from_variant( const fc::variant& v, bip::vector< T, A... >& d ) {
      const variants& vars = v.get_array();
      d.clear();
      d.resize( vars.size() );
      for( uint32_t i = 0; i < vars.size(); ++i ) {
         from_variant( vars[i], d[i] );
      }
    }

   namespace raw {
       namespace bip = boost::interprocess;

       template<typename Stream, typename T, typename... A>
       inline void pack( Stream& s, const bip::vector<T,A...>& value ) {
         pack( s, unsigned_int((uint32_t)value.size()) );
         auto itr = value.begin();
         auto end = value.end();
         while( itr != end ) {
           fc::raw::pack( s, *itr );
           ++itr;
         }
       }
       template<typename Stream, typename T, typename... A>
       inline void unpack( Stream& s, bip::vector<T,A...>& value ) {
         unsigned_int size;
         unpack( s, size );
         value.clear(); value.resize(size);
         for( auto& item : value )
             fc::raw::unpack( s, item );
       }
   }
}
