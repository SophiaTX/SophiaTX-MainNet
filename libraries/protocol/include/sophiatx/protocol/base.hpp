#pragma once
#include <sophiatx/protocol/asset.hpp>
#include <sophiatx/protocol/types.hpp>
#include <sophiatx/protocol/authority.hpp>
#include <sophiatx/protocol/version.hpp>

#include <fc/time.hpp>

namespace sophiatx { namespace protocol {

   struct base_operation
   {
      void get_required_authorities( vector<authority>& )const {}
      void get_required_active_authorities( flat_set<account_name_type>& )const {}
      void get_required_posting_authorities( flat_set<account_name_type>& )const {}
      void get_required_owner_authorities( flat_set<account_name_type>& )const {}

      virtual bool has_special_fee()const{return false;};
      virtual asset get_required_fee(asset_symbol_type in_symbol)const{
         if(in_symbol == SBD1_SYMBOL )//USD
            return asset(100000, SBD1_SYMBOL);
         if(in_symbol == SBD2_SYMBOL )//EUR
            return asset(80000, SBD2_SYMBOL);
         if(in_symbol == SBD3_SYMBOL ) //CHF
            return asset(100000, SBD3_SYMBOL);
         if(in_symbol == SBD4_SYMBOL ) //CNY
            return asset(640000, SBD4_SYMBOL);
         if(in_symbol == SBD5_SYMBOL ) //CNY
            return asset(75000, SBD5_SYMBOL);
         return asset(100000, SOPHIATX_SYMBOL);
      };

      virtual bool is_virtual()const { return false; }
      void validate()const {}
      asset fee;
      virtual account_name_type get_fee_payer()const { return SOPHIATX_INIT_MINER_NAME; };
      virtual ~base_operation(){}
   };

   struct virtual_operation : public base_operation
   {
      bool is_virtual()const { return true; }
      void validate()const { FC_ASSERT( false, "This is a virtual operation" ); }
   };

   typedef static_variant<
      void_t,
      version,              // Normal witness version reporting, for diagnostics and voting
      hardfork_version_vote // Voting for the next hardfork to trigger
      >                                block_header_extensions;

   typedef static_variant<
      void_t
      >                                future_extensions;

   typedef flat_set<block_header_extensions > block_header_extensions_type;
   typedef flat_set<future_extensions> extensions_type;


} } // sophiatx::protocol

FC_REFLECT( sophiatx::protocol::base_operation, (fee))
FC_REFLECT_TYPENAME( sophiatx::protocol::block_header_extensions )
FC_REFLECT_TYPENAME( sophiatx::protocol::future_extensions )
