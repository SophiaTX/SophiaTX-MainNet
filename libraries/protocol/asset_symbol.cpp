//
// Created by Josef Sevcik on 18/07/2018.
//

#include <sophiatx/protocol/asset_symbol.hpp>


namespace sophiatx { namespace protocol {
asset_symbol_type asset_symbol_type::from_string( const std::string& str ){
   FC_ASSERT((str.size() >= 3 && str.size() <= 6), "invalid symbol length");
   const char* c_str = str.c_str();
   uint64_t ret = 0;
   int i = str.size();
   while( i-- ){
      ret = (ret << 8) | uint64_t(c_str[i]);
   }
   asset_symbol_type rv (ret) ;
   return rv;
}

std::string asset_symbol_type::to_string()const{
   std::string ret;
   uint64_t symbol = value;
   while ( symbol ) {
      ret.push_back(symbol & (uint64_t(255)));
      symbol = symbol >> 8;
   }
   //FC_ASSERT((ret.size() >= 3), "invalid symbol (${s}) length: ${l}", ("s",ret)("l",ret.size()));
   return ret;
}

}}//namespace sophiatx::protocol


namespace fc{

void to_variant( const sophiatx::protocol::asset_symbol_type& sym, fc::variant& var )
{
   try
   {
      var = sym.to_string();
   } FC_CAPTURE_AND_RETHROW()
}

void from_variant( const fc::variant& var, sophiatx::protocol::asset_symbol_type& vo )
{
   try
   {
      vo = sophiatx::protocol::asset_symbol_type::from_string(var.as< std::string >());

   } FC_CAPTURE_AND_RETHROW()
}

} // fc