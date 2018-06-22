file(GLOB HEADERS "include/sophiatx/private_message/*.hpp")

add_library( sophiatx_private_message
             private_message_plugin.cpp
           )

target_link_libraries( sophiatx_private_message sophiatx_chain sophiatx_protocol sophiatx_app )
target_include_directories( sophiatx_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   sophiatx_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
