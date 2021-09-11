ADD_SUBDIRECTORY(test)

ADD_SOURCES (
        printops.cpp
        TestEventSniffer.cpp
        TestManager.cpp
        testutil.cpp
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        TestEventSniffer.h
        TestManager.h
        )
