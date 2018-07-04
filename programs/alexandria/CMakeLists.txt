add_library( alexandria MODULE alexandria.cpp alexandria.hpp )

if( UNIX AND NOT APPLE )
    set(rt_library rt )
endif()

set_target_properties(alexandria PROPERTIES PUBLIC_HEADER alexandria.hpp)

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling alexandria with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()


target_link_libraries( alexandria PRIVATE
        sophiatx_protocol sophiatx_utilities fc
        ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )


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
