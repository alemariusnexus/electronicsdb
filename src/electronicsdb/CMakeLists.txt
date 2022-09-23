ADD_SUBDIRECTORY(command)
ADD_SUBDIRECTORY(db)
ADD_SUBDIRECTORY(exception)
ADD_SUBDIRECTORY(gui)
ADD_SUBDIRECTORY(model)
ADD_SUBDIRECTORY(script)
ADD_SUBDIRECTORY(util)

IF(EDB_BUILD_TESTS)
    ADD_SUBDIRECTORY(test)
ENDIF()

ADD_SOURCES (
        global.cpp
        main.cpp
        MainApplication.cpp
        System.cpp
)
ADD_SOURCES(OUTVAR UIC_SRCS
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        MainApplication.h
        System.h
        )
