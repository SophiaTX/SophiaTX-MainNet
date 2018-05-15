#include <steem/protocol/asset.hpp>

#include <fc/io/json.hpp>

#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>



namespace steem { namespace protocol {

//1USD/5SPHTX   10SPHTX/2USD
#define BQ(a) \
   std::tie( a.base.symbol, a.quote.symbol )

#define DEFINE_PRICE_COMPARISON_OPERATOR( op ) \
bool operator op ( const price& a, const price& b ) \
{ \
   price a_(a.quote, a.base) ;\
   if( BQ(a) != BQ(b) ){ \
     if( BQ(a_) != BQ(b) ){ \
       return false; \
     }else{ \
       return a_ op b;\
     } \
   } \
   const uint128_t amult = uint128_t( b.quote.amount.value ) * a.base.amount.value; \
   const uint128_t bmult = uint128_t( a.quote.amount.value ) * b.base.amount.value; \
   \
   return amult op bmult;  \
}

DEFINE_PRICE_COMPARISON_OPERATOR( == )
DEFINE_PRICE_COMPARISON_OPERATOR( != )
DEFINE_PRICE_COMPARISON_OPERATOR( <  )
DEFINE_PRICE_COMPARISON_OPERATOR( <= )
DEFINE_PRICE_COMPARISON_OPERATOR( >  )
DEFINE_PRICE_COMPARISON_OPERATOR( >= )

      asset operator * ( const asset& a, const price& b )
      {
         if( a.symbol == b.base.symbol )
         {
            FC_ASSERT( b.base.amount.value > 0 );
            uint128_t result = (uint128_t(a.amount.value) * b.quote.amount.value)/b.base.amount.value;
            FC_ASSERT( result.hi == 0 );
            return asset( result.to_uint64(), b.quote.symbol );
         }
         else if( a.symbol == b.quote.symbol )
         {
            FC_ASSERT( b.quote.amount.value > 0 );
            uint128_t result = (uint128_t(a.amount.value) * b.base.amount.value)/b.quote.amount.value;
            FC_ASSERT( result.hi == 0 );
            return asset( result.to_uint64(), b.base.symbol );
         }
         FC_THROW_EXCEPTION( fc::assert_exception, "invalid asset * price", ("asset",a)("price",b) );
      }

      price operator / ( const asset& base, const asset& quote )
      { try {
         FC_ASSERT( base.symbol != quote.symbol );
         return price{ base, quote };
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }

      price price::max( asset_symbol_type base, asset_symbol_type quote ) { return asset( share_type(STEEM_MAX_SATOSHIS), base ) / asset( share_type(1), quote); }
      price price::min( asset_symbol_type base, asset_symbol_type quote ) { return asset( 1, base ) / asset( STEEM_MAX_SATOSHIS, quote); }

      bool price::is_null() const { return *this == price(); }

      void price::validate() const
      { try {
         FC_ASSERT( base.amount > share_type(0) );
         FC_ASSERT( quote.amount > share_type(0) );
         FC_ASSERT( base.symbol != quote.symbol );
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }


string asset::to_string()const
{
   string result = fc::to_string(amount.value / SOPHIATX_SATOSHIS);
   auto fract = amount.value % SOPHIATX_SATOSHIS;
      // prec is a power of ten, so for example when working with
      // 7.005 we have fract = 5, prec = 1000.  So prec+fract=1005
      // has the correct number of zeros and we can simply trim the
      // leading 1.
   result += "." + fc::to_string(SOPHIATX_SATOSHIS + fract).erase(0,1);
   
   return result + " " + symbol.to_string();
}

asset asset::from_string( const std::string& from )
{
   try
   {
      std::string s = fc::trim( from );
      auto space_pos = s.find( " " );
      auto dot_pos = s.find( "." );

      FC_ASSERT( space_pos != std::string::npos );

      asset result;

      std::string str_symbol = s.substr( space_pos + 1 );

      if( dot_pos != std::string::npos )
      {
         FC_ASSERT( space_pos > dot_pos );

         auto intpart = s.substr( 0, dot_pos );
         auto fractpart = "1" + s.substr( dot_pos + 1, space_pos - dot_pos - 1 );
         //uint8_t decimals = uint8_t( fractpart.size() - 1 );

         result.symbol = asset_symbol_type::from_string( str_symbol.c_str() );

         result.amount = fc::to_int64( intpart );
         result.amount.value *= SOPHIATX_SATOSHIS;
         result.amount.value += fc::to_int64( fractpart );
         result.amount.value -= SOPHIATX_SATOSHIS;
      }
      else
      {
         auto intpart = s.substr( 0, space_pos );
         result.amount = fc::to_int64( intpart );
         result.symbol = asset_symbol_type::from_string( str_symbol.c_str() );
      }
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (from) )
}

} } // steem::protocol

namespace fc {
   void to_variant( const steem::protocol::asset& var, fc::variant& vo )
   {
      vo = var.to_string();
      /*try
      {
         std::vector< variant > v( 2 );
         v[0] = boost::lexical_cast< std::string >( var.amount.value );
         v[1] = var.symbol.to_string();
         vo = v;
      } FC_CAPTURE_AND_RETHROW()*/
   }

   void from_variant( const fc::variant& var, steem::protocol::asset& vo )
   {
      vo = steem::protocol::asset::from_string(var.as< std::string >());
      /*try
      {
         auto v = var.as< std::vector< variant > >();
         FC_ASSERT( v.size() == 2, "Expected tuple of length 2." );

         // share_type is safe< int64_t >
         vo.amount = boost::lexical_cast< int64_t >( v[0].as< std::string >() );
         FC_ASSERT( vo.amount >= 0, "Asset amount cannot be negative" );
         vo.symbol = steem::protocol::asset_symbol_type::from_string( v[1].as< std::string >() );
      } FC_CAPTURE_AND_RETHROW((var))*/
   }
}
