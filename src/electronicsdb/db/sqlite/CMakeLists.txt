ADD_SOURCES (
        SQLiteDatabaseConnection.cpp
        SQLiteDatabaseConnectionWidget.cpp
        SQLiteDatabaseWrapper.cpp
        )
ADD_SOURCES(OUTVAR UIC_SRCS
        SQLiteDatabaseConnectionWidget.ui
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        SQLiteDatabaseConnection.h
        SQLiteDatabaseConnectionWidget.h
        SQLiteDatabaseWrapper.h
        )
