add_executable( sophiatx_cli_wallet main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling sophiatx_cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( SOPHIATX_STATIC_BUILD )
   target_link_libraries( sophiatx_cli_wallet PRIVATE
                          "-static-libstdc++ -static-libgcc"
                          graphene_net sophiatx_chain sophiatx_protocol sophiatx_egenesis_full sophiatx_utilities sophiatx_wallet alexandria_api_plugin fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${readline_libraries})
else( SOPHIATX_STATIC_BUILD )
   target_link_libraries( sophiatx_cli_wallet PRIVATE
                          graphene_net sophiatx_chain sophiatx_protocol sophiatx_egenesis_full sophiatx_utilities sophiatx_wallet alexandria_api_plugin fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
endif( SOPHIATX_STATIC_BUILD )

if( CLANG_TIDY_EXE )
   set_target_properties(
      sophiatx_cli_wallet PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

set(APP_INSTALL_DIR "/usr/local/bin" CACHE PATH "Aplication install directory")
set(CONF_INSTALL_DIR "/etc" CACHE PATH "Configuration install directory")

install(PROGRAMS ${CMAKE_BINARY_DIR}/programs/cli_wallet/sophiatx_cli_wallet DESTINATION ${APP_INSTALL_DIR} COMPONENT sophiatx_cli_wallet)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/etc/ DESTINATION ${CONF_INSTALL_DIR} COMPONENT sophiatx_cli_wallet)