#include <iostream>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/VariadicBind.h>

void create_databases(SQLite::Database& db);
void print_table_person(SQLite::Database& db);
void print_table_info(SQLite::Database& db);

int main() {
   try {
      SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
      // Creates, fills and prints initial sqlite databases content
      create_databases(db);

      // Standard variable types bind examples - for more detailed example see SophiaTX/libraries/SQLiteCpp/examples/example1/main.cpp
      // Use "?" only if you use parameter indexing or generic bind SQLite::bind(query, param1, param2, ...)
      std::cout << "Standard variable types bind examples - for more detailed example see SophiaTX/libraries/SQLiteCpp/examples/example1/main.cpp" << std::endl << std::endl;
      {
         SQLite::Statement query1(db, "INSERT INTO person VALUES (?, ?)");
         query1.bind(1, 1);      // parameter indexing
         query1.bind(2, "bar");  // parameter indexing
         query1.exec();
         std::cout << "Parameter indexing bind sample INSERT INTO person VALUES (?, ?):" << std::endl;
         print_table_person(db);

         SQLite::Statement query2(db, "INSERT INTO person VALUES (?, ?)");
         SQLite::bind(query2, 2, "bob"); // generic bind
         query2.exec();
         std::cout << "Generic bind sample INSERT INTO person VALUES (?, ?):" << std::endl;
         print_table_person(db);
      }

      // Use ":VVV", "@VVV" or "$VVV" if you use parameter naming bind
      {
         SQLite::Statement query1(db, "INSERT INTO person VALUES (:new_person_id, @new_person_name)");
         query1.bind(":new_person_id", 3);          // parameter naming
         query1.bind("@new_person_name", "alice");  // parameter naming
         query1.exec();
         std::cout << "Parameter naming bind sample INSERT INTO person VALUES (:new_person_id, @new_person_name):" << std::endl;
         print_table_person(db);
      }



      // fc::mutable_variant_object, fc::variant_object, fc::variant variable types bind examples
      // Same rules for using "?", ":VVV", "@VVV" or "$VVV" as for standard types are applied - see description above
      std::cout << "fc::mutable_variant_object, fc::variant_object, fc::variant variable types bind examples" << std::endl << std::endl;
      {
         // version using generic bind of fc::mutable_variant_object members casted to standard types (parameter naming & indexing might be used as well)
         fc::mutable_variant_object generic_object1 = fc::json::from_string("{\"id\":4,\"name\":\"hello\"}").get_object();
         SQLite::Statement query1(db, "INSERT INTO person VALUES (?, ?)");
         SQLite::bind(query1, generic_object1["id"].as_int64(), generic_object1["name"].as_string());
         query1.exec();
         std::cout << "Generic bind using fc::mutable_variant_object members casted to standard types sample: INSERT INTO person VALUES (?, ?):" << std::endl;
         print_table_person(db);

         // simplified version using parameter indexing bind of fc::mutable_variant_object members - fc::variant (parameter naming & generic bind might be used as well)
         // !!! it is programmer responsibility to make sure that internal types of fc::mutable_variant_object members are the same as table columns types
         // There is automatic cast from fc::variant to its internal type which must be compatible with table column type
         fc::mutable_variant_object generic_object2 = fc::json::from_string("{\"id\":5,\"name\":\"world\"}").get_object();
         SQLite::Statement query2(db, "INSERT INTO person VALUES (?, ?)");
         query2.bind(1, generic_object2["id"]);
         query2.bind(2, generic_object2["name"]);
         query2.exec();
         std::cout << "Parameter indexing bind using fc::mutable_variant_object members(fc::variant type) sample: INSERT INTO person VALUES (?, ?):" << std::endl;
         print_table_person(db);

         // even more simplified version using bind of fc::mutable_variant_object(or fc::variant_object).
         // Only parameter naming bind(which is implicit in this case) is allowed for this version.
         // !!! it is programmer responsibility to make sure that fc::mutable_variant_object has members with the same keys as sql query
         // parameter names and that internal types of fc::mutable_variant_object members are the same as table columns types
         // There is automatic cast from fc::variant to its internal type which must be compatible with table column type
         fc::mutable_variant_object generic_object3 = fc::json::from_string("{\"id\":6,\"name\":\"SophiaTX\"}").get_object();
         SQLite::Statement query3(db, "INSERT INTO person VALUES ($id, $name)");
         query3.bind(generic_object3);
         query3.exec();
         std::cout << "Bind using directly fc::mutable_variant_object(or fc::variant_object): INSERT INTO person VALUES ($id, $name):" << std::endl;
         print_table_person(db);
      }
   }
   catch (const SQLite::Exception& e) {
      std::cout << "SQLite::Exception: " << e.what() << std::endl;
   }
   catch (const std::exception& e) {
      std::cout << "std::exception: " << e.what() << std::endl;
   }
   catch (...) {
      std::cout << "Unknown exception" << std::endl;
   }
}


// helpers functions
void create_databases(SQLite::Database& db) {
   db.exec("DROP TABLE IF EXISTS person");
   db.exec("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT)");
   SQLite::Statement query1(db, "INSERT INTO person VALUES (0, 'foo')");
   query1.exec();

   db.exec("DROP TABLE IF EXISTS info");
   db.exec("CREATE TABLE info (id INTEGER PRIMARY KEY, person_id INTEGER, age INTEGER, city TEXT)");
   SQLite::Statement query2(db, "INSERT INTO info VALUES (0, 0, 26, 'Bratislava')");
   query2.exec();

   std::cout << "Initial database content info:" << std::endl;
   print_table_person(db);
   print_table_info(db);
}

void print_table_person(SQLite::Database& db) {
   std::cout << "table 'person' content:" << std::endl;
   SQLite::Statement query3(db, "SELECT * FROM person");
   while (query3.executeStep()) {
      std::cout << fc::json::to_string(query3.getRow()) << std::endl;
   }
   std::cout << std::endl;
}

void print_table_info(SQLite::Database& db) {
   std::cout << "table 'info' content:" << std::endl;
   SQLite::Statement query4(db, "SELECT * FROM info");
   while (query4.executeStep()) {
      std::cout << fc::json::to_string(query4.getRow()) << std::endl;
   }
   std::cout << std::endl;
}