#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>

namespace fc {
std::unique_ptr<sophiatx::utilities::SysLogger> Logger::logger_ = nullptr;


void Logger::init(const std::string& app_name, const std::string& log_level_str) {
   uint log_level;
   if (log_level_str == "debug") {
      log_level = LOG_DEBUG;
   } else if (log_level_str == "info") {
      log_level = LOG_INFO;
   } else if (log_level_str == "notice") {
      log_level = LOG_NOTICE;
   } else if (log_level_str == "warning") {
      log_level = LOG_WARNING;
   } else if (log_level_str == "error") {
      log_level = LOG_ERR;
   } else if (log_level_str == "critical") {
      log_level = LOG_CRIT;
   } else if (log_level_str == "alert") {
      log_level = LOG_ALERT;
   } else if (log_level_str == "emergency") {
      log_level = LOG_EMERG;
   } else {
      FC_THROW("Invalid value for log-level: " + log_level_str);
   }

   logger_ = std::make_unique<sophiatx::utilities::SysLogger>(app_name, log_level);
}

const std::unique_ptr<sophiatx::utilities::SysLogger>& Logger::get_instance() {
   if (logger_ == nullptr) {
      throw std::runtime_error("fc::Logger::init(...) should be called before fc::Logger::get_instance()");
   }

   return logger_;
}

} // namespace fc
