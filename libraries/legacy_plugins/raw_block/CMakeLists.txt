file(GLOB HEADERS "include/sophiatx/plugins/raw_block/*.hpp")

add_library( sophiatx_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( sophiatx_raw_block sophiatx_app sophiatx_chain sophiatx_protocol fc )
target_include_directories( sophiatx_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
