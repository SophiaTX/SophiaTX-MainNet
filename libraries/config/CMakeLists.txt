file(GLOB HEADERS "include/sophiatx/config/*.hpp")


add_library( sophiatx_config
        config.cpp
        include/sophiatx/config/config.hpp
        )
target_link_libraries( sophiatx_config fc )


target_include_directories( sophiatx_config
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if (USE_PCH)
    cotire(sophiatx_config)
endif(USE_PCH)

if( CLANG_TIDY_EXE )
    set_target_properties(
            sophiatx_config PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif( CLANG_TIDY_EXE )

if(MSVC)
    set_source_files_properties( config.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)



INSTALL( TARGETS
        sophiatx_config

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
INSTALL( FILES ${HEADERS} DESTINATION "include/sophiatx/config" )
