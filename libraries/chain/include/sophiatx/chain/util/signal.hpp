#pragma once

#include <boost/signals2/signal.hpp>

namespace sophiatx { namespace chain { namespace util {

inline void disconnect_signal( boost::signals2::connection& signal )
{
   if( signal.connected() )
      signal.disconnect();
   FC_ASSERT( !signal.connected() );
}

} } }
