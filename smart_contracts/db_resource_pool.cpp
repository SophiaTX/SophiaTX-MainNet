#include "sophiatx/smart_contracts/db_resource_pool.h"
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

namespace sophiatx { namespace smart_contracts {

db_resource_pool::db_resource_pool(uint32_t max_resources_count) :
   db_resources(),
   db_resources_by_name(db_resources.get<by_account_name>()),
   db_resources_by_last_access(db_resources.get<by_last_access>()),
   max_resources(max_resources_count)
{}

SQLite::Database& db_resource_pool::get_resource(const std::string &account_name) {
   try {
      auto found_db = db_resources_by_name.find(account_name);
      if (found_db == db_resources_by_name.end()) {
         return create_resource(account_name);
      }

      // Adjusts last_access of the found db
      db_resources_by_name.modify(found_db, [](db_resource &resource) { resource.update_access_time(); });

      return found_db->db_handle;
   }
   catch (const std::exception& e) {
      throw sophiatx::smart_contracts::resource_error(e.what());
   }
   catch (const boost::exception& e) {
      throw sophiatx::smart_contracts::resource_error(boost::diagnostic_information(e));
   }
   catch (...) {
      throw sophiatx::smart_contracts::resource_error("Unknown");
   }
}

SQLite::Database& db_resource_pool::create_resource(const std::string &account_name) {
   if (db_resources_by_name.size() >= max_resources) {
      pop_resource();
   }

   // Creates new database handle inside pool
   auto emplace_result = db_resources_by_name.emplace(account_name);
   if (emplace_result.second == false) {
      throw std::runtime_error("Unable to create database resource for account: " + account_name + ". Possible problem: Such resource already exists");
   }

   return emplace_result.first->db_handle;
}

void db_resource_pool::pop_resource() {
   if (db_resources_by_last_access.empty() == false) {
      db_resources_by_last_access.erase(db_resources_by_last_access.begin());
   }
}

void db_resource_pool::test() {
   for (auto it = db_resources_by_last_access.begin(); it != db_resources_by_last_access.end(); it++) {
      std::cout << "it->account_name: " << it->account_name << std::endl;
   }
}

}}