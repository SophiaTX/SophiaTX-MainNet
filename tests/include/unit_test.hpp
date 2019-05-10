#ifdef __GNUC__
    #ifndef __clang__
        #define SUPRESSING
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-overflow="
    #endif
#endif

#include <boost/test/included/unit_test.hpp>

#ifdef SUPPRESSING
    #undef SUPPRESSING
    #pragma GCC diagnostic pop
#endif
