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

#include "System.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QMultiMap>
#include <QMessageBox>
#include <QPalette>
#include <QRegularExpression>
#include <QSettings>
#include <QSqlDatabase>
#include <QTranslator>
#include <nxcommon/exception/InvalidValueException.h>
#include <nxcommon/log.h>
#include "db/SQLDatabaseWrapperFactory.h"
#include "exception/NoDatabaseConnectionException.h"
#include "exception/SQLException.h"
#include "model/container/PartContainerFactory.h"
#include "model/part/PartFactory.h"
#include "model/StaticModelManager.h"
#include "util/KeyVault.h"

namespace electronicsdb
{


System* System::instance = nullptr;


System::System()
        : mainWin(nullptr), currentDbConn(nullptr), startupConn(nullptr), editStack(nullptr), quitStatus(false),
          currentlyConnecting(false), emergencyDisconnectRequested(false), appModelLoaded(false),
          clogProcessingInhibited(false)
{
}

System::~System()
{
    while (!tasks.empty()) {
        endTask(tasks.top());
    }

    disconnect(false);

    for (DatabaseConnection* conn : permDbConns) {
        if (conn == currentDbConn)
            currentDbConn = nullptr;
        delete conn;
    }

    delete currentDbConn;

    for (PartCategory* pcat : partCats) {
        delete pcat;
    }

    delete mainWin;

    delete editStack;

    StaticModelManager::destroy();
}

System* System::getInstance()
{
    if (!instance) {
        instance = new System;
        instance->editStack = new EditStack;

        QStringList sqlDriverNames = QSqlDatabase::drivers();
        LogDebug("Supported SQL drivers (Qt): %s", sqlDriverNames.join(", ").toUtf8().constData());

        instance->initTheme();
    }

    return instance;
}

void System::destroy()
{
    if (instance) {
        System* inst = instance;
        instance = nullptr;
        delete inst;
    }
}

void System::initTheme()
{
    QSettings s;
    QString theme = s.value("gui/theme", "default").toString();

    if (theme == "dark") {
        appPalette[PaletteColorStatusOK] = QColor(0, 170, 0);
        appPalette[PaletteColorStatusError] = QColor(255, 0, 0);
        appPalette[PaletteColorWarning] = QColor(255, 0, 0);
    } else {
        appPalette[PaletteColorStatusOK] = QColor(0, 170, 0);
        appPalette[PaletteColorStatusError] = QColor(255, 0, 0);
        appPalette[PaletteColorWarning] = QColor(255, 0, 0);
    }

    if (theme == "dark") {
        QPalette darkPalette;
        QColor darkColor = QColor(45,45,45);
        QColor disabledColor = QColor(127,127,127);
        darkPalette.setColor(QPalette::Window, darkColor);
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(18,18,18));
        darkPalette.setColor(QPalette::AlternateBase, darkColor);
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
        darkPalette.setColor(QPalette::Button, darkColor);
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

        qApp->setPalette(darkPalette);
    }
}

PartPropertyMetaType* System::getPartPropertyMetaType(const QString& typeID)
{
    for (PartPropertyMetaType* mtype : propMetaTypes) {
        if (mtype->metaTypeID == typeID) {
            return mtype;
        }
    }
    return nullptr;
}

PartCategory* System::getPartCategory(const QString& id)
{
    for (PartCategory* cat : partCats) {
        if (cat->getID() == id) {
            return cat;
        }
    }
    return nullptr;
}

PartLinkType* System::getPartLinkType(const QString& id)
{
    for (PartLinkType* type : partLinkTypes) {
        if (type->getID() == id) {
            return type;
        }
    }
    return nullptr;
}

void System::processChangelogs()
{
    PartFactory::getInstance().processChangelog();
    PartContainerFactory::getInstance().processChangelog();
}

void System::truncateChangelogs()
{
    PartFactory::getInstance().truncateChangelog();
    PartContainerFactory::getInstance().truncateChangelog();
}

void System::unhandledException(Exception& ex)
{
    QString errMsg;

    if (!ex.getBacktrace().isNull()) {
        errMsg = QString("ERROR: Unhandled exception caught:\n\n%1\n\n%2")
                .arg(ex.what(), ex.getBacktrace());
    } else {
        errMsg = QString("ERROR: Unhandled exception caught:\n\n%1")
                .arg(ex.what());
    }

    fprintf(stderr, "%s\n", errMsg.toUtf8().constData());

    QMessageBox::critical(mainWin, "Unhandled Exception", errMsg);
}

void System::addPermanentDatabaseConnection(DatabaseConnection* conn)
{
    permDbConns.push_back(conn);
}

void System::removePermanentDatabaseConnection(DatabaseConnection* conn)
{
    permDbConns.removeOne(conn);
    if (startupConn == conn) {
        startupConn = nullptr;
    }
    delete conn;
}

