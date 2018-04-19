#pragma once

#include <fc/io/raw.hpp>
#include <steem/protocol/types_fwd.hpp>


#ifdef IS_TEST_NET

#define VESTS_SYMBOL_U64  (uint64_t('V') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define STEEM_SYMBOL_U64  (uint64_t('T') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define SBD_SYMBOL_U64    (uint64_t('T') | (uint64_t('B') << 8) | (uint64_t('D') << 16))

#else

#define VESTS_SYMBOL_U64  (uint64_t('V') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define STEEM_SYMBOL_U64  (uint64_t('S') | (uint64_t('P') << 8) | (uint64_t('H') << 16) | (uint64_t('T') << 24) | (uint64_t('X') << 32))
#define SBD_SYMBOL_U64    (uint64_t('S') | (uint64_t('B') << 8) | (uint64_t('D') << 16))

#endif

#define VESTS_SYMBOL_SER  (uint64_t(6) | (VESTS_SYMBOL_U64 << 8)) ///< VESTS|VESTS with 6 digits of precision
#define STEEM_SYMBOL_SER  (uint64_t(6) | (STEEM_SYMBOL_U64 << 8)) ///< SPHTX|TESTS with 6 digits of precision
#define SBD_SYMBOL_SER    (uint64_t(6) |   (SBD_SYMBOL_U64 << 8)) ///< SBD|TBD with 4 digits of precision

#define STEEM_ASSET_MAX_DECIMALS 12

namespace steem { namespace protocol {

  class asset_symbol_type
  {
  public:
     uint64_t value = STEEM_SYMBOL_SER;
  public:
     asset_symbol_type() {}
     asset_symbol_type(uint64_t v): value(v) {}
     asset_symbol_type(const asset_symbol_type& as){value = as.value;}


     static asset_symbol_type from_string( const std::string& str ){
       FC_ASSERT((str.size() >= 3 && str.size() <= 6), "invalid symbol length");
       const char* c_str = str.c_str();
       uint64_t ret;
       int i =0;
       while( c_str[i] ){
          ret = (ret << 8) | uint64_t(c_str[i]);
          i++;
       }
       asset_symbol_type rv (ret <<8 & (uint64_t(6))) ;
       return rv;
     }

     static asset_symbol_type from_string( const std::string& str, uint decimals ){
       FC_ASSERT((str.size() >= 3 && str.size() <= 6), "invalid symbol length");
       FC_ASSERT(STEEM_ASSET_MAX_DECIMALS>= decimals);
       const char* c_str = str.c_str();
       uint64_t ret;
       int i =0;
       while( c_str[i] ){
         ret = (ret << 8) | uint64_t(c_str[i]);
         i++;
       }

        asset_symbol_type rv (ret <<8 & (uint64_t(decimals))) ;
        return rv;
     }

     std::string to_string()const{
       std::string ret;
       uint64_t symbol = value >>8;
       while ( symbol ) {
         ret.push_back(symbol & (uint64_t(255)));
         symbol = symbol >> 8;
       }
       FC_ASSERT((ret.size() >= 3), "invalid symbol length");
       return ret;
     }

     uint8_t decimals()const{ uint64_t dec = value & (uint64_t(255)); return (uint8_t)dec; };

