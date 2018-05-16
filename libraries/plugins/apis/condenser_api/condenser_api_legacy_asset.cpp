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
   return SOPHIATX_SATOSHIS;
}


}

namespace steem { namespace plugins { namespace condenser_api {






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

      FC_ASSERT( space_pos != std::string::npos );

      auto numpart = s.substr( 0, space_pos );
      auto dvalue = fc::to_double(numpart);

      legacy_asset result;

      string str_symbol = s.substr( space_pos + 1 );

      result.symbol = asset_symbol_type::from_string( str_symbol.c_str() );
      result.amount = dvalue * precision( result.symbol );

      return result;
   }
   FC_CAPTURE_AND_RETHROW( (from) )
}

} } }
