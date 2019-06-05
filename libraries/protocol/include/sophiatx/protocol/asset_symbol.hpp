#pragma once

#include <fc/io/raw.hpp>
#include <sophiatx/protocol/types_fwd.hpp>
#include <sophiatx/protocol/config.hpp>

namespace sophiatx { namespace protocol {

  class asset_symbol_type
  {
  public:
     uint64_t value = SOPHIATX_SYMBOL_SER;
  public:
     asset_symbol_type() {};
     asset_symbol_type(uint64_t v): value(v) {}
     asset_symbol_type(const asset_symbol_type& as){value = as.value;}

     uint8_t decimals()const{ return SOPHIATX_DECIMALS; };


     static asset_symbol_type from_string( const std::string& str );

     std::string to_string()const;


     friend bool operator == ( const asset_symbol_type& a, const asset_symbol_type& b )
     {  return (a.value == b.value);   }
     friend bool operator != ( const asset_symbol_type& a, const asset_symbol_type& b )
     {  return (a.value != b.value);   }
     friend bool operator < ( const asset_symbol_type& a, const asset_symbol_type& b )
     {  return (a.value < b.value);    }
  };

} } // sophiatx::protocol

namespace fc {
void to_variant( const sophiatx::protocol::asset_symbol_type& var,  fc::variant& vo );
void from_variant( const fc::variant& var,  sophiatx::protocol::asset_symbol_type& vo );
}

FC_REFLECT( sophiatx::protocol::asset_symbol_type, (value) )
