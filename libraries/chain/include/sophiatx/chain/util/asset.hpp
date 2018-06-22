#pragma once

#include <sophiatx/protocol/asset.hpp>

namespace sophiatx { namespace chain { namespace util {

using sophiatx::protocol::asset;
using sophiatx::protocol::price;

inline asset to_sbd( const price& p, const asset& sophiatx )
{
   FC_ASSERT( sophiatx.symbol == SOPHIATX_SYMBOL );
   FC_ASSERT( p.quote.symbol == SOPHIATX_SYMBOL || p.base.symbol == SOPHIATX_SYMBOL );
   return sophiatx * p;
}

inline asset to_sophiatx( const price& p, const asset& sbd )
{
   FC_ASSERT( sbd.symbol == SBD1_SYMBOL || sbd.symbol == SBD2_SYMBOL || sbd.symbol == SBD3_SYMBOL || sbd.symbol == SBD4_SYMBOL || sbd.symbol == SBD5_SYMBOL );
   FC_ASSERT( p.quote.symbol == SOPHIATX_SYMBOL || p.base.symbol == SOPHIATX_SYMBOL );
   FC_ASSERT( p.quote.symbol == sbd.symbol || p.base.symbol == sbd.symbol);
   return sbd * p;
}

} } }
