add_subdirectory( cli_wallet )
add_subdirectory( sophiatxd )
#add_subdirectory( delayed_node )
add_subdirectory( js_operation_serializer )
add_subdirectory( size_checker )
add_subdirectory( util )
add_subdirectory( alexandria_deamon )

if(BUILD_PIC)
    add_subdirectory( alexandria )
    add_subdirectory( alexandriaJNI )
endif(BUILD_PIC)

