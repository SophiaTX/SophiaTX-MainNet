file(GLOB HEADERS "include/sophiatx/chain/*.hpp" "include/sophiatx/chain/util/*.hpp" "include/sophiatx/chain/database/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( sophiatx_chain

             # As database takes the longest to compile, start it first
             database.cpp
             database_interface.cpp
             hybrid_database.cpp
             remote_db_api.cpp

             sophiatx_evaluator.cpp
             sophiatx_objects.cpp

             witness_schedule.cpp
             fork_database.cpp

             shared_authority.cpp
             block_log.cpp
             economics.cpp

             util/impacted.cpp

             ${HEADERS}
       )

target_link_libraries( sophiatx_chain sophiatx_protocol fc chainbase sophiatx_schema ${PATCH_MERGE_LIB} )
target_include_directories( sophiatx_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )
if (USE_PCH)
    cotire(sophiatx_chain)
endif(USE_PCH)

if( CLANG_TIDY_EXE )
   set_target_properties(
      sophiatx_chain PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   sophiatx_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/sophiatx/chain" )
