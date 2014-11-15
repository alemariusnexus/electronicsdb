/*
	Copyright 2010-2014 David "Alemarius Nexus" Lerch

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
#include "PartCategoryProvider.h"
#include "NoDatabaseConnectionException.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <QtGui/QMessageBox>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTranslator>
#include <nxcommon/sql/sql.h>


System* System::instance = NULL;


System::System()
		: mainWin(NULL), currentDbConn(NULL), startupConn(NULL), editStack(NULL), quitStatus(false),
		  currentlyConnecting(false), emergencyDisconnectRequested(false)
{
}


System::~System()
{
	while (!tasks.empty()) {
		endTask(tasks.top());
	}

	disconnect(false);

	for (DatabaseConnectionIterator it = permDbConns.begin() ; it != permDbConns.end() ; it++) {
		DatabaseConnection* conn = *it;

		if (conn == currentDbConn)
			currentDbConn = NULL;

		delete conn;
	}

	delete currentDbConn;

	for (PartCategoryIterator it = partCats.begin() ; it != partCats.end() ; it++) {
		delete *it;
	}

	delete mainWin;

	delete editStack;

	PartCategoryProvider::destroy();
}


System* System::getInstance()
{
	if (!instance) {
		instance = new System;
		instance->editStack = new EditStack;
	}

	return instance;
}


void System::destroy()
{
	if (instance) {
		System* inst = instance;
		instance = NULL;
		delete inst;
	}
}


void System::unhandeledException(Exception& ex)
{
	QString errMsg;

	if (!ex.getBacktrace().isNull()) {
		errMsg = QString("ERROR: Unhandeled exception caught:\n\n%1\n\n%2")
				.arg(ex.what()).arg(ex.getBacktrace());
	} else {
		errMsg = QString("ERROR: Unhandeled exception caught:\n\n%1")
				.arg(ex.what());
	}

	fprintf(stderr, "%s\n", errMsg.toAscii().constData());

	QMessageBox::critical(mainWin, "Unhandeled Exception", errMsg);
}


void System::addPartCategory(PartCategory* cat)
{
	partCats << cat;
}


void System::addPermanentDatabaseConnection(DatabaseConnection* conn)
{
	permDbConns.push_back(conn);
	emit permanentDatabaseConnectionAdded(conn);
}


void System::removePermanentDatabaseConnection(DatabaseConnection* conn)
{
	permDbConns.removeOne(conn);

	if (startupConn == conn)
		startupConn = NULL;

	emit permanentDatabaseConnectionRemoved(conn);
}


bool System::connectDatabase(DatabaseConnection* conn, const QString& pw, bool dialogs, bool createTables)
{
	if (currentDbConn) {
		if (!disconnect(dialogs)) {
			return false;
		}
	}

	try {
		if (conn->getType() == DatabaseConnection::MySQL) {
			currentSqlDb = MySQLDriver::getInstance()->openDatabase (
					conn->getMySQLHost().toUtf8().constData(),
					conn->getMySQLUser().toUtf8().constData(),
					pw.toUtf8().constData(),
					conn->getMySQLDatabaseName().toUtf8().constData(),
					conn->getMySQLPort()
					);
		} else {
			currentSqlDb = SQLiteDriver::getInstance()->openDatabase (
					File(conn->getSQLiteFilePath().toUtf8().constData()), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		}
	} catch (SQLException& ex) {
		if (dialogs) {
			QMessageBox::critical(mainWin, tr("Failed To Open Connection"), ex.getMessage());
			return false;
		} else {
			throw ex;
		}
	}

	DatabaseConnection* oldConn = currentDbConn;
	currentDbConn = conn;

	if (createTables) {
		this->createTables();
	}

	QFileInfo fileRootInfo(conn->getFileRoot());

	if (!fileRootInfo.exists()) {
		QMessageBox::warning(mainWin, tr("File Root Not Found"),
				tr(	"The file root directory '%1' could not be found! The program will continue to work, but "
					"opening or writing of the embedded files will be disabled.")
					.arg(conn->getFileRoot()));
	} else if (!fileRootInfo.isDir()) {
		QMessageBox::warning(mainWin, tr("Invalid File Root"),
				tr(	"The file root directory '%1' exists but is not a directory! The program will continue to "
					"work, but opening or writing of the embedded files will be disabled.")
					.arg(conn->getFileRoot()));
	}

	currentlyConnecting = true;
	emit databaseConnectionStatusChanged(oldConn, conn);
	currentlyConnecting = false;

	if (emergencyDisconnectRequested) {
		emergencyDisconnectRequested = false;

		DatabaseConnection* oldConn = currentDbConn;

		currentDbConn = NULL;
		currentSqlDb.close();
		currentSqlDb = SQLDatabase();

		emit databaseConnectionStatusChanged(oldConn, NULL);

		return false;
	}

	displayStatusMessage(tr("Connection Established!"));

	return true;
}


bool System::disconnect(bool ask)
{
	if (!currentDbConn)
		return true;

	if (ask) {
		QMessageBox::StandardButtons res = QMessageBox::warning(mainWin, tr("Close Connection?"),
				tr("There is already an open connection. If you continue, the current connection will be closed.\n\n"
				"Are you sure you want to continue?"),
				QMessageBox::Yes | QMessageBox::No);

		if (res != QMessageBox::Yes) {
			return false;
		}
	}

	currentSqlDb.close();
	currentSqlDb = SQLDatabase();

	DatabaseConnection* oldConn = currentDbConn;

	currentDbConn = NULL;

	emit databaseConnectionStatusChanged(oldConn, NULL);

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
	return currentSqlDb.isValid()  &&  !emergencyDisconnectRequested;
}


bool System::isPermanentDatabaseConnection(DatabaseConnection* conn) const
{
	return permDbConns.contains(conn);
}


void System::savePermanentDatabaseConnectionSettings()
{
	QSettings s;

	if (startupConn) {
		s.setValue("main/startup_db", permDbConns.indexOf(startupConn));
	} else {
		s.setValue("main/startup_db", -1);
	}

	unsigned int numConns = permDbConns.size();

	unsigned int i;

	for (i = numConns ; s.childGroups().contains((QString("db%1").arg(i))) ; i++) {
		s.remove(QString("db%1").arg(i));
	}

	i = 0;
	for (DatabaseConnectionIterator it = permDbConns.begin() ; it != permDbConns.end() ; it++, i++) {
		DatabaseConnection* conn = *it;

		s.beginGroup(QString("db%1").arg(i));

		if (conn->getType() == DatabaseConnection::MySQL) {
			s.setValue("type", "mysql");
			s.setValue("host", conn->getMySQLHost());
			s.setValue("port", conn->getMySQLPort());
			s.setValue("user", conn->getMySQLUser());
			s.setValue("db", conn->getMySQLDatabaseName());
		} else {
			s.setValue("type", "sqlite");
			s.setValue("path", conn->getSQLiteFilePath());
		}

		s.setValue("connname", conn->getName());
		s.setValue("fileroot", conn->getFileRoot());

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


void System::displayStatusMessage(const QString& msg, unsigned int timeout)
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
	connect(task, SIGNAL(updated(int)), this, SLOT(taskUpdated(int)));

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


void System::jumpToPart(PartCategory* cat, unsigned int id)
{
	if (!mainWin  ||  id == INVALID_PART_ID)
		return;

	mainWin->jumpToPart(cat, id);
}


void System::jumpToContainer(unsigned int cid)
{
	if (!mainWin  ||  cid == INVALID_CONTAINER_ID)
		return;

	mainWin->jumpToContainer(cid);
}


void System::signalContainersChanged()
{
	emit containersChanged();
}


void System::signalDropAccepted(Qt::DropAction action, const char* dragIdentifier)
{
	emit dropAccepted(action, dragIdentifier);
}


void System::createTables()
{
	if (!hasValidDatabaseConnection()) {
		throw NoDatabaseConnectionException("Database connection needed to create tables!",
				__FILE__, __LINE__);
	}

	DatabaseConnection* conn = getCurrentDatabaseConnection();
	SQLDatabase sql = getCurrentSQLDatabase();

	if (conn->getType() == DatabaseConnection::MySQL) {
		sql.sendQuery((const UChar*) QString(
				"CREATE TABLE IF NOT EXISTS container (\n"
				"    id INT UNSIGNED NOT NULL AUTO_INCREMENT,\n"
				"    name VARCHAR(%1) NOT NULL,\n"
				"    PRIMARY KEY(id)\n"
				")"
				).arg(MAX_CONTAINER_NAME).utf16());
		sql.sendQuery((const UChar*) QString (
				"CREATE TABLE IF NOT EXISTS container_part (\n"
				"    cid INT UNSIGNED NOT NULL,\n"
				"    ptype VARCHAR(%1) NOT NULL,\n"
				"    pid INT UNSIGNED NOT NULL,\n"
				"    KEY(cid),\n"
				"    KEY(ptype),\n"
				"    KEY(pid)\n"
				")"
				).arg(MAX_PART_CATEGORY_TABLE_NAME).utf16());
	} else {
		sql.sendQuery((const UChar*) QString (
				"CREATE TABLE IF NOT EXISTS container (\n"
				"    id INTEGER NOT NULL,\n"
				"    name VARCHAR(%1) NOT NULL,\n"
				"    PRIMARY KEY(id)\n"
				")"
				).arg(MAX_CONTAINER_NAME).utf16());
		sql.sendQuery((const UChar*) QString (
				"CREATE TABLE IF NOT EXISTS container_part (\n"
				"    cid INTEGER NOT NULL,\n"
				"    ptype VARCHAR(%1) NOT NULL,\n"
				"    pid INTEGER NOT NULL\n"
				")"
				).arg(MAX_PART_CATEGORY_TABLE_NAME).utf16());
		sql.sendQuery(u"CREATE INDEX IF NOT EXISTS container_part_cid_idx ON container_part (cid)");
		sql.sendQuery(u"CREATE INDEX IF NOT EXISTS container_part_ptype_idx ON container_part (ptype)");
		sql.sendQuery(u"CREATE INDEX IF NOT EXISTS container_part_pid_idx ON container_part (pid)");
	}

	for (PartCategoryIterator it = partCats.begin() ; it != partCats.end() ; it++) {
		PartCategory* cat = *it;
		cat->createTables();
	}
}


QStringList System::getInstalledLanguages() const
{
	QStringList res;

	QStringList qmFiles = QDir(":/").entryList(QStringList() << "electronics_*.qm", QDir::Files, QDir::NoSort);

	for (QString qmFile : qmFiles) {
		QRegExp langRegex("electronics_(.*).qm", Qt::CaseSensitive, QRegExp::RegExp);
		langRegex.exactMatch(qmFile);
		QString langCode = langRegex.cap(1);
		res << langCode;
	}

	return res;
}


QString System::getLanguageName(const QString& langCode) const
{
	QTranslator ttrans;
	ttrans.load(QString(":/electronics_%1.qm").arg(langCode));
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


