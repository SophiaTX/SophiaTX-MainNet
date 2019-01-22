# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindSQLite3
-----------

Find the SQLite libraries, v3

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``SQLite::SQLite3``

Variables used by this module, they can change the default behaviour and need
to be set before calling find_package:

SQLITE3_ROOT_DIR         Set this variable to the root installation of
                         SQLite3 if the module has problems finding the
                         proper installation path.

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SQLite3_INCLUDE_DIRS``
  where to find sqlite3.h, etc.
``SQLite3_LIBRARIES``
  the libraries to link against to use SQLite3.
``SQLite3_VERSION``
  version of the SQLite3 library found
``SQLite3_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
FIND_PATH(SQLITE3_ROOT_DIR
        NAMES include/sqlite3.h
        PATHS /opt/local/ /usr/local/ /usr/
        NO_DEFAULT_PATH
        )

# Look for the header file.
FIND_PATH(SQLite3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS ${SQLITE3_ROOT_DIR}/include
        NO_DEFAULT_PATH
        )

# Look for the library.
FIND_LIBRARY(SQLite3_LIBRARY
        NAMES sqlite3 sqlite
        PATHS ${SQLITE3_ROOT_DIR}/lib
        NO_DEFAULT_PATH)

# Extract version information from the header file
if(SQLite3_INCLUDE_DIR)
    file(STRINGS ${SQLite3_INCLUDE_DIR}/sqlite3.h _ver_line
            REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
            LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
            SQLite3_VERSION "${_ver_line}")
    unset(_ver_line)
endif()

find_package_handle_standard_args(SQLite3
        REQUIRED_VARS SQLite3_INCLUDE_DIR SQLite3_LIBRARY
        VERSION_VAR SQLite3_VERSION)

# Create the imported target
if(SQLite3_FOUND)
    set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
    set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})
    if(NOT TARGET SQLite::SQLite3)
        add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
        set_target_properties(SQLite::SQLite3 PROPERTIES
                IMPORTED_LOCATION             "${SQLite3_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}")
    endif()
endif()