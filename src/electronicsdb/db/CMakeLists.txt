ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(mysql)
ADD_SUBDIRECTORY(psql)
ADD_SUBDIRECTORY(sqlite)

ADD_SOURCES (
        DatabaseConnection.cpp
        DatabaseConnectionWidget.cpp
        SQLDatabaseWrapper.cpp
        SQLDatabaseWrapperFactory.cpp
        SQLTransaction.cpp
        )
ADD_SOURCES(OUTVAR UIC_SRCS
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        DatabaseConnection.h
        DatabaseConnectionWidget.h
        SQLDatabaseWrapper.h
        )
