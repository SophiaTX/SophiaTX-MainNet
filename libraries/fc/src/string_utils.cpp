#include <fc/string_utils.hpp>
#include <fc/exception/exception.hpp>

namespace fc {
/**
   * Parses a size including an optional multiplicative suffix.
   *
   * M   -> 1024*1024 bytes
   * MB  -> 1000*1000 bytes
   * MiB -> 1024*1024 bytes
   *
   * The 'M' may be any of KMGTPEZY (upper or lower case)
   */
uint64_t parse_size( const std::string& s )
{
    try
    {
        size_t i = 0, n = s.size(), suffix_start = n;
        for( i=0; i<n; i++ )
        {
            if( !((s[i] >= '0') && (s[i] <= '9')) )
            {
                suffix_start = i;
                break;
            }
        }
        uint64_t u = std::stoull( s.substr( 0, suffix_start ) );

        FC_ASSERT( n - suffix_start <= 3 );

        uint64_t m = 1;
        uint64_t thousand = 1024;

        if( suffix_start == n )
        {
            return u;
        }
        else if( suffix_start == n-1 )
        {
        }
        else if( suffix_start == n-2 )
        {
            FC_ASSERT( (s[suffix_start+1] == 'b') || (s[suffix_start+1] == 'B') );
            thousand = 1000;
        }
        else if( suffix_start == n-3 )
        {
            FC_ASSERT( (s[suffix_start+1] == 'i') || (s[suffix_start+1] == 'I') );
            FC_ASSERT( (s[suffix_start+2] == 'b') || (s[suffix_start+2] == 'B') );
        }
        switch( s[suffix_start] )
        {
            case 'y':
            case 'Y':
                m *= thousand;
            case 'z':
            case 'Z':
                m *= thousand;
            case 'e':
            case 'E':
                m *= thousand;
            case 'p':
            case 'P':
                m *= thousand;
            case 't':
            case 'T':
                m *= thousand;
            case 'g':
            case 'G':
                m *= thousand;
            case 'm':
            case 'M':
                m *= thousand;
            case 'k':
            case 'K':
                m *= thousand;
                break;
            default:
                FC_ASSERT( false );
        }
        return u*m;
    }
    catch( const fc::exception& e )
    {
        FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse size" );
    }
}
}