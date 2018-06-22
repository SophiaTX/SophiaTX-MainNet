file(GLOB HEADERS "include/sophiatx/plugins/auth_util/*.hpp")

add_library( sophiatx_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( sophiatx_auth_util sophiatx_app sophiatx_chain sophiatx_protocol fc )
target_include_directories( sophiatx_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
