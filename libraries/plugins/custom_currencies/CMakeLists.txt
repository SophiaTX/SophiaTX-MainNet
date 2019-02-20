file(GLOB HEADERS "include/sophiatx/plugins/custom_currencies/*.hpp")

add_library(custom_currencies_plugin
            custom_currencies_plugin.cpp
            custom_currencies_api.cpp
           )

target_link_libraries(custom_currencies_plugin
        alexandria_api_plugin chain_plugin sophiatx_chain sophiatx_protocol sophiatx_utilities json_rpc_plugin)

target_include_directories(custom_currencies_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (CLANG_TIDY_EXE)
    set_target_properties(
            custom_currencies_plugin PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif (CLANG_TIDY_EXE)

install(TARGETS
        custom_currencies_plugin

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
