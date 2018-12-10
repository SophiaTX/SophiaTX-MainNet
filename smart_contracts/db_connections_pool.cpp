#include "include/sophiatx/smart_contracts/db_connections_pool.h"
#include <iostream>

namespace sophiatx { namespace smart_contracts {

db_connections_pool::db_connections_pool(uint32_t max_handles_count) :
   db_handles(),
   db_handles_by_name(db_handles.get<by_account_name>()),
   db_handles_by_last_access(db_handles.get<by_last_access>()),
   max_handles(max_handles_count)
{}

SQLite::Database& db_connections_pool::get_database(const std::string& account_name) {
   //db_handles_by_name_index& databases = db_handles.get<by_account_name>();

   auto found_db = db_handles_by_name.find(account_name);
   if (found_db == db_handles_by_name.end()) {
      return create_database(account_name);
   }

   // Adjusts last_access of the found db
   db_handles_by_name.modify( found_db, [](db_connection_resource& resource){ resource.update_access_time(); } );

   return found_db->get_db_handle();
}

SQLite::Database& db_connections_pool::create_database(const std::string& account_name) {
   //db_handles_by_name_index& databases = db_handles.get<by_account_name>();
   if (db_handles_by_name.size() >= max_handles) {
      delete_least_used_database();
   }

   // Creates new database handle inside pool
   auto emplace_result = db_handles_by_name.emplace(account_name);
   if (emplace_result.second == false) {
      throw std::runtime_error("Unable to create database for account: " + account_name + ". Possible problem: Such database already exists");
   }

   return emplace_result.first->get_db_handle();
}

void db_connections_pool::delete_least_used_database() {
//   db_handles_by_last_access_index & databases = db_handles.get<by_last_access>();

   if (db_handles_by_last_access.empty() == false) {
      db_handles_by_last_access.erase(db_handles_by_last_access.begin());
   }
}

void db_connections_pool::test() {
   for (auto it = db_handles_by_last_access.begin(); it != db_handles_by_last_access.end(); it++) {
      std::cout << "it->account_name: " << it->account_name << std::endl;
   }
}

}}