DatabaseConnection* System::getPermanentDatabaseConnection(const QString& id)
{
    for (DatabaseConnection* conn : permDbConns) {
        if (conn->getID() == id) {
            return conn;
        }
    }
    return nullptr;
}

void System::replacePermanentDatabaseConnection(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
    int idx = permDbConns.indexOf(oldConn);
    if (idx < 0) {
        throw InvalidValueException("Database connection not found", __FILE__, __LINE__);
    }
    permDbConns.replace(idx, newConn);
    delete oldConn;
}

bool System::connectDatabase(DatabaseConnection* conn, const QString& pw, bool dialogs)
{
    if (currentDbConn) {
        if (!disconnect(dialogs)) {
            return false;
        }
    }

    assert(!currentDbConn);

    DatabaseConnection* connCpy = conn->clone();

    emit databaseConnectionAboutToChange(nullptr, connCpy);

    try {
        LogInfo("Opening database %s...", connCpy->getDescription().toUtf8().constData());
        connCpy->openDatabase(pw);
    } catch (SQLException& ex) {
        if (dialogs) {
            QMessageBox::critical(mainWin, tr("Failed To Open Connection"), ex.getMessage());
            return false;
        } else {
            throw ex;
        }
    }

    currentDbConn = connCpy;

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    dbw->initSession();

    LogDebug("Creating database schema...");
    dbw->createSchema();

    QFileInfo fileRootInfo(connCpy->getFileRoot());

    if (!fileRootInfo.exists()) {
        QMessageBox::warning(mainWin, tr("File Root Not Found"),
                tr(	"The file root directory '%1' could not be found! The program will continue to work, but "
                    "opening or writing of the embedded files will be disabled.")
                    .arg(connCpy->getFileRoot()));
    } else if (!fileRootInfo.isDir()) {
        QMessageBox::warning(mainWin, tr("Invalid File Root"),
                tr(	"The file root directory '%1' exists but is not a directory! The program will continue to "
                    "work, but opening or writing of the embedded files will be disabled.")
                    .arg(connCpy->getFileRoot()));
    }

    currentlyConnecting = true;
    emit databaseConnectionChanged(nullptr, connCpy);
    currentlyConnecting = false;

    if (emergencyDisconnectRequested) {
        emergencyDisconnectRequested = false;

        DatabaseConnection* oldConn = currentDbConn;

        emit databaseConnectionAboutToChange(oldConn, nullptr);

        dbw->setDatabase(QSqlDatabase());
        db = QSqlDatabase();

        currentDbConn = nullptr;

        QString connName = QSqlDatabase::database().connectionName();
        conn->closeDatabase(connName);

        emit databaseConnectionChanged(oldConn, nullptr);

        delete oldConn;

        return false;
    }

    displayStatusMessage(tr("Connection Established!"));
    LogDebug("Connection successful.");

    loadAppModel();

    return true;
}

bool System::disconnect(bool ask)
{
    if (!currentDbConn) {
        return true;
    }

    if (ask) {
        QMessageBox::StandardButtons res = QMessageBox::warning(mainWin, tr("Close Connection?"),
                tr("There is already an open connection. If you continue, the current connection will be closed.\n\n"
                "Are you sure you want to continue?"),
                QMessageBox::Yes | QMessageBox::No);

        if (res != QMessageBox::Yes) {
            return false;
        }
    }

    unloadAppModel();

    emit databaseConnectionAboutToChange(currentDbConn, nullptr);

    LogDebug("Closing database connection...");

    QString connName = QSqlDatabase::database().connectionName();
    currentDbConn->closeDatabase(connName);

    DatabaseConnection* oldConn = currentDbConn;

    currentDbConn = nullptr;

    emit databaseConnectionChanged(oldConn, nullptr);

    delete oldConn;

    LogDebug("Database connection closed.");
    displayStatusMessage(tr("Connection Closed!"));

    return true;
}

void System::emergencyDisconnect()
{
    if (currentlyConnecting) {
        emergencyDisconnectRequested = true;
    } else {
        disconnect(false);
    }
}

bool System::hasValidDatabaseConnection() const
{
    return QSqlDatabase::database().isValid()  &&  !emergencyDisconnectRequested;
}

QString System::askConnectDatabasePassword(DatabaseConnection* conn, bool* ok) const
{
    if (!conn->needsPassword()) {
        *ok = true;
        return QString();
    }

    KeyVault& vault = KeyVault::getInstance();

    if (conn->isKeepPasswordInVault()  &&  vault.containsKey(conn->getPasswordVaultID())) {
        QString pw = vault.getKeyMaybeWithUserPrompt(conn->getPasswordVaultID(), mainWin);
        if (!pw.isNull()) {
            *ok = true;
            return pw;
        }
    }

    QString pw = QInputDialog::getText(mainWin, tr("Enter Password"),
                                       QString(tr("Enter password for connection '%1':")).arg(conn->getName()),
                                       QLineEdit::Password, QString(), ok);
    return pw;
}

