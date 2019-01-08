#pragma once
#include <stdint.h>

namespace fc {
class uint128;
class variant;
} // fc

namespace sophiatx { namespace protocol {
template< typename Storage = fc::uint128 >
class fixed_string_impl;

class asset_symbol_type;
class legacy_sophiatx_asset_symbol_type;
struct legacy_sophiatx_asset;
} } // sophiatx::protocol

namespace fc { namespace raw {

template<typename Stream>
inline void pack( Stream& s, const uint128& u );
template<typename Stream>
inline void unpack( Stream& s, uint128& u, uint32_t depth );

template< typename Stream, typename Storage >
inline void pack( Stream& s, const sophiatx::protocol::fixed_string_impl< Storage >& u );
template< typename Stream, typename Storage >
inline void unpack( Stream& s, sophiatx::protocol::fixed_string_impl< Storage >& u, uint32_t depth );

/*template< typename Stream >
inline void pack( Stream& s, const sophiatx::protocol::asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, sophiatx::protocol::asset_symbol_type& sym );
*/

template< typename Stream >
inline void pack( Stream& s, const sophiatx::protocol::legacy_sophiatx_asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, sophiatx::protocol::legacy_sophiatx_asset_symbol_type& sym, uint32_t depth );

} // raw

template< typename Storage >
inline void to_variant( const sophiatx::protocol::fixed_string_impl< Storage >& s, fc::variant& v );
template< typename Storage >
inline void from_variant( const variant& v, sophiatx::protocol::fixed_string_impl< Storage >& s );

//inline void to_variant( const sophiatx::protocol::asset_symbol_type& sym, fc::variant& v );

inline void from_variant( const fc::variant& v, sophiatx::protocol::legacy_sophiatx_asset& leg );
inline void to_variant( const sophiatx::protocol::legacy_sophiatx_asset& leg, fc::variant& v );

} // fc