     friend bool operator == ( const asset_symbol_type& a, const asset_symbol_type& b )
     {  return (a.value == b.value);   }
     friend bool operator != ( const asset_symbol_type& a, const asset_symbol_type& b )
     {  return (a.value != b.value);   }
  };
/*
class asset_symbol_type
{
   public:

      asset_symbol_type() {}

      // buf must have space for STEEM_ASSET_SYMBOL_MAX_LENGTH+1
      static asset_symbol_type from_string( const std::string& str );
      static asset_symbol_type from_asset_num( uint32_t asset_num )
      {   asset_symbol_type result;   result.asset_num = asset_num;   return result;   }

      std::string to_string()const;

      //Returns true when symbol represents vesting variant of the token, false for liquid one.
      bool is_vesting() const;
      //Returns vesting symbol when called from liquid one and liquid symbol when called from vesting one. Returns back the SBD symbol if represents SBD.

      asset_symbol_type get_paired_symbol() const;

      uint8_t decimals()const
      {  return uint8_t( asset_num & 0x0F );    }
      void validate()const;

      friend bool operator == ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num == b.asset_num);   }
      friend bool operator != ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num != b.asset_num);   }
      friend bool operator <  ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num <  b.asset_num);   }
      friend bool operator >  ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num >  b.asset_num);   }
      friend bool operator <= ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num <= b.asset_num);   }
      friend bool operator >= ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num >= b.asset_num);   }

      uint32_t asset_num = 0;
};
*/
} } // steem::protocol

FC_REFLECT(steem::protocol::asset_symbol_type, (value))

/*
FC_REFLECT(steem::protocol::asset_symbol_type, (asset_num))

namespace fc { namespace raw {

template< typename Stream >
inline void pack( Stream& s, const steem::protocol::asset_symbol_type& sym )
{
   switch( sym.space() )
   {
      case steem::protocol::asset_symbol_type::legacy_space:
      {
         uint64_t ser = 0;
         switch( sym.asset_num )
         {
            case STEEM_ASSET_NUM_STEEM:
               ser = STEEM_SYMBOL_SER;
               break;
            case STEEM_ASSET_NUM_SBD:
               ser = SBD_SYMBOL_SER;
               break;
            case STEEM_ASSET_NUM_VESTS:
               ser = VESTS_SYMBOL_SER;
               break;
            default:
               FC_ASSERT( false, "Cannot serialize unknown asset symbol" );
         }
         pack( s, ser );
         break;
      }
      case steem::protocol::asset_symbol_type::smt_nai_space:
         pack( s, sym.asset_num );
         break;
      default:
         FC_ASSERT( false, "Cannot serialize unknown asset symbol" );
   }
}

template< typename Stream >
inline void unpack( Stream& s, steem::protocol::asset_symbol_type& sym )
{
   uint64_t ser = 0;
   s.read( (char*) &ser, 4 );

   switch( ser )
   {
      case STEEM_SYMBOL_SER & 0xFFFFFFFF:
         s.read( ((char*) &ser)+4, 4 );
         FC_ASSERT( ser == STEEM_SYMBOL_SER, "invalid asset bits" );
         sym.asset_num = STEEM_ASSET_NUM_STEEM;
         break;
      case SBD_SYMBOL_SER & 0xFFFFFFFF:
         s.read( ((char*) &ser)+4, 4 );
         FC_ASSERT( ser == SBD_SYMBOL_SER, "invalid asset bits" );
         sym.asset_num = STEEM_ASSET_NUM_SBD;
         break;
      case VESTS_SYMBOL_SER & 0xFFFFFFFF:
         s.read( ((char*) &ser)+4, 4 );
         FC_ASSERT( ser == VESTS_SYMBOL_SER, "invalid asset bits" );
         sym.asset_num = STEEM_ASSET_NUM_VESTS;
         break;
      default:
         sym.asset_num = uint32_t( ser );
   }
   sym.validate();
}

} // fc::raw

inline void to_variant( const steem::protocol::asset_symbol_type& sym, fc::variant& var )
{
   try
   {
      std::vector< variant > v( 2 );
      v[0] = sym.decimals();
      v[1] = sym.to_nai_string();
   } FC_CAPTURE_AND_RETHROW()
}

inline void from_variant( const fc::variant& var, steem::protocol::asset_symbol_type& sym )
{
   try
   {
      auto v = var.as< std::vector< variant > >();
      FC_ASSERT( v.size() == 2, "Expected tuple of length 2." );

      sym = steem::protocol::asset_symbol_type::from_nai_string( v[1].as< std::string >().c_str(), v[0].as< uint8_t >() );
   } FC_CAPTURE_AND_RETHROW()
}

} // fc
*/