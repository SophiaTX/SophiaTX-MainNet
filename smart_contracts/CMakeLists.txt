file(GLOB HEADERS "include/sophiatx/smart_contracts/*.hpp")

add_library( smart_contracts db_resource.cpp db_resource_pool.cpp)
target_link_libraries( smart_contracts fc SQLiteCpp)
target_include_directories( smart_contracts
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

option(SMART_CONTRACTS_BUILD_EXAMPLES "Build examples." OFF)
if (SMART_CONTRACTS_BUILD_EXAMPLES)
    add_executable( sqlite_wrapper_example examples/sqlite_wrapper_example.cpp)
    target_link_libraries( sqlite_wrapper_example fc smart_contracts)
else (SMART_CONTRACTS_BUILD_EXAMPLES)
    message(STATUS "SMART_CONTRACTS_BUILD_EXAMPLES OFF")
endif (SMART_CONTRACTS_BUILD_EXAMPLES)

if( CLANG_TIDY_EXE )
    set_target_properties(
        smart_contracts PROPERTIES
        CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif( CLANG_TIDY_EXE )

install( TARGETS
    smart_contracts
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)