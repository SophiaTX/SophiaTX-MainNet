file(GLOB HEADERS "include/sophiatx/remote_db/*.hpp")

add_library( sophiatx_remote_db include/sophiatx/remote_db/remote_db.hpp )
target_link_libraries( sophiatx_remote_db fc )
target_include_directories( sophiatx_remote_db PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )


if( CLANG_TIDY_EXE )
    set_target_properties(
            sophiatx_remote_db PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif( CLANG_TIDY_EXE )

