list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libraries/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision(GIT_REFSPEC SOPHIATX_GIT_REVISION_SHA)
get_git_unix_timestamp(SOPHIATX_GIT_REVISION_UNIX_TIMESTAMP)
git_describe(SOPHIATX_GIT_REVISION_DESCRIPTION --tags)
if(NOT SOPHIATX_GIT_REVISION_DESCRIPTION)
    set(SOPHIATX_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT SOPHIATX_GIT_REVISION_DESCRIPTION)

file(GLOB HEADERS "include/sophiatx/utilities/*.hpp")

set(sources
   benchmark_dumper.cpp
   key_conversion.cpp
   string_escape.cpp
   tempdir.cpp
   words.cpp
   logging_config.cpp
   sys_logger.cpp
   ${HEADERS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")

if(MSVC)
	find_library (PSAPI Psapi)
	list( APPEND PLATFORM_SPECIFIC_LIBS PSAPI )
endif()

add_library( sophiatx_utilities
             ${sources}
             ${HEADERS} )
target_link_libraries( sophiatx_utilities fc ${PLATFORM_SPECIFIC_LIBS})
target_include_directories( sophiatx_utilities
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
if (USE_PCH)
  set_target_properties(sophiatx_utilities PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(sophiatx_utilities)
endif(USE_PCH)

if( CLANG_TIDY_EXE )
   set_target_properties(
      sophiatx_utilities PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   sophiatx_utilities

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/sophiatx/utilities" )
