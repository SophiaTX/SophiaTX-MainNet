file(GLOB HEADERS "include/sophiatx/delayed_node/*.hpp")

add_library( sophiatx_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( sophiatx_delayed_node sophiatx_chain sophiatx_protocol sophiatx_app )
target_include_directories( sophiatx_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   sophiatx_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
