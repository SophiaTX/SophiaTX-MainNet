file(GLOB HEADERS "include/sophiatx/plugins/template/*.hpp")

add_library( template_plugin
             template_plugin.cpp
             template_api.cpp
           )

target_link_libraries( template_plugin chain_plugin sophiatx_chain sophiatx_protocol sophiatx_utilities json_rpc_plugin )
target_include_directories( template_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      template_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   template_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
