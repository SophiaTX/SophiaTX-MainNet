add_executable( smart_contracts_tests main.cpp db_resource_pool_tests.cpp generic_object_tests.cpp )
target_link_libraries( smart_contracts_tests smart_contracts sophiatx_utilities ${PLATFORM_SPECIFIC_LIBS})


