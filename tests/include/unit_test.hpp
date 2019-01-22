
#if __GNUC__ >= 8 && defined( __has_warning )
#   if __has_warning( "-Wformat-overflow=" )
#       define SUPPRESSING
#       pragma GCC diagnostic push
#       pragma GCC diagnostic ignored "-Wformat-overflow="
#   endif
#endif

#include <boost/test/included/unit_test.hpp>

#ifdef SUPPRESSING
#   undef SUPPRESSING
#   pragma GCC diagnostic pop
#endif