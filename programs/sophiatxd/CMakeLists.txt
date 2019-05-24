add_executable( sophiatxd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling sophiatxd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( SOPHIATX_STATIC_BUILD )
   target_link_libraries( sophiatxd PRIVATE
      "-static-libstdc++ -static-libgcc"
      appbase
      sophiatx_utilities
      sophiatx_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
else( SOPHIATX_STATIC_BUILD )
   target_link_libraries( sophiatxd PRIVATE
      appbase
      sophiatx_utilities
      sophiatx_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
endif( SOPHIATX_STATIC_BUILD )

if( CLANG_TIDY_EXE )
   set_target_properties(
      sophiatxd PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

set(APP_INSTALL_DIR "/usr/local/bin" CACHE PATH "Aplication install directory")
set(CONF_INSTALL_DIR "/etc" CACHE PATH "Configuration install directory")

install(PROGRAMS ${CMAKE_BINARY_DIR}/programs/sophiatxd/sophiatxd DESTINATION ${APP_INSTALL_DIR} COMPONENT sophiatxd)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/etc/ DESTINATION ${CONF_INSTALL_DIR} COMPONENT sophiatxd)
