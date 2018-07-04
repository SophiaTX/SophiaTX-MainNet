file(GLOB HEADERS "include/sophiatx/chain_plugin/*.hpp")
file(GLOB EGENESIS_HEADERS "../../egenesis/include/sophiatx/egenesis/*.hpp")

add_library( chain_plugin
             chain_plugin.cpp
             ${HEADERS}
             ${EGENESIS_HEADERS}
        )

target_link_libraries( chain_plugin sophiatx_chain sophiatx_egenesis_full appbase sophiatx_utilities )
target_include_directories( chain_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../egenesis/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      chain_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   chain_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/sophiatx/chain_plugin" )
