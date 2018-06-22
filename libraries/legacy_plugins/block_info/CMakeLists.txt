file(GLOB HEADERS "include/sophiatx/plugins/block_info/*.hpp")

add_library( sophiatx_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( sophiatx_block_info sophiatx_app sophiatx_chain sophiatx_protocol fc )
target_include_directories( sophiatx_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
