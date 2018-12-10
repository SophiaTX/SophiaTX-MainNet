
#include <iostream>
#include <sophiatx/smart_contracts/db_connections_pool.h>

int main() {
   using namespace sophiatx::smart_contracts;

   std::cout << "hello smart contract" << std::endl;

   db_connections_pool connections_pool(3);
   connections_pool.get_database("acc1");
   connections_pool.get_database("acc2");
   connections_pool.get_database("acc3");

   connections_pool.test();

   connections_pool.get_database("acc1");
   connections_pool.get_database("acc4");

   connections_pool.test();

   SQLite::Database& db = connections_pool.get_database("acc3");

   try
   {
      // Begin transaction
      SQLite::Transaction transaction(db);

      db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

      int nb = db.exec("INSERT INTO test VALUES (NULL, \"test\")");
      std::cout << "INSERT INTO test VALUES (NULL, \"test\")\", returned " << nb << std::endl;

      // Commit transaction
      transaction.commit();
   }
   catch (std::exception& e)
   {
      std::cout << "exception: " << e.what() << std::endl;
   }

//   db_connections_index db_resource_pool;
//   db_connections_index::index<by_account>::type& db_connections_by_name = db_resource_pool.get<by_account>();
//   db_connections_by_name.emplace("acc1");
//   db_connections_by_name.emplace("acc2");
//
//   for (auto it = db_connections_by_name.begin(); it != db_connections_by_name.end(); it++) {
//      std::cout << "it->account_name: " << it->account_name << std::endl;
//   }

   return 0;
}