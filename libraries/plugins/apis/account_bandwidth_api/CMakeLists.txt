file(GLOB HEADERS "include/sophiatx/plugins/account_bandwidth_api/*.hpp")
add_library(
        account_bandwidth_api_plugin
        account_bandwidth_api_plugin.cpp
        account_bandwidth_api.cpp
        account_bandwidth_api_impl.cpp
        ${HEADERS}
        )

target_link_libraries(
        account_bandwidth_api_plugin
        json_rpc_plugin
        chain_plugin
        )

target_include_directories( account_bandwidth_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      account_bandwidth_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   account_bandwidth_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

install( FILES ${HEADERS} DESTINATION "include/sophiatx/account_bandwidth_api_plugin" )