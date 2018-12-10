#ifndef SOPHIATX_DB_CONNECTION_RESOURCE_H
#define SOPHIATX_DB_CONNECTION_RESOURCE_H

#include <SQLiteCpp/SQLiteCpp.h>
#include <chrono>

namespace sophiatx { namespace smart_contracts {


class db_connection_resource {
public:
   // these members are public because of boost_multindex container
   std::string                              account_name;
   std::chrono::system_clock::time_point    last_access;


   db_connection_resource(const std::string& acc_name);
   ~db_connection_resource()                                         = default;

   // Disables default/copy/move ctors - database handle should not be copied
   db_connection_resource()                                          = delete;
   db_connection_resource(const db_connection_resource&)             = delete;
   db_connection_resource& operator=(const db_connection_resource&)  = delete;
   db_connection_resource(db_connection_resource&&)                  = delete;
   db_connection_resource& operator=(db_connection_resource&&)       = delete;

   SQLite::Database& get_db_handle();
   void update_access_time();

private:
   SQLite::Database                         db_handle;
};


}}

#endif //SOPHIATX_DB_CONNECTION_RESOURCE_H









