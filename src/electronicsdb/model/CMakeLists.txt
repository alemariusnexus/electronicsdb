ADD_SUBDIRECTORY(abstract)
ADD_SUBDIRECTORY(assoc)
ADD_SUBDIRECTORY(container)
ADD_SUBDIRECTORY(part)

ADD_SOURCES (
        AbstractPartProperty.cpp
        DatabaseMigrator.cpp
        PartCategory.cpp
        PartLinkType.cpp
        PartProperty.cpp
        PartPropertyMetaType.cpp
        StaticModelManager.cpp
        )
ADD_SOURCES(OUTVAR UIC_SRCS
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        AbstractPartProperty.h
        DatabaseMigrator.h
        PartCategory.h
        PartLinkType.h
        PartProperty.h
        StaticModelManager.h
        )
