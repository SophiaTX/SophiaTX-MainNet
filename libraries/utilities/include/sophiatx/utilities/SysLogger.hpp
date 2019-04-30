#ifndef SOPHIATX_SYSLOGGER_HPP
#define SOPHIATX_SYSLOGGER_HPP

#include <sys/syslog.h>
#include <string>
#include <sstream>
#include <experimental/optional>

namespace sophiatx { namespace utilities {

class SysLogger {
public:
   /**
    * @brief Constructor
    *
    * @param appName     name of application that syslog message belongs to. Can be used for filtering
    *                    application specific messages from /var/log/syslog and redirecting it to the custom file
    * @param minLogLevel minimum level of message that is displayeD in syslog. Possible values range: <LOG_DEBUG, LOG_EMERG>
    * @param msgPrefix   custom prefix that is desplayed before actual message
    * @param facility    facility codes
    *                        LOG_KERN	(0<<3)	// kernel messages
    *                   	    LOG_USER	(1<<3)	// random user-level messages
    *                       	LOG_MAIL	(2<<3)	// mail system
    *                       	LOG_DAEMON	(3<<3)	// system daemons
    *                       	LOG_AUTH	(4<<3)	// security/authorization messages
    *                       	LOG_SYSLOG	(5<<3)	// messages generated internally by syslogd
    *                       	LOG_LPR		(6<<3)	// line printer subsystem
    *                       	LOG_NEWS	(7<<3)	// network news subsystem
    *                       	LOG_UUCP	(8<<3)	// UUCP subsystem
    *                       	LOG_CRON	(9<<3)	// clock daemon
    *                       	LOG_AUTHPRIV	(10<<3)	// security/authorization messages (private)
    *                       	LOG_FTP		(11<<3)	// ftp daemon
    *                    // other codes through 15 reserved for system use
    *                       	LOG_LOCAL0	(16<<3)	// reserved for local use
    *                       	LOG_LOCAL1	(17<<3)	// reserved for local use
    *                       	LOG_LOCAL2	(18<<3)	// reserved for local use
    *                       	LOG_LOCAL3	(19<<3)	// reserved for local use
    *                       	LOG_LOCAL4	(20<<3)	// reserved for local use
    *                       	LOG_LOCAL5	(21<<3)	// reserved for local use
    *                       	LOG_LOCAL6	(22<<3)	// reserved for local use
    *                       	LOG_LOCAL7	(23<<3)	// reserved for local use
    * @param options     Option flags for openlog.
    *                        LOG_PID		0x01	  // log the pid with each message
    *                        LOG_CONS	0x02	  // log on the console if errors in sending
    *                        LOG_ODELAY	0x04	// delay open until first syslog() (default)
    *                        LOG_NDELAY	0x08	// don't delay open
    *                        LOG_NOWAIT	0x10	// don't wait for console forks: DEPRECATED
    *                        LOG_PERROR	0x20	// log to stderr as well
    */
   SysLogger(const std::string &appName,
             int minLogLevel = LOG_INFO,
             const std::experimental::optional<std::string> &msgPrefix = {},
             int facility = LOG_LOCAL0,
             int options = LOG_NDELAY | LOG_PID | LOG_PERROR);

   /**
    * @brief Destructor
    */
   ~SysLogger();

   /**
    * @brief Logs "debug" message to the syslog
    *        debug == Debug-level messages.
    *                 Messages that contain information normally of use only when debugging a program
    * @param message
    */
   template<typename... Args>
   void debug(Args &&... args) {
      loggToSyslog(LOG_DEBUG, std::forward<Args>(args)...);
   }

   /**
    * @brief Logs "info" message to the syslog
    *        info == Informational messages.
    *
    * @param message
    */
   template<typename... Args>
   void info(Args &&... args) {
      loggToSyslog(LOG_INFO, std::forward<Args>(args)...);
   }

   /**
    * @brief Logs "notice" message to the syslog
    *        notice == Normal but significant conditions.
    *                  Conditions that are not error conditions, but that may require special handling.
    *
    * @param message
    */
   template<typename... Args>
   void notice(Args &&... args) {
      loggToSyslog(LOG_NOTICE, std::forward<Args>(args)...);
   }

   /**
    * @brief Logs "warning" message to the syslog
    *        warning == Warning conditions.
    *
    * @param message
    */
   template<typename... Args>
   void warning(Args &&... args) {
      loggToSyslog(LOG_WARNING, std::forward<Args>(args)...);
   }

   /**
    * @brief Logs "error" message to the syslog
    *        error == Error conditions.
    *
    * @param message
    */
   template<typename... Args>
   void error(Args &&... args) {
      loggToSyslog(LOG_ERR, std::forward<Args>(args)...);
   }

   /**
    * @brief Logs "critical" message to the syslog
    *        critical == Critical conditions, such as hard device errors.
    *
    * @param message
    */
   template<typename... Args>
   void critical(Args &&... args) {
      loggToSyslog(LOG_CRIT, std::forward<Args>(args)...);
   }

   /**
    * @brief Logs "alert" message to the syslog
    *        alert == Action must be taken immediately.
    *                 A condition that should be corrected immediately, such as a corrupted system database.
    *
    * @param message
    */
   template<typename... Args>
   void alert(Args &&... args) {
      loggToSyslog(LOG_ALERT, std::forward<Args>(args)...);
   }

   /**
    * @brief Logs "emergency" message to the syslog
    *        emergency == System is unusable.
    *                     A panic condition.
    *
    * @param message
    */
   template<typename... Args>
   void emergency(Args &&... args) {
      loggToSyslog(LOG_EMERG, std::forward<Args>(args)...);
   }


   /**
    * Getters
    */
   const std::experimental::optional<std::string> &getMsgPrefix() const;

   const std::string &getAppName() const;

   int getMinLogLevel() const;

private:
   /**
    * @brief Logs provided arguments to the syslog
    *
    * @tparam Args
    * @param logLevel
    * @param args
    */
   template<typename... Args>
   void loggToSyslog(int logLevel, Args &&... args) {
      // Extracts only 3 bottom bits that are the priority (0-7) - DEBUG, ERROR, etc...
      int logPriorityLevel = logLevel & LOG_PRIMASK;
      if (logPriorityLevel > minLogLevel_) {
         return;
      }

      std::string message;

      // If prefix specified, adds it to the beginning of message
      if (msgPrefix_ && msgPrefix_->empty() == false) {
         message = argsToString(msgPrefix_.value(), std::forward<Args>(args)...);
      } else {
         message = argsToString(std::forward<Args>(args)...);
      }

      // Sends message to the syslog
      syslog(logLevel, "%s", message.c_str());
   }

   /**
    * @brief convert provided arguments to string(if possible - each arg type must overload operator <<)
    *
    * @tparam Args
    * @param args
    * @return string representation of args, separated by whitespace
    */
   template<typename... Args>
   std::string argsToString(Args &&... args) {
      std::ostringstream stream;
      using List = int[];
      (void) List{0, ((void) (stream << args << ' '), 0)...};
      return stream.str();
   }


private:
   std::string appName_;
   int minLogLevel_;
   std::experimental::optional<std::string> msgPrefix_;
};

}}

#endif //SOPHIATX_SYSLOGGER_HPP
