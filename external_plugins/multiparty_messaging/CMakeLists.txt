file(GLOB HEADERS "include/sophiatx/plugins/multiparty_messaging/*.hpp")

add_library( multiparty_messaging_plugin SHARED
              multiparty_messaging_plugin.cpp
              multiparty_messaging_api.cpp
              multiparty_messaging_plugin_export.cpp
           )

target_link_libraries( multiparty_messaging_plugin alexandria_api_plugin chain_plugin sophiatx_chain sophiatx_protocol sophiatx_utilities json_rpc_plugin )
target_include_directories( multiparty_messaging_plugin
                            PUBLIC "include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      multiparty_messaging_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   multiparty_messaging_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
