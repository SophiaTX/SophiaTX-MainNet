cmake_minimum_required(VERSION 3.12)
project(test)

set(CMAKE_CXX_STANDARD 14)

add_executable( test main.cpp)

target_include_directories( test
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

target_link_libraries( test ${Boost_LIBRARIES} dl)
target_include_directories( test PUBLIC ${Boost_INCLUDE_DIR})

