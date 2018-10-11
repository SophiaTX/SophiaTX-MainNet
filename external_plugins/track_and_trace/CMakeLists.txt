file(GLOB HEADERS "include/sophiatx/plugins/track_and_trace/*.hpp")

add_library( track_and_trace_plugin
             track_and_trace_plugin.cpp
             track_and_trace_api.cpp
           )

target_link_libraries( track_and_trace_plugin chain_plugin sophiatx_chain sophiatx_protocol sophiatx_utilities json_rpc_plugin )
target_include_directories( track_and_trace_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      track_and_trace_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   track_and_trace_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
