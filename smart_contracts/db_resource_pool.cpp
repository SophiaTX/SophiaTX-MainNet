#include "sophiatx/smart_contracts/db_resource_pool.h"
#include <boost/filesystem.hpp>
#include <iostream>

namespace sophiatx { namespace smart_contracts {

db_resource_pool::db_resource_pool(const boost::filesystem::path& data_directory, uint32_t max_resources_count) :
      max_resources(max_resources_count),
      data_dir(data_directory),
      db_resources(),
      db_resources_by_name(db_resources.get<by_account_name>()),
      db_resources_by_last_access(db_resources.get<by_last_access>()) {
   if (data_dir.is_relative() == true) {
      throw resource_error("Provided data dir path for smart contracts must be absolute.");
   }

   if (boost::filesystem::exists(data_dir) == false) {
      boost::filesystem::create_directory(data_dir);
   }
}

SQLite::Database& db_resource_pool::get_resource(const std::string &account_name, bool create_flag) {
   auto resource = db_resources_by_name.find(account_name);
   if (resource == db_resources_by_name.end()) {
      if (create_flag == true) {
         return create_resource(account_name);
      }

      throw resource_error("There is no resource mapped to the acc name: " + account_name);
   }

   // Adjusts last_access of the found db
   if (db_resources_by_name.modify(resource, [](db_resource &resource) { resource.update_access_time(); }) == false) {
      throw resource_error("Unable to modify last access time of resource mapped to the acc name: " + account_name);
   }

   return resource->db_handle;
}

SQLite::Database& db_resource_pool::create_resource(const std::string &account_name) {
   auto resource = db_resources_by_name.find(account_name);
   if (resource != db_resources_by_name.end()) {
      return resource->db_handle;
   }

   if (db_resources_by_name.size() >= max_resources) {
      pop_resource();
   }

   // Creates new database handle inside pool
   auto emplace_result = db_resources_by_name.emplace(data_dir, account_name);
   if (emplace_result.second == false) {
      throw resource_error("Unable to create database resource for account: " + account_name + ". Possible problem: Such resource already exists");
   }

   return emplace_result.first->db_handle;
}

void db_resource_pool::pop_resource() {
   if (db_resources_by_last_access.empty() == false) {
      db_resources_by_last_access.erase(db_resources_by_last_access.begin());
   }
}

const uint32_t db_resource_pool::size() const {
   return db_resources.size();
}

}}