ADD_SUBDIRECTORY(sql)

ADD_SOURCES (
        CompoundEditCommand.cpp
        DummyEditCommand.cpp
        EditCommand.cpp
        EditStack.cpp
        SQLEditCommand.cpp
)
ADD_SOURCES(OUTVAR UIC_SRCS
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        EditStack.h
        )
