#include <boost/test/unit_test.hpp>
#include <fc/exception/exception.hpp>
#include <sophiatx/smart_contracts/db_resource_pool.hpp>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>
#include <iostream>
#include <SQLiteCpp/VariadicBind.h>

using namespace sophiatx::smart_contracts;

BOOST_AUTO_TEST_SUITE( smart_contracts_tests )

BOOST_AUTO_TEST_CASE( generic_object_tests )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: generic_object" );

      // Creates generic object from json
      const std::string input_json("{\"name\":\"foo\",\"friends\":[\"alice\",\"bob\"],\"school_id\":\"1\"}");
      fc::mutable_variant_object generic_object = fc::json::from_string( input_json ).get_object();
      BOOST_CHECK_EQUAL(input_json, fc::json::to_string(generic_object));

      SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

      // Creates table school
      {
         db.exec("DROP TABLE IF EXISTS school");
         db.exec("CREATE TABLE school (id INTEGER PRIMARY KEY, name TEXT, city TEXT)");

         SQLite::Statement query(db, "INSERT INTO school VALUES (?, ?, ?)");

         // Binds fc::variant value. Type is automatically detected so it is up to programmer to make sure that fc::variant inner type and sql column type are the same
         SQLite::bind(query, generic_object["school_id"], "STU BA", "Bratislava");
         int modifiedRows = query.exec();
         BOOST_CHECK_EQUAL(modifiedRows, 1);
      }

      {
         SQLite::Statement query(db, "SELECT * FROM school WHERE id = :school_id");
         // Binds fc::mutable_variant_object. It is up to programmer to make sure that mutable_variant_object has member with same key as parameter name from prepared statement
         // and it has also the same type as specified sql column
         query.bind(generic_object);

         bool executeResult = query.executeStep();
         BOOST_CHECK_EQUAL(executeResult, true);

         if (executeResult)
         {
            fc::variant expectedVariant = fc::variant("STU BA");
            BOOST_CHECK_EQUAL(query.getColumn("name").getVariant().as_string(), expectedVariant.as_string());
            // Adds new fc::variant to the generic object
            generic_object.set("school_name", query.getColumn("name").getVariant());

            fc::mutable_variant_object expectedVariantObject = fc::json::from_string( "{\"id\":1,\"name\":\"STU BA\",\"city\":\"Bratislava\"}" ).get_object();
            BOOST_CHECK_EQUAL(fc::json::to_string(query.getRow()), fc::json::to_string(expectedVariantObject));
            // Adds new fc::mutable_variant_object to the generic object
            generic_object.set("school", query.getRow());
         }

         // Expected serialized json of generic object after changes made to the object
         const std::string final_json("{\"name\":\"foo\",\"friends\":[\"alice\",\"bob\"],\"school_id\":\"1\",\"school_name\":\"STU BA\",\"school\":{\"id\":1,\"name\":\"STU BA\",\"city\":\"Bratislava\"}}");
         BOOST_CHECK_EQUAL(fc::json::to_string(generic_object), final_json);
      }


      {
         db.exec("DROP TABLE IF EXISTS failure");
         db.exec("CREATE TABLE failure (id INTEGER PRIMARY KEY, description TEXT)");

         SQLite::Statement query(db, "INSERT INTO failure VALUES (?, ?)");

         // Tries to bind string(generic_object["name"]) to the integer(failure.id) - not possible
         SQLite::bind(query, generic_object["name"], "This is epic fail");
         BOOST_REQUIRE_THROW(query.exec(), SQLite::Exception);

         // Tries to bind members of fc::mutable_variant_object to the unnamed statement parameters - not possible
         query.reset();
         BOOST_REQUIRE_THROW(query.bind(generic_object), SQLite::Exception);

         // Tries to bind non existing member "id" of the fc::mutable_variant_object
         SQLite::Statement query2(db, "INSERT INTO failure VALUES (:id, :description)");
         BOOST_REQUIRE_THROW(query2.bind(generic_object), SQLite::Exception);
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()