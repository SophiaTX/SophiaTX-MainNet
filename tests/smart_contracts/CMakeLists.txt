file(GLOB HEADERS "include/sophiatx/smart_contracts/*.hpp")

file(GLOB UNIT_TESTS "*.cpp")
add_executable( smart_contracts_tests ${UNIT_TESTS} )
target_link_libraries( smart_contracts_tests smart_contracts ${PLATFORM_SPECIFIC_LIBS})
target_include_directories( smart_contracts_tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )