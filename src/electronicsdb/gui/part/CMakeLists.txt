ADD_SOURCES (
        ChoosePartDialog.cpp
        FilterWidget.cpp
        ListingTable.cpp
        PartCategoryWidget.cpp
        PartDisplayWidget.cpp
        PartLinkEditWidget.cpp
        PartTableModel.cpp
        PropertyComboBox.cpp
        PropertyDateTimeEdit.cpp
        PropertyFileWidget.cpp
        PropertyLineEdit.cpp
        PropertyMultiValueWidget.cpp
        PropertyTextEdit.cpp
        )
ADD_SOURCES(OUTVAR UIC_SRCS
        PartLinkEditWidget.ui
        PropertyDateTimeEdit.ui
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        ChoosePartDialog.h
        FilterWidget.h
        ListingTable.h
        PartCategoryWidget.h
        PartDisplayWidget.h
        PartLinkEditWidget.h
        PartTableModel.h
        PropertyComboBox.h
        PropertyDateTimeEdit.h
        PropertyFileWidget.h
        PropertyLineEdit.h
        PropertyMultiValueWidget.h
        PropertyTextEdit.h
        )
