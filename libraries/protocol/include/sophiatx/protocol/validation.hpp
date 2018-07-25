
#pragma once

#include <sophiatx/protocol/base.hpp>
#include <sophiatx/protocol/block_header.hpp>
#include <sophiatx/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace sophiatx { namespace protocol {

inline bool is_asset_type( asset asset, asset_symbol_type symbol )
{
   return asset.symbol == symbol;
}

inline void validate_account_name( const string& name )
{
   auto hash = fc::base64_decode(name);
   FC_ASSERT( hash.size() <= sizeof(account_name_type::data) );

}

inline void validate_permlink( const string& permlink )
{
   FC_ASSERT( permlink.size() < SOPHIATX_MAX_PERMLINK_LENGTH, "permlink is too long" );
   FC_ASSERT( fc::is_utf8( permlink ), "permlink not formatted in UTF8" );
}

} }
