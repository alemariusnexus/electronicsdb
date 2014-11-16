# Copyright 2010-2014 David "Alemarius Nexus" Lerch
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



CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(electronicsdb C CXX)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

IF(WIN32 AND NOT MINGW)
	FIND_PACKAGE(Qt4 4.8.0 COMPONENTS QtCore QtGui qtmain REQUIRED)
ELSE(WIN32 AND NOT MINGW)
	FIND_PACKAGE(Qt4 4.8.0 COMPONENTS QtCore QtGui REQUIRED)
ENDIF(WIN32 AND NOT MINGW)

FIND_PACKAGE(Nxcommon REQUIRED CONFIG)

#FIND_LIBRARY(LIBMYSQLCLIENT_LIB_PATH NAMES mysqlclient libmysqlclient mysqlclient18 libmysqlclient18)
#FIND_PATH(LIBMYSQLCLIENT_INCLUDE_DIR mysql/mysql.h)

FIND_LIBRARY(XAPIAN_LIB_PATH NAMES xapian libxapian)
FIND_PATH(XAPIAN_INCLUDE_DIR xapian.h)

FIND_PATH(ICU_INCLUDE_PATH unicode/utypes.h)
FIND_LIBRARY(ICU_BASE_LIB_PATH NAMES icuuc libicuuc)
FIND_LIBRARY(ICU_DATA_LIB_PATH NAMES icudata libicuudata)

INCLUDE(${QT_USE_FILE})
INCLUDE(GetGitRevisionDescription)

GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)

CONFIGURE_FILE(edb_config.cmake.h "${CMAKE_CURRENT_BINARY_DIR}/edb_config.h")


SET(SRCS
    main.cpp MainWindow.cpp ConnectDialog.cpp System.cpp SettingsDialog.cpp DatabaseConnection.cpp
    PartTableModel.cpp FilterWidget.cpp elutil.cpp Filter.cpp PartCategory.cpp PartProperty.cpp
    ListingTable.cpp global.cpp PartDisplayWidget.cpp SQLCommand.cpp
    SQLInsertCommand.cpp SQLDeleteCommand.cpp SQLUpdateCommand.cpp SQLMultiValueInsertCommand.cpp
    SQLNewRecordCommand.cpp PartCategoryWidget.cpp PartEditCommand.cpp EditStack.cpp Task.cpp
    ChoosePartDialog.cpp sqlutils.cpp
    PropertyComboBox.cpp PropertyLineEdit.cpp PlainLineEdit.cpp PropertyMultiValueWidget.cpp
    PropertyLinkEditWidget.cpp PropertyTextEdit.cpp PropertyFileWidget.cpp ContainerWidget.cpp
    ContainerTableModel.cpp ContainerEditCommand.cpp ContainerPartTableModel.cpp
    CompoundEditCommand.cpp PartCategoryProvider.cpp ContainerTableItemDelegate.cpp
    MainApplication.cpp ConnectionEditWidget.cpp
    )
SET(MOC_SRCS
    MainWindow.h ConnectDialog.h System.h SettingsDialog.h DatabaseConnection.h PartTableModel.h
    FilterWidget.h PartCategory.h ListingTable.h PartDisplayWidget.h PartCategoryWidget.h
    EditStack.h Task.h PlainLineEdit.h ChoosePartDialog.h
    PartProperty.h PropertyComboBox.h
    PropertyLineEdit.h PropertyMultiValueWidget.h PropertyLinkEditWidget.h PropertyTextEdit.h
    PropertyFileWidget.h ContainerWidget.h ContainerTableModel.h ContainerPartTableModel.h
    PartCategoryProvider.h ContainerTableItemDelegate.h MainApplication.h ConnectionEditWidget.h
    )
SET(UIC_SRCS
    MainWindow.ui ConnectDialog.ui SettingsDialog.ui ContainerWidget.ui ConnectionEditWidget.ui
    )


SET_SOURCE_FILES_PROPERTIES(electronics_en.ts electronics_de.ts PROPERTIES
    OUTPUT_LOCATION "${CMAKE_SOURCE_DIR}")

QT4_WRAP_CPP(MOC_COMPILED_SRCS ${MOC_SRCS})
QT4_WRAP_UI(UIC_COMPILED_SRCS ${UIC_SRCS})
QT4_ADD_TRANSLATION(QM_FILES electronics_en.ts electronics_de.ts)

QT4_ADD_RESOURCES(QT_RESOURCE_FILES resources.qrc)

FOREACH(VAR ${LIBNXCOMMON_INCLUDE_DIRS})
    MESSAGE("${VAR}")
ENDFOREACH(VAR)

INCLUDE_DIRECTORIES(${LIBMYSQLCLIENT_INCLUDE_DIR} ${XAPIAN_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
        "${ICU_INCLUDE_PATH}")

ADD_COMPILE_OPTIONS(--std=c++11)
ADD_EXECUTABLE(electronicsdb ${SRCS} ${MOC_COMPILED_SRCS} ${UIC_COMPILED_SRCS} ${QM_FILES} ${QT_RESOURCE_FILES})
TARGET_LINK_LIBRARIES(electronicsdb ${QT_LIBRARIES} ${LIBMYSQLCLIENT_LIB_PATH} ${XAPIAN_LIB_PATH} nxcommon
        "${ICU_BASE_LIB_PATH}" "${ICU_DATA_LIB_PATH}")
