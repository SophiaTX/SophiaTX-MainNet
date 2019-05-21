#pragma once
#include <string>

namespace fc
{

    typedef std::string string;

    uint64_t parse_size( const std::string& s );

    class variant_object;
    std::string format_string( const std::string&, const variant_object& );

}
