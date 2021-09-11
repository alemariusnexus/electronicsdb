# Copyright 2010-2021 David "Alemarius Nexus" Lerch
# 
# This file is part of electronicsdb.
#
# electronicsdb is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# electronicsdb is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with electronicsdb.  If not, see <http://www.gnu.org/licenses/>.

CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
PROJECT(electronicsdb C CXX)

SET(EDB_BUILD_TESTS OFF CACHE BOOL "Whether the automatic tests should be built")

SET(EDB_VERSION_MAJOR 0)
SET(EDB_VERSION_MINOR 2)
SET(EDB_VERSION_PATCH 0)

SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

FIND_PACKAGE(Qt6 COMPONENTS Core Gui Widgets Sql LinguistTools QUIET)
IF(NOT Qt6_FOUND)
    FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets Sql LinguistTools REQUIRED)
    SET(QT_VERSION_MAJOR 5)
    MESSAGE(STATUS "Using Qt5. If you want to use Qt6, make sure CMake finds it (e.g. by setting CMAKE_PREFIX_PATH).")
ELSE()
    SET(QT_VERSION_MAJOR 6)
    MESSAGE(STATUS "Using Qt6.")
ENDIF()

IF(NOT "${NXCOMMON_QT_SUPPORT}" STREQUAL "qt${QT_VERSION_MAJOR}")
    UNSET(NXCOMMON_QT_SUPPORT CACHE)
ENDIF()
SET(NXCOMMON_QT_SUPPORT_DEFAULT "qt${QT_VERSION_MAJOR}")

IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nxcommon/src/CMakeLists.txt")
    SET(NXCOMMON_BUILDING ON)
ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nxcommon/src/CMakeLists.txt")

IF(NXCOMMON_BUILDING)
    MESSAGE(STATUS "Building integrated nxcommon.")
    ADD_SUBDIRECTORY(nxcommon/src nxcommon/build)
ENDIF()

IF(NOT TARGET nxcommon)
    FIND_PACKAGE(Nxcommon REQUIRED CONFIG)
ENDIF()

INCLUDE(NxUtilities)


FIND_PACKAGE(ZLIB)

FIND_LIBRARY(XAPIAN_LIB_PATH NAMES xapian libxapian)
FIND_PATH(XAPIAN_INCLUDE_DIR xapian.h)

IF(EDB_BUILD_TESTS)
    FIND_PACKAGE(GTest)
ENDIF()


IF(QT_VERSION_MAJOR LESS 6)
    SET(EDB_BUILD_BUNDLED_QSQLITE_DEFAULT ON)
ELSE()
    SET(EDB_BUILD_BUNDLED_QSQLITE_DEFAULT OFF)
ENDIF()

SET(EDB_BUILD_BUNDLED_QSQLITE "${EDB_BUILD_BUNDLED_QSQLITE_DEFAULT}" CACHE BOOL
        "Whether to build the bundled QSQLITE driver. Useful for Qt5, which ships with an older version of SQLite")


LIST(APPEND LIBRARIES
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Sql
        )

LIST(APPEND LIBRARIES "${XAPIAN_LIB_PATH}")
LIST(APPEND INCDIRS "${XAPIAN_INCLUDE_DIR}")

LIST(APPEND LIBRARIES ZLIB::ZLIB)

IF(EDB_BUILD_TESTS)
    LIST(APPEND LIBRARIES GTest::gtest)
ENDIF()

LIST(APPEND LIBRARIES nxcommon)

LIST(APPEND INCDIRS "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

INCLUDE(GetGitRevisionDescription)

GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)
STRING(SUBSTRING "${GIT_SHA1}" 0 7 GIT_SHA1_SHORT)

ADD_SUBDIRECTORY(electronicsdb electronicsdb)
ADD_SUBDIRECTORY(libhydrogen EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(qsourcehighlite)

IF(EDB_BUILD_BUNDLED_QSQLITE)
    ADD_SUBDIRECTORY(qtplugins/sqlite)
ENDIF()

CONFIGURE_FILE(electronicsdb/edb_config.cmake.h "${CMAKE_CURRENT_BINARY_DIR}/edb_config.h")


SET_SOURCE_FILES_PROPERTIES(electronics_en.ts electronics_de.ts PROPERTIES
    OUTPUT_LOCATION "${CMAKE_SOURCE_DIR}")

QT_WRAP_CPP(MOC_COMPILED_SRCS "${MOC_SRCS}")
QT_WRAP_UI(UIC_COMPILED_SRCS "${UIC_SRCS}")
QT_ADD_TRANSLATION(QM_FILES electronics_en.ts electronics_de.ts)

QT_ADD_RESOURCES(QT_RESOURCE_FILES resources.qrc)

INCLUDE_DIRECTORIES("${INCDIRS}")

ADD_EXECUTABLE(electronicsdb WIN32 ${SRCS} ${MOC_COMPILED_SRCS} ${UIC_COMPILED_SRCS} ${QM_FILES} ${QT_RESOURCE_FILES})
SET_PROPERTY(TARGET electronicsdb PROPERTY CXX_STANDARD 17)
TARGET_LINK_LIBRARIES(electronicsdb "${LIBRARIES}" hydrogen)

IF(EDB_QT_EXTRA_PLUGIN_TARGETS)
    ADD_DEPENDENCIES(electronicsdb ${EDB_QT_EXTRA_PLUGIN_TARGETS})
ENDIF()

# I don't like this, but it's necessary: Because our main source directory has the same name as the generated
# executable, building will otherwise fail on Linux. That's because the source directory leads to a binary directory
# with the same name, which clashes with the executable name. While the binary directory can be changed in
# ADD_SUBDIRECTORY(), this change is NOT possible with QT_WRAP_CPP() and the files it generates. I've tried, but found
# no way for QT_WRAP_CPP() to change its output directory. So this will have to do...
SET_PROPERTY(TARGET electronicsdb PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")


SET(EDB_INSTALL_SKELETON_DIR "" CACHE STRING "Directory containing the basic file structure for a CMake INSTALL.")

INSTALL(TARGETS electronicsdb DESTINATION bin)

#IF(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21.0")
#    INSTALL(IMPORTED_RUNTIME_ARTIFACTS electronicsdb)
#    INSTALL(IMPORTED_RUNTIME_ARTIFACTS
#            Qt${QT_VERSION_MAJOR}::Core
#            Qt${QT_VERSION_MAJOR}::Gui
#            Qt${QT_VERSION_MAJOR}::Widgets
#            Qt${QT_VERSION_MAJOR}::Sql
#            )
#ENDIF()

IF(EDB_INSTALL_SKELETON_DIR)
    INSTALL(DIRECTORY "${EDB_INSTALL_SKELETON_DIR}/" DESTINATION .)
ENDIF()


INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_NAME "ElectronicsDB")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ElectronicsDB - An electronic component management program")
SET(CPACK_PACKAGE_VENDOR "David 'Alemarius Nexus' Lerch")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "${EDB_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${EDB_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${EDB_VERSION_PATCH}")

INCLUDE(CPack)
