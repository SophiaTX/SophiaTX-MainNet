# Defines SophiaTX library target.

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    MESSAGE(STATUS "Building with ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project( SophiaTX )
cmake_minimum_required( VERSION 3.2 )

set( BLOCKCHAIN_NAME "SophiaTX" )
set( CMAKE_CXX_STANDARD 14 )

set( GUI_CLIENT_EXECUTABLE_NAME SophiaTX )
set( CUSTOM_URL_SCHEME "gcs" )
set( INSTALLER_APP_ID "68ad7005-8eee-49c9-95ce-9eed97e5b347" )

OPTION( BUILD_PIC "Build with -fPIC flag (ON OR OFF)" ON)
if(BUILD_PIC)
    MESSAGE(STATUS "Building with -fPIC flag")
else(BUILD_PIC)
    MESSAGE(STATUS "Skipping libalexandria and external plugins build, enable with -DBUILD_PIC flag")
endif(BUILD_PIC)

if(WIN32)
  cmake_policy(SET CMP0054 OLD)
endif(WIN32)

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must DB at least 4.8!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang version must be at least 3.3!")
    endif()
endif()

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

if (USE_PCH)
  include (cotire)
endif(USE_PCH)

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/fc/CMakeModules" )
list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision( GIT_REFSPEC GIT_SHA2 )

SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS thread
                             date_time
                             filesystem
                             system
                             program_options
                             serialization
                             chrono
                             unit_test_framework
                             context
                             locale
                             coroutine)
SET( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

OPTION( BUILD_SOPHIATX_TESTNET "Build source for test network (ON OR OFF)" OFF )
MESSAGE( STATUS "BUILD_SOPHIATX_TESTNET: ${BUILD_SOPHIATX_TESTNET}" )
if( BUILD_SOPHIATX_TESTNET  )
  MESSAGE( STATUS "     " )
  MESSAGE( STATUS "             CONFIGURING FOR TEST NET             " )
  MESSAGE( STATUS "     " )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIS_TEST_NET" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIS_TEST_NET" )
endif()

OPTION( LOW_MEMORY_NODE "Build source for low memory node (ON OR OFF)" OFF )
MESSAGE( STATUS "LOW_MEMORY_NODE: ${LOW_MEMORY_NODE}" )
if( LOW_MEMORY_NODE )
  MESSAGE( STATUS "     " )
  MESSAGE( STATUS "             CONFIGURING FOR LOW MEMORY NODE      " )
  MESSAGE( STATUS "     " )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIS_LOW_MEM" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIS_LOW_MEM" )
endif()

OPTION( CHAINBASE_CHECK_LOCKING "Check locks in chainbase (ON or OFF)" ON )
MESSAGE( STATUS "CHAINBASE_CHECK_LOCKING: ${CHAINBASE_CHECK_LOCKING}" )
if( CHAINBASE_CHECK_LOCKING )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCHAINBASE_CHECK_LOCKING" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCHAINBASE_CHECK_LOCKING" )
endif()

OPTION( CLEAR_VOTES "Build source to clear old votes from memory" ON )
if( CLEAR_VOTES )
  MESSAGE( STATUS "   CONFIGURING TO CLEAR OLD VOTES FROM MEMORY" )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCLEAR_VOTES" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCLEAR_VOTES" )
endif()

OPTION( SKIP_BY_TX_ID "Skip ordering operation history by transaction id (ON or OFF)" OFF )
MESSAGE( STATUS "SKIP_BY_TX_ID: ${SKIP_BY_TX_ID}" )
if( SKIP_BY_TX_ID )
   SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSKIP_BY_TX_ID" )
   SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSKIP_BY_TX_ID" )
endif()

OPTION( SOPHIATX_STATIC_BUILD "Build sophiatxd as a static library (ON or OFF)" OFF )

if( SOPHIATX_STATIC_BUILD AND ( ( MSVC AND NOT MINGW ) OR APPLE ) )
   MESSAGE( STATUS "Static build is not available on Windows or OS X" )
   SET( SOPHIATX_STATIC_BUILD OFF )
endif()

MESSAGE( STATUS "SOPHIATX_STATIC_BUILD: ${SOPHIATX_STATIC_BUILD}" )


