file(GLOB HEADERS "include/sophiatx/smart_contracts/*.hpp")

add_library( smart_contracts db_resource.cpp db_resource_pool.cpp)
target_link_libraries( smart_contracts SQLiteCpp sqlite3 fc)
target_include_directories( smart_contracts
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
