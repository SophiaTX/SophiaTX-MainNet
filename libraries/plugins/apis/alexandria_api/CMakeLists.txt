file(GLOB HEADERS "include/sophiatx/plugins/alexandria_api/*.hpp")

add_library(
   alexandria_api_plugin
   alexandria_api.cpp
   alexandria_api_plugin.cpp
   alexandria_api_impl.cpp
   alexandria_api_operations.cpp
)

target_link_libraries(
   alexandria_api_plugin
   chain_plugin
   json_rpc_plugin
   block_api_plugin
   account_by_key_api_plugin
   account_history_api_plugin
   database_api_plugin
   block_api_plugin
   network_broadcast_api_plugin
   witness_api_plugin
   custom_api_plugin
   subscribe_api_plugin )

target_include_directories( alexandria_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      alexandria_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   alexandria_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