void System::loadAppModel()
{
    unloadAppModel();

    assert(!appModelLoaded);
    assert(propMetaTypes.empty());
    assert(partCats.empty());
    assert(partLinkTypes.empty());

    emit appModelAboutToBeReset();

    StaticModelManager* modelBuilder = StaticModelManager::getInstance();

    propMetaTypes = modelBuilder->buildPartPropertyMetaTypes();
    partCats = modelBuilder->buildCategories();
    partLinkTypes = modelBuilder->buildPartLinkTypes();

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    auto trans = dbw->beginDDLTransaction();

    dbw->createAppModelDependentSchema();

    trans.commit();

    appModelLoaded = true;

    emit appModelReset();
}

void System::unloadAppModel()
{
    if (!appModelLoaded) {
        return;
    }

    emit appModelAboutToBeReset();

    for (PartLinkType* type : partLinkTypes) {
        delete type;
    }
    partLinkTypes.clear();

    for (PartCategory* cat : partCats) {
        delete cat;
    }
    partCats.clear();

    for (PartPropertyMetaType* mtype : propMetaTypes) {
        delete mtype;
    }
    propMetaTypes.clear();

    appModelLoaded = false;

    emit appModelReset();
}

bool System::isPermanentDatabaseConnection(DatabaseConnection* conn) const
{
    return permDbConns.contains(conn);
}

DatabaseConnection* System::loadDatabaseConnectionFromSettings (
        const QSettings& s, const QString& dbNamePlaceholderValue
) {
    QString driverName = s.value("driver").toString();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(driverName));

    SQLDatabaseWrapper::DatabaseType dbType;
    for (auto dbTypeCand : dbw->getSupportedDatabaseTypes()) {
        if (dbTypeCand.driverName == driverName) {
            dbType = dbTypeCand;
            break;
        }
    }

    if (dbType.driverName.isNull()) {
        throw InvalidValueException("Invalid database driver name", __FILE__, __LINE__);
    }

    DatabaseConnection* conn = dbw->createConnection(dbType);
    assert(conn);
    conn->loadFromSettings(s, dbNamePlaceholderValue);

    return conn;
}

void System::savePermanentDatabaseConnectionSettings()
{
    QSettings s;

    s.setValue("main/startup_db", startupConn ? startupConn->getID() : QString(""));

    int numConns = permDbConns.size();

    int i;

    for (i = numConns ; s.childGroups().contains((QString("db%1").arg(i))) ; i++) {
        s.remove(QString("db%1").arg(i));
    }

    i = 0;
    for (DatabaseConnectionIterator it = permDbConns.begin() ; it != permDbConns.end() ; it++, i++) {
        DatabaseConnection* conn = *it;

        s.beginGroup(QString("db%1").arg(i));
        conn->saveToSettings(s);
        s.endGroup();
    }

    s.sync();

    emit permanentDatabaseConnectionSettingsSaved();
}

void System::quit()
{
    quitStatus = true;
    qApp->quit();
}

void System::forceQuit()
{
    quit();
    exit(1);
}

void System::displayStatusMessage(const QString& msg, int timeout)
{
    if (!mainWin)
        return;

    mainWin->statusBar()->showMessage(msg, timeout);
}

void System::saveAll()
{
    emit saveAllRequested();
}

void System::startTask(Task* task)
{
    tasks.push(task);
    connect(task, &Task::updated, this, &System::taskUpdated);

    updateTaskStatus();

    qApp->processEvents();
}

void System::endTask(Task* task)
{
    for (QStack<Task*>::iterator it = tasks.begin() ; it != tasks.end() ; it++) {
        if (*it == task) {
            tasks.erase(it);
            break;
        }
    }

    updateTaskStatus();

    qApp->processEvents();
}

void System::taskUpdated(int val)
{
    Task* task = (Task*) sender();

    if (task != tasks.top())
        return;

    mainWin->updateProgressBar(val);

    qApp->processEvents();
}

void System::updateTaskStatus()
{
    if (tasks.empty()) {
        mainWin->setProgressBarVisible(false);
    } else {
        Task* task = tasks.top();

        mainWin->setProgressBarRange(task->getMinimum(), task->getMaximum());
        mainWin->setProgressBarDescription(task->getDescription());
        mainWin->setProgressBarVisible(true);
        mainWin->updateProgressBar(task->getValue());
    }
}

