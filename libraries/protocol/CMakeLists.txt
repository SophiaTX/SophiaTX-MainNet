file(GLOB HEADERS "include/sophiatx/protocol/*.hpp")

set( hardfork_hpp_file "${CMAKE_CURRENT_BINARY_DIR}/include/sophiatx/protocol/hardfork.hpp" )
add_custom_target( build_hardfork_hpp
  COMMAND "${CMAKE_SOURCE_DIR}/programs/build_helpers/cat_parts.py" "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file} )

set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/sophiatx/protocol/hardfork.hpp" PROPERTIES GENERATED TRUE )

## SORT .cpp by most likely to change / break compile
add_library( sophiatx_protocol

             sophiatx_operations.cpp
             types.cpp
             authority.cpp
             operations.cpp
             sign_state.cpp
             operation_util_impl.cpp
             transaction.cpp
             block.cpp
             asset.cpp
             asset_symbol.cpp
             version.cpp
             get_config.cpp

             ${HEADERS}
             ${hardfork_hpp_file}
             "${CMAKE_CURRENT_BINARY_DIR}/include/sophiatx/protocol/hardfork.hpp"
           )

add_dependencies( sophiatx_protocol build_hardfork_hpp )
target_link_libraries( sophiatx_protocol fc )
target_include_directories( sophiatx_protocol
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      sophiatx_protocol PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

INSTALL( TARGETS
   sophiatx_protocol

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

INSTALL( FILES ${HEADERS} DESTINATION "include/sophiatx/protocol" )
