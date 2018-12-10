
#include <iostream>
#include <sophiatx/smart_contracts/db_resource_pool.h>

int main() {
   using namespace sophiatx::smart_contracts;

   std::cout << "hello smart contract" << std::endl;

   db_resource_pool connections_pool(3);

   try {
   connections_pool.get_resource("acc1");
   connections_pool.get_resource("acc1");
   connections_pool.get_resource("acc2");
   connections_pool.get_resource("acc3");

   connections_pool.test();

   connections_pool.get_resource("acc1");
   connections_pool.get_resource("acc4");

   connections_pool.test();




   }
   catch (const resource_error& e) {
      std::cout << "resource_error: " << e.what() << std::endl;
   }

   SQLite::Database& db = connections_pool.get_resource("acc3");

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

   return 0;
}