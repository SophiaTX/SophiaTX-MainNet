file(GLOB HEADERS "include/sophiatx/schema/*.hpp")
add_library( sophiatx_schema schema.cpp ${HEADERS} )
target_link_libraries( sophiatx_schema fc )
target_include_directories( sophiatx_schema PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   sophiatx_schema

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/sophiatx/schema" )
