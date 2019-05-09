#       define SUPPRESSING
#       pragma GCC diagnostic push
#       pragma GCC diagnostic ignored "-Wformat-overflow="

#include <boost/test/included/unit_test.hpp>

#ifdef SUPPRESSING
#   undef SUPPRESSING
#   pragma GCC diagnostic pop
#endif