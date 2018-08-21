file(GLOB HEADERS "include/sophiatx/plugins/subscribe_api/*.hpp")
add_library( subscribe_api_plugin
        subscribe_api_plugin.cpp
        subscribe_api.cpp
        )

target_link_libraries( subscribe_api_plugin json_rpc_plugin chain_plugin sophiatx_chain sophiatx_protocol )
target_include_directories( subscribe_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
    set_target_properties(
            subscribe_api_plugin PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif( CLANG_TIDY_EXE )

install( TARGETS
        subscribe_api_plugin

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
