#include "include/sophiatx/smart_contracts/db_connection_resource.h"

namespace sophiatx { namespace smart_contracts {

db_connection_resource::db_connection_resource(const std::string& acc_name) :
   account_name(acc_name),
   last_access(std::chrono::system_clock::from_time_t(time_t(nullptr))),
   db_handle(acc_name + ".db", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE)
{}


void db_connection_resource::update_access_time() {
   this->last_access = std::chrono::system_clock::now();
}

SQLite::Database& db_connection_resource::get_db_handle() {
   return db_handle;
}

}}