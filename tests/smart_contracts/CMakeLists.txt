file(GLOB HEADERS "include/sophiatx/smart_contracts/*.hpp")

add_executable( db_resource_pool_tests main.cpp db_resource_pool_tests.cpp )
target_link_libraries( db_resource_pool_tests smart_contracts db_fixture chainbase sophiatx_chain sophiatx_protocol account_history_plugin witness_plugin debug_node_plugin fc ${PLATFORM_SPECIFIC_LIBS})
target_include_directories( db_resource_pool_tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )