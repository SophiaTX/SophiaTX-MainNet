#pragma once

#include <steem/protocol/asset.hpp>

namespace steem { namespace chain { namespace util {

using steem::protocol::asset;
using steem::protocol::price;

inline asset to_sbd( const price& p, const asset& steem )
{
   FC_ASSERT( steem.symbol == STEEM_SYMBOL );
   FC_ASSERT( p.quote.symbol == STEEM_SYMBOL || p.base.symbol == STEEM_SYMBOL );
   return steem * p;
}

inline asset to_steem( const price& p, const asset& sbd )
{
   FC_ASSERT( sbd.symbol == SBD1_SYMBOL || sbd.symbol == SBD2_SYMBOL || sbd.symbol == SBD3_SYMBOL || sbd.symbol == SBD4_SYMBOL || sbd.symbol == SBD5_SYMBOL );
   FC_ASSERT( p.quote.symbol == STEEM_SYMBOL || p.base.symbol == STEEM_SYMBOL );
   FC_ASSERT( p.quote.symbol == sbd.symbol || p.base.symbol == sbd.symbol);
   return sbd * p;
}

} } }
