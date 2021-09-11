IF(QT_VERSION_MAJOR LESS 6)
    SET(EDB_BUILD_QSQLITE_DEFAULT ON)
ELSE()
    SET(EDB_BUILD_QSQLITE_DEFAULT OFF)
ENDIF()

SET(EDB_BUILD_QSQLITE "${EDB_BUILD_QSQLITE_DEFAULT}" CACHE BOOL
        "Whether to build the QSQLITE driver. Useful for Qt5, which ships with an older version of SQLite.")
SET(EDB_BUILD_QMYSQL ON CACHE BOOL
        "Whether to build the QMYSQL driver. Useful as long as Qt doesn't ship binaries of that driver.")

IF(EDB_BUILD_QSQLITE OR EDB_BUILD_QMYSQL)
    MESSAGE(STATUS "To compile Qt plugins, you need to download the Qt source code (use the correct version!)")
    FIND_PATH(EDB_QT_PLUGINS_SOURCE_PATH sqldrivers/sqlite/qsql_sqlite.cpp)
    IF(NOT EDB_QT_PLUGINS_SOURCE_PATH)
        MESSAGE(FATAL_ERROR "Qt plugin source code not found. Make sure to set EDB_QT_PLUGINS_SOURCE_PATH correctly.")
    ENDIF()
ENDIF()

IF(EDB_BUILD_QSQLITE)
    ADD_SUBDIRECTORY(sqlite)
ENDIF()
IF(EDB_BUILD_QMYSQL)
    ADD_SUBDIRECTORY(mysql)
ENDIF()

PROPAGATE_PARENT_SCOPE(EDB_QT_EXTRA_PLUGIN_TARGETS)