OPTION(ABAP_INTERFACE "Build without map obejcts on the interface (ON OR OFF)" OFF )
if(ABAP_INTERFACE)
    MESSAGE(STATUS "Building with ABAP compliant Alexandria interface")
    ADD_DEFINITIONS("-DABAP_INTERFACE")
endif(ABAP_INTERFACE)

SET( SOPHIATX_LINT_LEVEL "OFF" CACHE STRING "Lint level during sophiatx build (FULL, HIGH, LOW, OFF)" )
find_program(
   CLANG_TIDY_EXE
   NAMES "clang-tidy"
   DOC "Path to clain-tidy executable"
)

if( NOT CLANG_TIDY_EXE )
   message( STATUS "clang-tidy not found" )
elseif( VERSION LESS 3.6 )
   messgae( STATUS "clang-tidy found but only supported with CMake version >= 3.6" )
else()
   message( STATUS "clany-tidy found: ${CLANG_TIDY_EXE}" )
   if( "${SOPHIATX_LINT_LEVEL}" STREQUAL "FULL" )
      message( STATUS "Linting level set to: FULL" )
      set( DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks='*'" )
   elseif( "${SOPHIATX_LINT_LEVEL}" STREQUAL "HIGH" )
      message( STATUS "Linting level set to: HIGH" )
      set( DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks='boost-use-to-string,clang-analyzer-*,cppcoreguidelines-*,llvm-*,misc-*,performance-*,readability-*'" )
   elseif( "${SOPHIATX_LINT_LEVEL}" STREQUAL "LOW" )
      message( STATUS "Linting level set to: LOW" )
      set( DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks='clang-analyzer-*'" )
   else()
      unset( CLANG_TIDY_EXE )
      message( STATUS "Linting level set to: OFF" )
   endif()
endif( NOT CLANG_TIDY_EXE )

IF( WIN32 )
  SET(BOOST_ROOT $ENV{BOOST_ROOT})
  set(Boost_USE_MULTITHREADED ON)
  set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries
ENDIF(WIN32)


FIND_PACKAGE(Boost 1.65 REQUIRED COMPONENTS ${BOOST_COMPONENTS})

find_package(JNI)
if(JNI_FOUND AND (NOT ABAP_INTERFACE))
    MESSAGE(STATUS "JNI found, building libalexandriaJNI")
else(JNI_FOUND)
    MESSAGE(STATUS "Skipping libalexandriaJNI build, JNI not found")
endif(JNI_FOUND AND (NOT ABAP_INTERFACE))

find_package (SQLite3 3.22.1)
if (SQLite3_FOUND)
    message (STATUS "Found SQLite version: ${SQLite3_VERSION}")
else(SQLite3_FOUND)
    MESSAGE(STATUS "Skipping SQLite build")
endif (SQLite3_FOUND)

if( WIN32 )

   message( STATUS "Configuring SophiaTX on WIN32")
   set( DB_VERSION 60 )
   set( BDB_STATIC_LIBS 1 )

   set( ZLIB_LIBRARIES "" )
   SET( DEFAULT_EXECUTABLE_INSTALL_DIR bin/ )

   set(CRYPTO_LIB)

   if( MSVC )
      # Activate C++ exception handling, assume extern C calls don't throw (EHsc)
      add_compile_options(/wd4503 /wd4267 /wd4244 /EHsc)
      #looks like this flag can have different default on some machines.
      SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
      SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

      # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

      # On windows tcl should be installed to the directory pointed by setenv.bat script
      SET(TCL_INCLUDE_PATH $ENV{TCL_ROOT}/include)
      MESSAGE(STATUS "tcl INCLUDE PATH: ${TCL_INCLUDE_PATH}")

      FIND_PACKAGE(TCL)
      MESSAGE(STATUS "tcl_library: ${TCL_LIBRARY}")

      SET(TCL_LIBS "optimized;${TCL_LIBRARY};debug;")
      get_filename_component(TCL_LIB_PATH "${TCL_LIBRARY}" PATH)
      get_filename_component(TCL_LIB_NAME "${TCL_LIBRARY}" NAME_WE)
      get_filename_component(TCL_LIB_EXT "${TCL_LIBRARY}" EXT)

      SET(TCL_LIBS "${TCL_LIBS}${TCL_LIB_PATH}/${TCL_LIB_NAME}g${TCL_LIB_EXT}")
      SET(TCL_LIBRARY ${TCL_LIBS})
      # Needed to disable MSVC autolinking feature (#pragma comment)Ø
      ADD_DEFINITIONS("-DBOOST_ALL_NO_LIB")
      
   elseif( MINGW )
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive -msse4.2 -Wa,-mbig-obj")
      SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
      # Optimization flag apparently needed to get rid of "File too big" assembler errors when compiling in Debug mode
      # See: https://stackoverflow.com/questions/14125007/gcc-string-table-overflow-error-during-compilation/14601779#29479701
      SET(CMAKE_CXX_FLAGS_DEBUG "-O2")
      if ( SOPHIATX_STATIC_BUILD )
        set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
      endif ( SOPHIATX_STATIC_BUILD )
   endif( MSVC )

else( WIN32 ) # Apple AND Linux
    add_compile_options("-Wall" "-Werror")

    if( APPLE )
        # Apple Specific Options Here
        message( STATUS "Configuring SophiaTX on OS X" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -stdlib=libc++ -Wall -Wno-conversion -Wno-deprecated-declarations" )
    else( APPLE )
        # Linux Specific Options Here
        message( STATUS "Configuring SophiaTX on Linux" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall" )
        set( rt_library rt )
        set( pthread_library pthread)
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        if ( NOT DEFINED crypto_library )
          # I'm not sure why this is here, I guess someone has openssl and can't detect it with find_package()?
          # if you have a normal install, you can define crypto_library to the empty string to avoid a build error
          set( crypto_library crypto)
        endif ()
        if ( SOPHIATX_STATIC_BUILD )
          set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        endif ( SOPHIATX_STATIC_BUILD )
    endif( APPLE )

    if(BUILD_PIC)
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    endif(BUILD_PIC)

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
    endif()

    if( "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
        endif()
    endif()

    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -ftemplate-backtrace-limit=0")

    # based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
    # set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

endif( WIN32 )

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build SophiaTX for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

# fc/src/compress/miniz.c breaks strict aliasing. The Linux kernel builds with no strict aliasing
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -DBOOST_THREAD_DONT_PROVIDE_PROMISE_LAZY" )
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -DBOOST_THREAD_DONT_PROVIDE_PROMISE_LAZY" )

add_subdirectory( libraries )

if (BUILD_PIC)
add_subdirectory( external_plugins )
endif(BUILD_PIC)

add_subdirectory( programs )
add_subdirectory( tests )

if (ENABLE_INSTALLER)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)


include(InstallRequiredSystemLibraries)

set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/packages)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_NAME "sophiatx")
set(CPACK_PACKAGE_VENDOR "Equidato Technologies")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION "A client for the SophiaTX network")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A client for the SophiaTX network")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SophiaTX ${CPACK_PACKAGE_VERSION}")

if(WIN32)
 SET(CPACK_GENERATOR "ZIP;NSIS")
 set(CPACK_PACKAGE_NAME "SophiaTX") # override above
 set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
 set(CPACK_NSIS_PACKAGE_NAME "SophiaTX v${CPACK_PACKAGE_VERSION}")
 set(CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_PACKAGE_NAME}")
 set(CPACK_NSIS_DEFINES "  !define MUI_STARTMENUPAGE_DEFAULTFOLDER \\\"SophiaTX\\\"")
 # it seems like windows zip files usually don't have a single directory inside them, unix tgz frequently do
 SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

endif(WIN32)

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
endif()

if(LINUX)
  # Linux gets a .tgz
  SET(CPACK_GENERATOR "TGZ")
  SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
endif(LINUX)

 include(CPack)
endif(ENABLE_INSTALLER)

if( BUILD_SOPHIATX_TESTNET )
  MESSAGE( STATUS "\n\n             CONFIGURED FOR TEST NETWORK             \n\n" )
else()
  MESSAGE( STATUS "\n\n             CONFIGURED FOR SOPHIATX NETWORK             \n\n" )
endif()

if( LOW_MEMORY_NODE )
  MESSAGE( STATUS "\n\n             CONFIGURED FOR LOW MEMORY NODE          \n\n" )
else()
  MESSAGE( STATUS "\n\n             CONFIGURED FOR FULL NODE                \n\n" )
endif()


