#include <fc/log/sys_logger.hpp>

namespace fc {

SysLogger::SysLogger(const std::string &app_name,
                     int min_log_level,
                     const std::optional <std::string> &msg_prefix,
                     int facility,
                     int options) :
      app_name_(app_name),
      min_log_level_(min_log_level),
      msg_prefix_(msg_prefix) {

   // Validates min log level
   if (min_log_level < LOG_EMERG || min_log_level > LOG_DEBUG) {
      throw std::runtime_error("Invalid SysLogger min. level: " + std::to_string(min_log_level_));
   }

   openlog(app_name_.c_str(), options, facility);
}

SysLogger::~SysLogger() {
   closelog();
}

const std::string &SysLogger::getAppName() const {
   return app_name_;
}

int SysLogger::getMinLogLevel() const {
   return min_log_level_;
}

const std::optional <std::string> &SysLogger::getMsgPrefix() const {
   return msg_prefix_;
}

}
