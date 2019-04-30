#include <sophiatx/utilities/SysLogger.hpp>

namespace sophiatx { namespace utilities {

SysLogger::SysLogger(const std::string &appName,
                     int minLogLevel,
                     const std::experimental::optional <std::string> &msgPrefix,
                     int facility,
                     int options) :
      appName_(appName),
      minLogLevel_(minLogLevel),
      msgPrefix_(msgPrefix) {

   // Validates min log level
   if (minLogLevel < LOG_EMERG || minLogLevel > LOG_DEBUG) {
      throw std::runtime_error("Invalid SysLogger min. level: " + std::to_string(minLogLevel_));
   }

   openlog(appName_.c_str(), options, facility);
}

SysLogger::~SysLogger() {
   closelog();
}

const std::string &SysLogger::getAppName() const {
   return appName_;
}

int SysLogger::getMinLogLevel() const {
   return minLogLevel_;
}

const std::experimental::optional <std::string> &SysLogger::getMsgPrefix() const {
   return msgPrefix_;
}

}}
