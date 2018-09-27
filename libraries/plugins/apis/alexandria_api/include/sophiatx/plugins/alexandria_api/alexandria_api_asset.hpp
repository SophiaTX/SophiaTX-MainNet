#pragma once

#include <sophiatx/protocol/asset.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

using sophiatx::protocol::asset;
using sophiatx::protocol::asset_symbol_type;
using sophiatx::protocol::share_type;

struct api_asset
{
   public:
      api_asset() {}

      asset to_asset()const
      {
         return asset( amount, symbol );
      }

      operator asset()const { return to_asset(); }

      static api_asset from_asset( const asset& a )
      {
         api_asset leg;
         leg.amount = a.amount;
         leg.symbol = a.symbol;
         return leg;
      }

      string to_string() const;
      static api_asset from_string( const string& from );


      share_type                       amount;
      asset_symbol_type                symbol = SOPHIATX_SYMBOL;
};

} } } // sophiatx::plugins::alexandria_api

namespace fc {

   inline void to_variant( const sophiatx::plugins::alexandria_api::api_asset& a, fc::variant& var )
   {
      var = a.to_string();
   }

   inline void from_variant( const fc::variant& var, sophiatx::plugins::alexandria_api::api_asset& a )
   {
      a = sophiatx::plugins::alexandria_api::api_asset::from_string( var.as_string() );
   }

} // fc

FC_REFLECT( sophiatx::plugins::alexandria_api::api_asset,
   (amount)
   (symbol)
   )
