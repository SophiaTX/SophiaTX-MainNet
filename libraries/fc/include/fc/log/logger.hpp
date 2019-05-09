#pragma once

#include <memory>
#include <fc/log/log_message.hpp>
#include <sophiatx/utilities/sys_logger.hpp>

namespace fc
{

/**
 * @brief Singleton global application syslogger implementation. For custom Syslogger, use directly sophiatx::utilities::SysLogger
 */
class Logger {
public:
   // Delete all constructors
   Logger()                         = delete;
   Logger(const Logger&)            = delete;
   Logger& operator=(const Logger&) = delete;
   Logger(Logger&)                  = delete;
   Logger& operator=(Logger&&)      = delete;
   ~Logger()                        = default;

   /**
    * @brief Initializes logger
    *
    * @param app_name name of the application. Based on this parameter, separate logs are created from syslog, see configs in etc/syslog.d
    * @param log_level possible values:
    *           "debug"     - LOG_EMERG	0	   // system is unusable
    *           "emergency" - LOG_ALERT	1	   // action must be taken immediately
    *           "critical"  - LOG_CRIT	   2	   // critical conditions
    *           "error"     - LOG_ERR		3	   // error conditions
    *           "warning"   - LOG_WARNING	4	   // warning conditions
    *           "notice"    - LOG_NOTICE	5	   // normal but significant condition
    *           "info"      - LOG_INFO	   6	   // informational
    *           "debug"     - LOG_DEBUG	7	   // debug-level messages
    */
   static void init(const std::string& app_name, const std::string& log_level_str);

   /**
    * @brief Returns true if logger was initialized, otherwise false
    */
   static bool isInitialized();

   /**
    * @brief Returns pointer to logger
    */
   static const std::unique_ptr<sophiatx::utilities::SysLogger>& get_instance();
private:
   static std::unique_ptr<sophiatx::utilities::SysLogger> logger_;

}; // class Logger




} // namespace fc

#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define LOCATION "[" __FILE__ ":" STRINGIFY(__LINE__) "] --"

//Usage: ilog( "Format four: ${arg}  five: ${five}", ("arg",4)("five",5) );
#define dlog( FORMAT, ... ) fc::Logger::get_instance()->debug( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )
#define ilog( FORMAT, ... ) fc::Logger::get_instance()->info( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )
#define nlog( FORMAT, ... ) fc::Logger::get_instance()->notice( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )
#define wlog( FORMAT, ... ) fc::Logger::get_instance()->warning( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )
#define elog( FORMAT, ... ) fc::Logger::get_instance()->error( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )
#define clog( FORMAT, ... ) fc::Logger::get_instance()->critical( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )
#define alog( FORMAT, ... ) fc::Logger::get_instance()->alert( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )
#define emlog( FORMAT, ... ) fc::Logger::get_instance()->emergency( LOCATION, FC_LOG_MESSAGE_( FORMAT, __VA_ARGS__ ).get_message() )



// this disables all normal logging statements -- not something you'd normally want to do,
// but it's useful if you're benchmarking something and suspect logging is causing
// a slowdown.
#ifdef FC_DISABLE_LOGGING
   # undef dlog
   # define dlog(...)
   # undef ilog
   # define ilog(...)
   # undef nlog
   # define nlog(...)
   # undef wlog
   # define wlog(...)
   # undef elog
   # define elog(...)
   # undef clog
   # define clog(...)
   # undef alog
   # define alog(...)
   # undef emlog
   # define emlog(...)
#endif


#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/paren.hpp>


#define FC_FORMAT_ARG(r, unused, base) \
  BOOST_PP_STRINGIZE(base) ": ${" BOOST_PP_STRINGIZE( base ) "} "

#define FC_FORMAT_ARGS(r, unused, base) \
  BOOST_PP_LPAREN() BOOST_PP_STRINGIZE(base),fc::variant(base) BOOST_PP_RPAREN()

#define FC_FORMAT( SEQ )\
    BOOST_PP_SEQ_FOR_EACH( FC_FORMAT_ARG, v, SEQ )

// takes a ... instead of a SEQ arg because it can be called with an empty SEQ
// from FC_CAPTURE_AND_THROW()
#define FC_FORMAT_ARG_PARAMS( ... )\
    BOOST_PP_SEQ_FOR_EACH( FC_FORMAT_ARGS, v, __VA_ARGS__ )

#define ddump( SEQ ) \
    dlog( FC_FORMAT(SEQ), FC_FORMAT_ARG_PARAMS(SEQ) )
#define idump( SEQ ) \
    ilog( FC_FORMAT(SEQ), FC_FORMAT_ARG_PARAMS(SEQ) )
#define wdump( SEQ ) \
    wlog( FC_FORMAT(SEQ), FC_FORMAT_ARG_PARAMS(SEQ) )
#define edump( SEQ ) \
    elog( FC_FORMAT(SEQ), FC_FORMAT_ARG_PARAMS(SEQ) )
