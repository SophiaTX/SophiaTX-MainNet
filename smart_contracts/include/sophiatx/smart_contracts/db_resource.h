#ifndef SOPHIATX_DB_RESOURCE_H
#define SOPHIATX_DB_RESOURCE_H

#include <SQLiteCpp/SQLiteCpp.h>
#include <chrono>

namespace sophiatx { namespace smart_contracts {


class db_resource {
public:
   std::string                              account_name;
   std::chrono::system_clock::time_point    last_access;
   mutable SQLite::Database                 db_handle;

   db_resource(const std::string& acc_name);
   ~db_resource()                              = default;

   // Disables default/copy/move ctors - database handle should not be copied
   db_resource()                               = delete;
   db_resource(const db_resource&)             = delete;
   db_resource& operator=(const db_resource&)  = delete;
   db_resource(db_resource&&)                  = delete;
   db_resource& operator=(db_resource&&)       = delete;

   void update_access_time();
};


}}

#endif //SOPHIATX_DB_RESOURCE_H