void System::jumpToPart(const Part& part)
{
    if (!mainWin  ||  !part  ||  !part.hasID())
        return;
    mainWin->jumpToPart(part);
}

void System::jumpToContainer(const PartContainer& cont)
{
    if (!mainWin  ||  !cont)
        return;
    mainWin->jumpToContainer(cont);
}

QString System::getRelativeFileAbsolutePath(const QString& relPath, QWidget* parent)
{
    DatabaseConnection* conn = getCurrentDatabaseConnection();

    QString fpath = relPath;
    QFileInfo fpathInfo(fpath);

    if (!fpathInfo.isAbsolute()) {
        if (conn) {
            fpathInfo = QFileInfo(QDir(conn->getFileRoot()), fpath);
            fpath = fpathInfo.absoluteFilePath();
        } else {
            QMessageBox::critical(parent, tr("File Not Found"),
                                  tr("The path '%1' is not absolute and there is no current database connection to "
                                     "read the file root path from.").arg(fpath));
            return QString();
        }
    }

    return fpath;
}

bool System::openRelativeFile(const QString& relPath, QWidget* parent)
{
    QString fpath = getRelativeFileAbsolutePath(relPath, parent);
    QFileInfo fpathInfo(fpath);

    if (!fpathInfo.exists()  ||  !fpathInfo.isFile()) {
        QMessageBox::critical(parent, tr("File Not Found"),
                              tr("The file '%1' does not exist!").arg(fpath));
        return false;
    }

    QDesktopServices::openUrl(QUrl(QString("file:///%1").arg(fpath)));

    return true;
}

/*void System::signalDropAccepted(Qt::DropAction action, const QString& context, const QUuid& uuid)
{
    emit dropAccepted(action, context, uuid);
}*/

QStringList System::getInstalledLanguages() const
{
    QStringList res;

    QStringList qmFiles = QDir(":/").entryList(QStringList() << "electronics_*.qm", QDir::Files, QDir::NoSort);

    for (QString qmFile : qmFiles) {
        QRegularExpression langRegex("\\Aelectronics_(.*).qm\\z");
        auto match = langRegex.match(qmFile);
        QString langCode = match.captured(1);
        res << langCode;
    }

    return res;
}

QString System::getLanguageName(const QString& langCode) const
{
    QTranslator ttrans;
    (void) ttrans.load(QString(":/electronics_%1.qm").arg(langCode));
    return ttrans.translate("Global", "ThisLanguage");
}

QString System::getActiveLanguage() const
{
    QSettings s;

    QString lang = s.value("main/lang", QLocale::system().name()).toString();

    QStringList langs = getInstalledLanguages();
    if (!langs.contains(lang, Qt::CaseInsensitive)) {
        lang = "en";
    }

    return lang;
}

QColor System::getAppPaletteColor(int type) const
{
    return appPalette.find(type)->second;
}

void System::showDatabaseCorruptionWarningDialog(QWidget* parent)
{
    QSettings s;

    bool showDDLWarning = false;

    if (hasValidDatabaseConnection()) {
        QSqlDatabase db = QSqlDatabase::database();
        std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

        if (!dbw->supportsTransactionalDDL()) {
            showDDLWarning = true;
        }
    }

    if (    !s.value("gui/db_corruption_warning_shown").toBool()
        ||  (showDDLWarning  &&  !s.value("gui/db_corruption_warning_ddl_shown").toBool())
    ) {
        QString extraText;

        s.setValue("gui/db_corruption_warning_shown", true);
        if (showDDLWarning) {
            s.setValue("gui/db_corruption_warning_ddl_shown", true);
        }

        s.sync();

        if (showDDLWarning) {
            extraText = tr("<br/><br/>You are using a database which doesn't support transactional DDL. That means "
                           "that if any single step of this fails, it <b><u>will definitely lead to data "
                           "corruption</u></b>, as the previous steps can't be rolled back. Consider using a database "
                           "with support for transactional DDL (e.g. PostgreSQL or SQLite), or be absolutely certain "
                           "that you have a valid, up-to-date database backup.");
        }

        QMessageBox::warning(parent, tr("Dangerous Operation"),
                             tr("<b>WARNING:</b> Changing the static model has a risk of causing <b>database corruption</b>. "
                                "This is beta functionality and very likely has errors. You should always "
                                "<b>backup your database</b> before changing the static model. Not doing so may lead "
                                "to <b>data loss</b>.%1").arg(extraText));
    }
}

void System::showFirstRunDialog(QWidget* parent)
{
    QSettings s;

    if (!s.value("gui/first_run_dialog_shown").toBool()) {
        s.setValue("gui/first_run_dialog_shown", true);
        s.sync();

        QMessageBox::information(parent, tr("First Steps"), tr("FirstRunDialogText"));
    }
}


}
