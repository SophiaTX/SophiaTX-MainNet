#file(GLOB HEADERS "include/sophiatx/egenesis/*.hpp")


add_library( sophiatx_egenesis_none
egenesis_none.cpp
include/sophiatx/egenesis/egenesis.hpp
)

target_link_libraries( sophiatx_egenesis_none sophiatx_protocol sophiatx_chain fc )
target_include_directories( sophiatx_egenesis_none
PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_executable( embed_genesis
embed_genesis.cpp
)

target_link_libraries( embed_genesis sophiatx_protocol sophiatx_chain sophiatx_egenesis_none fc )

set( embed_genesis_args
-t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
)

if( SOPHIATX_EGENESIS_JSON )
MESSAGE( STATUS "egenesis: " ${SOPHIATX_EGENESIS_JSON} )
list( APPEND embed_genesis_args --genesis-json "${SOPHIATX_EGENESIS_JSON}" )
else( SOPHIATX_EGENESIS_JSON )
MESSAGE( STATUS "egenesis: none" )
endif( SOPHIATX_EGENESIS_JSON )

MESSAGE( STATUS "embed_genesis_args: " ${embed_genesis_args} )

add_custom_command(
OUTPUT
"${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMAND embed_genesis ${embed_genesis_args}
DEPENDS
"${SOPHIATX_EGENESIS_JSON}"
"${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
embed_genesis )

add_library( sophiatx_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp" include/sophiatx/egenesis/egenesis.hpp )

target_link_libraries( sophiatx_egenesis_full sophiatx_protocol sophiatx_chain fc )

target_include_directories( sophiatx_egenesis_full PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

