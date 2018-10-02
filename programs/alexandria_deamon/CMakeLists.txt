add_executable( alexandria_deamon main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling alexandria_deamon with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( sophiatx_STATIC_BUILD )
   target_link_libraries( alexandria_deamon PRIVATE
                          "-static-libstdc++ -static-libgcc"
                          graphene_net sophiatx_chain sophiatx_protocol sophiatx_utilities lib_alexandria condenser_api_plugin fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${readline_libraries})
else( sophiatx_STATIC_BUILD )
   target_link_libraries( alexandria_deamon PRIVATE
                          graphene_net sophiatx_chain sophiatx_protocol sophiatx_utilities lib_alexandria condenser_api_plugin fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
endif( sophiatx_STATIC_BUILD )

if( CLANG_TIDY_EXE )
   set_target_properties(
      alexandria_deamon PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   alexandria_deamon

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
