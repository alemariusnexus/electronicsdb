/*
    Copyright 2010-2021 David "Alemarius Nexus" Lerch

    This file is part of electronicsdb.

    electronicsdb is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    electronicsdb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with electronicsdb.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "global.h"

#include <unordered_map>
#include <QColor>
#include <QObject>
#include <QStack>
#include <nxcommon/exception/Exception.h>
#include "command/EditStack.h"
#include "model/container/PartContainer.h"
#include "model/part/Part.h"
#include "model/PartCategory.h"
#include "model/PartLinkType.h"
#include "model/PartPropertyMetaType.h"
#include "gui/MainWindow.h"
#include "util/Task.h"
#include "db/DatabaseConnection.h"

namespace electronicsdb
{


class System : public QObject
{
    Q_OBJECT

public:
    typedef QList<DatabaseConnection*> DatabaseConnectionList;
    typedef DatabaseConnectionList::iterator DatabaseConnectionIterator;
    typedef DatabaseConnectionList::const_iterator ConstDatabaseConnectionIterator;

    typedef QList<PartCategory*> PartCategoryList;
    typedef PartCategoryList::iterator PartCategoryIterator;
    typedef PartCategoryList::const_iterator ConstPartCategoryIterator;

    enum DebugFlags
    {
        DebugFlagLogSQLQueries = 0x01
    };

    enum PaletteColor
    {
        PaletteColorStatusOK,
        PaletteColorStatusError,
        PaletteColorWarning
    };

public:
    static System* getInstance();
    static void destroy();

public:
    ~System();
    EditStack* getEditStack() { return editStack; }

    DatabaseConnection* getCurrentDatabaseConnection() { return currentDbConn; }

    void addPermanentDatabaseConnection(DatabaseConnection* conn);
    void removePermanentDatabaseConnection(DatabaseConnection* conn);
    void replacePermanentDatabaseConnection(DatabaseConnection* oldConn, DatabaseConnection* newConn);
    DatabaseConnection* getPermanentDatabaseConnection(const QString& id);
    DatabaseConnectionList getPermanentDatabaseConnections() { return permDbConns; }
    bool isPermanentDatabaseConnection(DatabaseConnection* conn) const;

    void setStartupDatabaseConnection(DatabaseConnection* conn) { startupConn = conn; }
    DatabaseConnection* getStartupDatabaseConnection() { return startupConn; }

    bool connectDatabase(DatabaseConnection* conn, const QString& pw, bool dialogs = true);
    bool disconnect(bool ask = false);
    void emergencyDisconnect();
    bool hasValidDatabaseConnection() const;
    bool isEmergencyDisconnecting() const { return emergencyDisconnectRequested; }
    QString askConnectDatabasePassword(DatabaseConnection* conn, bool* ok) const;

    void loadAppModel();
    void unloadAppModel();
    bool isAppModelLoaded() const { return appModelLoaded; }

    void displayStatusMessage(const QString& msg, int timeout = 2500);

    QList<PartPropertyMetaType*>& getPartPropertyMetaTypes() { return propMetaTypes; }
    PartPropertyMetaType* getPartPropertyMetaType(const QString& typeID);

    PartCategoryList getPartCategories() const { return partCats; }
    PartCategory* getPartCategory(const QString& id);

    const QList<PartLinkType*>& getPartLinkTypes() const { return partLinkTypes; }
    PartLinkType* getPartLinkType(const QString& id);

    void setChangelogProcessingInhibited(bool inhibit) { clogProcessingInhibited = inhibit; }
    bool isChangelogProcessingInhibited() const { return clogProcessingInhibited; }
    void processChangelogs();
    void truncateChangelogs();

    void unhandledException(Exception& ex);

    DatabaseConnection* loadDatabaseConnectionFromSettings(const QSettings& s,
            const QString& dbNamePlaceholderValue = QString());
    void savePermanentDatabaseConnectionSettings();

    void registerMainWindow(MainWindow* mw) { mainWin = mw; }

    void saveAll();

    void startTask(Task* task);
    void endTask(Task* task);

    bool hasQuit() const { return quitStatus; }

    void jumpToPart(const Part& part);
    void jumpToContainer(const PartContainer& cont);


    QString getRelativeFileAbsolutePath(const QString& relPath, QWidget* parent = nullptr);
    bool openRelativeFile(const QString& relPath, QWidget* parent = nullptr);

    //void signalDropAccepted(Qt::DropAction action, const QString& context, const QUuid& uuid);

    QStringList getInstalledLanguages() const;
    QString getLanguageName(const QString& langCode) const;
    QString getActiveLanguage() const;

    QColor getAppPaletteColor(int type) const;

    void showDatabaseCorruptionWarningDialog(QWidget* parent = nullptr);
    void showFirstRunDialog(QWidget* parent = nullptr);

public slots:
    void quit();
    void forceQuit();

private slots:
    void taskUpdated(int val);

private:
    void initTheme();

    void updateTaskStatus();

signals:
    void databaseConnectionAboutToChange(DatabaseConnection* oldConn, DatabaseConnection* newConn);
    void databaseConnectionChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);

    void appModelAboutToBeReset();
    void appModelReset();

    void permanentDatabaseConnectionSettingsSaved();
    void saveAllRequested();
    //void dropAccepted(Qt::DropAction action, const QString& context, const QUuid& uuid);

private:
    System();

private:
    static System* instance;

private:
    MainWindow* mainWin;
    DatabaseConnectionList permDbConns;
    DatabaseConnection* currentDbConn;
    DatabaseConnection* startupConn;
    EditStack* editStack;
    QStack<Task*> tasks;
    bool quitStatus;
    bool currentlyConnecting;
    bool emergencyDisconnectRequested;

    QList<PartPropertyMetaType*> propMetaTypes;
    PartCategoryList partCats;
    QList<PartLinkType*> partLinkTypes;
    bool appModelLoaded;
    bool clogProcessingInhibited;

    std::unordered_map<int, QColor> appPalette;
};


}
