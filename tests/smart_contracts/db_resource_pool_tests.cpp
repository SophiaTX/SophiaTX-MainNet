#include <boost/test/unit_test.hpp>
#include <fc/exception/exception.hpp>
#include <sophiatx/utilities/lru_resource_pool.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <boost/filesystem/operations.hpp>

BOOST_AUTO_TEST_SUITE( smart_contracts_tests )

BOOST_AUTO_TEST_CASE( db_resource_pool_tests )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: db_resource_pool" );
      boost::filesystem::path data_dir(boost::filesystem::current_path() / "databases/");
      if (boost::filesystem::exists(data_dir) == false) {
         boost::filesystem::create_directory(data_dir);
      }

      constexpr uint32_t max_pool_size = 1;
      sophiatx::utilities::LruResourcePool<std::string, SQLite::Database> resourcePool(max_pool_size);


      SQLite::Database& acc1_db = resourcePool.getResourceAut("acc1", data_dir.generic_string() + "acc1.db", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
      auto& acc2_db             = resourcePool.getResourceAut("acc2", data_dir.generic_string() + "acc2.db", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);


      BOOST_TEST_MESSAGE( "--- Test if created database have different files" );
      BOOST_CHECK_NE( acc1_db.getFilename(), acc2_db.getFilename() );

      BOOST_TEST_MESSAGE( "--- Test if the least used resource was deleted" );
      BOOST_CHECK_EQUAL( resourcePool.size(), max_pool_size );
      BOOST_CHECK_EQUAL( resourcePool.getResource("acc1").has_value(), false );

      boost::filesystem::remove_all(data_dir);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()