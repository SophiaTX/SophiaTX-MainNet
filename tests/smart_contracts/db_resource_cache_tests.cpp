#include <boost/test/unit_test.hpp>
#include <fc/exception/exception.hpp>
#include <fc/lru_cache.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <boost/filesystem/operations.hpp>

BOOST_AUTO_TEST_SUITE( smart_contracts_tests )

BOOST_AUTO_TEST_CASE( db_resource_cache_tests )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: db_resource_cache" );
      boost::filesystem::path data_dir(boost::filesystem::current_path() / "databases/");
      if (boost::filesystem::exists(data_dir) == false) {
         boost::filesystem::create_directory(data_dir);
      }

      constexpr uint32_t max_pool_size = 1;
      auto DbCreator = [data_dir = data_dir.generic_string()](const std::string& db_name) { return std::make_shared<SQLite::Database>(data_dir + db_name, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE); };
      fc::LruCache<std::string, std::shared_ptr<SQLite::Database>, decltype(DbCreator)> resourcePool(max_pool_size, std::chrono::milliseconds(100), DbCreator);

      std::shared_ptr<SQLite::Database> acc1_db = resourcePool.emplace("acc1", "acc1.db");
      auto& acc2_db                             = resourcePool.emplace("acc2", "acc2.db");

      BOOST_TEST_MESSAGE( "--- Test if created database have different files" );
      BOOST_CHECK_NE( acc1_db->getFilename(), acc2_db->getFilename() );


      BOOST_TEST_MESSAGE( "--- Test if created database have different files" );
      BOOST_CHECK_NE( acc1_db->getFilename(), acc2_db->getFilename() );

      BOOST_TEST_MESSAGE( "--- Test if the least used resource was deleted" );
      BOOST_CHECK_EQUAL( resourcePool.size(), max_pool_size );

      boost::filesystem::remove_all(data_dir);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()