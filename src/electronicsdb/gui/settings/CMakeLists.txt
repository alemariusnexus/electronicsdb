ADD_SOURCES (
        BackupDialog.cpp
        ConnectDialog.cpp
        ConnectionEditWidget.cpp
        DatabaseMigrationDialog.cpp
        KeyVaultDialog.cpp
        KeyVaultWidget.cpp
        SettingsDialog.cpp
        )
ADD_SOURCES(OUTVAR UIC_SRCS
        BackupDialog.ui
        ConnectDialog.ui
        ConnectionEditWidget.ui
        DatabaseMigrationDialog.ui
        KeyVaultDialog.ui
        KeyVaultWidget.ui
        SettingsDialog.ui
        )
ADD_SOURCES(OUTVAR MOC_SRCS
        BackupDialog.h
        ConnectDialog.h
        ConnectionEditWidget.h
        DatabaseMigrationDialog.h
        KeyVaultDialog.h
        KeyVaultWidget.h
        SettingsDialog.h
        )
