file(GLOB HEADERS "include/sophiatx/plugins/test_plugin/*.h")

add_library( test_external_plugin SHARED TestPlugin.cpp TestPluginApi.cpp)

target_include_directories( test_external_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

target_link_libraries( test_external_plugin ${Boost_LIBRARIES} chain_plugin)

if( CLANG_TIDY_EXE )
   set_target_properties(
           test_external_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   test_external_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
