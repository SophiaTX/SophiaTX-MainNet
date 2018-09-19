if(APPLE)
    add_library( alexandria MODULE alexandria.cpp alexandria.hpp )
    add_library( alexandria_shared SHARED alexandria.cpp alexandria.hpp )
else(APPLE)
    add_library( alexandria MODULE alexandria.cpp alexandria.hpp )
endif(APPLE)

if( UNIX AND NOT APPLE )
    set(rt_library rt )
endif()

set_target_properties(alexandria PROPERTIES PUBLIC_HEADER alexandria.hpp)
if(APPLE)
   set_target_properties(alexandria_shared PROPERTIES PUBLIC_HEADER alexandria.hpp)
endif(APPLE)   

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling alexandria with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()


target_link_libraries( alexandria PRIVATE
        sophiatx_protocol sophiatx_utilities fc
        ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
if(APPLE)
        target_link_libraries( alexandria_shared PRIVATE sophiatx_protocol sophiatx_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
endif(APPLE)

if( CLANG_TIDY_EXE )
    set_target_properties(
            alexandria PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif( CLANG_TIDY_EXE )




install( TARGETS
        alexandria
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION lib
        )
