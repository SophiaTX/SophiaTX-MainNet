file(GLOB UTILITIES_TESTS "lru_cache/*.cpp")
add_executable( utilities_tests ${UTILITIES_TESTS} )
target_link_libraries( utilities_tests sophiatx_utilities ${PLATFORM_SPECIFIC_LIBS})