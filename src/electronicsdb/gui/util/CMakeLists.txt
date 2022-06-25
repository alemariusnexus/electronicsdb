ADD_SOURCES (
        ChangelogEntry.cpp
        ChangelogTableModel.cpp
        ChangelogWidget.cpp
        PlainLineEdit.cpp
        RichTextItemDelegate.cpp
        SimpleURLDownloader.cpp
        )
ADD_SOURCES(OUTVAR UIC_SRCS
        ChangelogWidget.ui
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        ChangelogEntry.h
        ChangelogTableModel.h
        ChangelogWidget.h
        PlainLineEdit.h
        RichTextItemDelegate.h
        SimpleURLDownloader.h
        )
