file(GLOB HEADERS "include/sophiatx/plugins/multiparty_messaging/*.hpp")

add_library( multiparty_messaging_plugin
             multiparty_messaging_plugin.cpp
             multiparty_messaging_api.cpp
           )

target_link_libraries( multiparty_messaging_plugin chain_plugin sophiatx_chain sophiatx_protocol sophiatx_utilities json_rpc_plugin )
target_include_directories( multiparty_messaging_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

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
