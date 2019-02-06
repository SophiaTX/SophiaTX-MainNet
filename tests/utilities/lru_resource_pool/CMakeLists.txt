file(GLOB SOURCE_FILES "*.cpp")
add_executable( utility_lru_resource_pool_test main.cpp lru_resource_pool_tests.cpp )
target_link_libraries( utility_lru_resource_pool_test sophiatx_utilities ${PLATFORM_SPECIFIC_LIBS})


