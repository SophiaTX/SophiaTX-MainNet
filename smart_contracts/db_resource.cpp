#include "sophiatx/smart_contracts/db_resource.h"

namespace sophiatx { namespace smart_contracts {

db_resource::db_resource(const boost::filesystem::path& data_directory, const std::string& acc_name) :
   account_name(acc_name),
   last_access(std::chrono::system_clock::now()),
   db_handle(data_directory.generic_string() + "/" + account_name + ".db", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE)
{}


void db_resource::update_access_time() {
   this->last_access = std::chrono::system_clock::now();
}

}}