file(GLOB HEADERS "include/sophiatx/account_statistics/*.hpp")

add_library( sophiatx_account_statistics
             account_statistics_plugin.cpp
             account_statistics_api.cpp
           )

target_link_libraries( sophiatx_account_statistics sophiatx_chain sophiatx_protocol sophiatx_app )
target_include_directories( sophiatx_account_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   sophiatx_account_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
