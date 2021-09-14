ADD_SUBDIRECTORY(container)
ADD_SUBDIRECTORY(dragndrop)
ADD_SUBDIRECTORY(part)
ADD_SUBDIRECTORY(settings)
ADD_SUBDIRECTORY(smodel)
ADD_SUBDIRECTORY(util)

ADD_SOURCES (
        GUIStatePersister.cpp
        MainWindow.cpp
        )
ADD_SOURCES(OUTVAR UIC_SRCS
        MainWindow.ui
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        GUIStatePersister.h
        MainWindow.h
        )
