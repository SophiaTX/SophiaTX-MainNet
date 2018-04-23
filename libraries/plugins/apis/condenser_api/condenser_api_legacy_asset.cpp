#include <steem/plugins/condenser_api/condenser_api_legacy_asset.hpp>


namespace {

void remove_trailing_zeros(std::string& str) {
   if (str.find('.') == std::string::npos) {
      return;
   }

   int offset = 1;
   if (str.find_last_not_of('0') == str.find('.')) {
      offset = 0;
   }
   str.erase(str.find_last_not_of('0') + offset, std::string::npos);
}

int64_t precision( const steem::protocol::asset_symbol_type& symbol )
{
   /*static int64_t table[] = {
         1, 10, 100, 1000, 10000,
         100000, 1000000, 10000000, 100000000ll,
         1000000000ll, 10000000000ll,
         100000000000ll, 1000000000000ll,
         10000000000000ll, 100000000000000ll
   };
   uint8_t d = symbol.decimals();
   return table[ d ];*/
   return 1000000;
}


}

namespace steem { namespace plugins { namespace condenser_api {


share_type legacy_asset::amount_from_string(std::string amount_string) const
{

   try {
      remove_trailing_zeros(amount_string);

      bool negative_found = false;
      bool decimal_found = false;
      for( const char c : amount_string )
      {
         if( isdigit( c ) )
            continue;

         if( c == '-' && !negative_found )
         {
            negative_found = true;
            continue;
         }

         if( c == '.' && !decimal_found )
         {
            decimal_found = true;
            continue;
         }

         FC_THROW( (amount_string) );
      }

      share_type satoshis = 0;

      uint32_t num_precision = this->symbol.decimals();
      share_type scaled_precision = precision(this->symbol);

      const auto decimal_pos = amount_string.find( '.' );
      const string lhs = amount_string.substr( negative_found, decimal_pos );
      if( !lhs.empty() )
         satoshis += fc::safe<int64_t>(std::stoll(lhs)) *= scaled_precision;

      if( decimal_found )
      {
         const size_t max_rhs_size = std::to_string( scaled_precision.value ).substr( 1 ).size();

         string rhs = amount_string.substr( decimal_pos + 1, num_precision );
         FC_ASSERT( rhs.size() <= max_rhs_size );

         while( rhs.size() < max_rhs_size )
            rhs += '0';

         if( !rhs.empty() )
            satoshis += std::stoll( rhs );
      }

      if( negative_found )
         satoshis *= -1;

      return satoshis;
   } FC_CAPTURE_AND_RETHROW( (amount_string) )

}

string legacy_asset::amount_to_string(share_type amount) const
{
   share_type scaled_precision = 1;
   for( uint8_t i = 0; i < this->symbol.decimals(); ++i )
      scaled_precision *= 10;
   assert(scaled_precision > 0);

   string result = fc::to_string(amount.value / scaled_precision.value);
   auto decimals = amount.value % scaled_precision.value;
   if( decimals )
      result += "." + fc::to_string(scaled_precision.value + decimals).erase(0,1);
   return result;
}





string legacy_asset::to_string()const
{
   int64_t prec = precision(symbol);
   string result = fc::to_string(amount.value / prec);
   if( prec > 1 )
   {
      auto fract = amount.value % prec;
      // prec is a power of ten, so for example when working with
      // 7.005 we have fract = 5, prec = 1000.  So prec+fract=1005
      // has the correct number of zeros and we can simply trim the
      // leading 1.
      result += "." + fc::to_string(prec + fract).erase(0,1);
   }
   return result + " " + symbol.to_string();
}

legacy_asset legacy_asset::from_string( const string& from )
{
   try
   {
      string s = fc::trim( from );
      auto space_pos = s.find( " " );
      auto dot_pos = s.find( "." );

      FC_ASSERT( space_pos != std::string::npos );

      legacy_asset result;

      string str_symbol = s.substr( space_pos + 1 );

      if( dot_pos != std::string::npos )
      {
         FC_ASSERT( space_pos > dot_pos );

         auto intpart = s.substr( 0, dot_pos );
         auto fractpart = "1" + s.substr( dot_pos + 1, space_pos - dot_pos - 1 );
         uint8_t decimals = uint8_t( fractpart.size() - 1 );

         result.symbol = asset_symbol_type::from_string( str_symbol.c_str() );

         int64_t prec = precision( result.symbol );

         result.amount = fc::to_int64( intpart );
         result.amount.value *= prec;
         result.amount.value += fc::to_int64( fractpart );
         result.amount.value -= prec;
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

} } }
