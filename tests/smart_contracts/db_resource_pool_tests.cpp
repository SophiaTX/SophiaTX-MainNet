#include <boost/test/unit_test.hpp>
#include <fc/exception/exception.hpp>
#include <sophiatx/smart_contracts/db_resource_pool.hpp>
#include <boost/filesystem/operations.hpp>
#include <iostream>

using namespace sophiatx::smart_contracts;

BOOST_AUTO_TEST_SUITE( smart_contracts_tests )

BOOST_AUTO_TEST_CASE( db_resource_pool_tests )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: db_resource_pool" );
      boost::filesystem::path data_dir(boost::filesystem::current_path() / "databases/");
      constexpr uint32_t max_pool_size = 3;

      db_resource_pool resource_pool(data_dir, max_pool_size);
      resource_pool.create_resource("acc1");
      resource_pool.create_resource("acc2");
      resource_pool.create_resource("acc3");
      resource_pool.create_resource("acc4");


      BOOST_TEST_MESSAGE( "--- Test max number of resources enabled in pool" );
      BOOST_CHECK_EQUAL(max_pool_size, resource_pool.size());


      BOOST_TEST_MESSAGE( "--- Test if the least used resource was deleted" );
      BOOST_CHECK_THROW( resource_pool.get_resource("acc1", false/*do not create resource*/), sophiatx::smart_contracts::resource_error );


      BOOST_TEST_MESSAGE( "--- Test failure when trying to get non-existing resource" );
      BOOST_CHECK_THROW( resource_pool.get_resource("acc5", false/*do not create resource*/), sophiatx::smart_contracts::resource_error );


      BOOST_TEST_MESSAGE( "--- Test creating non-existing resource with get_resource method" );
      BOOST_CHECK_NO_THROW( resource_pool.get_resource("acc5") );


      // There are now resources mapped to accounts acc3, acc4, acc5 that are ordered(according to the last access_time): acc3, acc4, acc5
      resource_pool.get_resource("acc3", false/*do not create resource*/); // use resource acc3 -> updates last access_time
      // Now there should be present resources in order: acc4, acc5, acc3
      resource_pool.create_resource("acc6");                               // create new resource for acc6
      // Now there should be present resources in order: acc5, acc3, acc6
      BOOST_TEST_MESSAGE( "--- Test if last_access time of resource was updated when get_resource called" );
      BOOST_CHECK_NO_THROW( resource_pool.get_resource("acc3") );


      BOOST_TEST_MESSAGE( "--- Test if the least used resource was deleted" );
      BOOST_CHECK_THROW( resource_pool.get_resource("acc4", false/*do not create resource*/), sophiatx::smart_contracts::resource_error );

      boost::filesystem::remove_all(data_dir);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()