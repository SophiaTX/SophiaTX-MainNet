#include <boost/test/unit_test.hpp>
#include <fc/exception/exception.hpp>
#include <fc/timed_lru_cache.hpp>
#include <boost/filesystem/operations.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE( timed_lru_cache_tests )

BOOST_AUTO_TEST_CASE( timed_lru_cache_tests )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: timed_lru_cache" );
      constexpr uint32_t max_pool_size = 3;

      fc::TimedLruCache<std::string, std::string> resourcePool(max_pool_size, std::chrono::milliseconds(100));
      resourcePool.emplace("key1", "value1");
      resourcePool.emplace("key2", "value2");
      resourcePool.emplace("key3", "value3");
      resourcePool.emplace("key4", "value4");


      BOOST_TEST_MESSAGE( "--- Test max number of resources enabled in pool" );
      BOOST_CHECK_EQUAL(max_pool_size, resourcePool.size());


      BOOST_TEST_MESSAGE( "--- Test if the least used resource was deleted" );
      BOOST_CHECK( !resourcePool.get("key1") );


      BOOST_TEST_MESSAGE( "--- Test trying to get non-existing resource" );
      BOOST_CHECK( !resourcePool.get("key5") );


      BOOST_TEST_MESSAGE( "--- Test automatic creation of non-existing resource with getAut method" );
      BOOST_CHECK_EQUAL( resourcePool.emplace("key6", "value6"), "value6" );

      // There are now resources mapped to key key3, key4, key6 that are ordered(according to the last access_time): key3, key4, key6
      resourcePool.get("key3"); // use resource key3 -> updates last access_time

      // Now there should be present resources in order: key4, key6, key3
      std::string& createdResource = resourcePool.emplace("key7", "value7");  // create new resource for key7
      BOOST_TEST_MESSAGE( "--- Test if newly created resource original value" );
      BOOST_CHECK_EQUAL( resourcePool.get("key7").value(), "value7");


      // Now there should be present resources in order: key6, key3, key7
      BOOST_TEST_MESSAGE( "--- Test if last_access time of resource was updated when get called" );
      BOOST_CHECK( resourcePool.get("key3") );


      BOOST_TEST_MESSAGE( "--- Test if the least used resource was deleted" );
      BOOST_CHECK( !resourcePool.get("key4"));

      // Change internal value of newly created resource
      createdResource = "here we go changing the value";
      BOOST_TEST_MESSAGE( "--- Test if newly created resource value was changed" );
      BOOST_CHECK_EQUAL( resourcePool.get("key7").value(), "here we go changing the value");

      // Change internal value of resource obtained by get method
      boost::optional<std::string&> foundResourceOpt = resourcePool.get("key7");
      std::string& foundResource = foundResourceOpt.get();
      foundResource = "Change me again";
      BOOST_TEST_MESSAGE( "--- Test if obtained resource value was changed" );
      BOOST_CHECK_EQUAL( resourcePool.get("key7").value(), "Change me again");

      // Get resource by method, which automatically creates new resource, if there is none mapped to the provided key
      std::string& foundResource2 = resourcePool.emplace("key7", "default new value");
      BOOST_TEST_MESSAGE( "--- Test if getAut did not create new resource if there exist one with provided key" );
      BOOST_CHECK_EQUAL( resourcePool.get("key7").value(), "Change me again");

      // Change internal value of resource obtained by getAut method
      foundResource2 = "And again";
      BOOST_TEST_MESSAGE( "--- Test if obtained resource value was changed" );
      BOOST_CHECK_EQUAL( resourcePool.get("key7").value(), "And again");
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()