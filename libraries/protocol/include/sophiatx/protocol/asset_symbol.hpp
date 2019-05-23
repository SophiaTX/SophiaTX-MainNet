#pragma once

#include <fc/io/raw.hpp>
#include <sophiatx/protocol/types_fwd.hpp>
#include <sophiatx/protocol/config.hpp>

#define VESTS_SYMBOL_U64  (uint64_t('V') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define SOPHIATX_SYMBOL_U64  (uint64_t('S') | (uint64_t('P') << 8) | (uint64_t('H') << 16) | (uint64_t('T') << 24) | (uint64_t('X') << 32))
#define SBD1_SYMBOL_U64    (uint64_t('U') | (uint64_t('S') << 8) | (uint64_t('D') << 16) )
#define SBD2_SYMBOL_U64    (uint64_t('E') | (uint64_t('U') << 8) | (uint64_t('R') << 16) )
#define SBD3_SYMBOL_U64    (uint64_t('C') | (uint64_t('H') << 8) | (uint64_t('F') << 16) )
#define SBD4_SYMBOL_U64    (uint64_t('C') | (uint64_t('N') << 8) | (uint64_t('Y') << 16) )
#define SBD5_SYMBOL_U64    (uint64_t('G') | (uint64_t('B') << 8) | (uint64_t('P') << 16) )


#define VESTS_SYMBOL_SER  (VESTS_SYMBOL_U64) ///< VESTS|VESTS with 6 digits of precision
#define SOPHIATX_SYMBOL_SER  (SOPHIATX_SYMBOL_U64) ///< SPHTX|TESTS with 6 digits of precision
#define SBD1_SYMBOL_SER    (SBD1_SYMBOL_U64)  
#define SBD2_SYMBOL_SER    (SBD2_SYMBOL_U64)  
#define SBD3_SYMBOL_SER    (SBD3_SYMBOL_U64) 
#define SBD4_SYMBOL_SER    (SBD4_SYMBOL_U64) 
#define SBD5_SYMBOL_SER    (SBD5_SYMBOL_U64)

#define SOPHIATX_ASSET_MAX_DECIMALS 6

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
