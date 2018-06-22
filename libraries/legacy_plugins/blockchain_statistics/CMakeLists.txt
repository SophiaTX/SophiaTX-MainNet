file(GLOB HEADERS "include/sophiatx/blockchain_statistics/*.hpp")

add_library( sophiatx_blockchain_statistics
             blockchain_statistics_plugin.cpp
             blockchain_statistics_api.cpp
           )

target_link_libraries( sophiatx_blockchain_statistics sophiatx_chain sophiatx_protocol sophiatx_app )
target_include_directories( sophiatx_blockchain_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   sophiatx_blockchain_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
