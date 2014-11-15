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

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "global.h"
#include <QtCore/QObject>
#include <QtCore/QLinkedList>
#include <QtCore/QStack>
#include "DatabaseConnection.h"
#include "MainWindow.h"
#include "EditStack.h"
#include "Task.h"
#include "PartCategory.h"
#include <mysql/mysql.h>
#include <nxcommon/sql/sql.h>


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

public:
	static System* getInstance();
	static void destroy();

public:
	~System();
	SQLDatabase getCurrentSQLDatabase() { return currentSqlDb; }
	EditStack* getEditStack() { return editStack; }
	DatabaseConnection* getCurrentDatabaseConnection() { return currentDbConn; }
	void addPermanentDatabaseConnection(DatabaseConnection* conn);
	void removePermanentDatabaseConnection(DatabaseConnection* conn);
	size_t getPermanentDatabaseConnectionSize() const { return permDbConns.size(); }
	DatabaseConnection* getPermanentDatabaseConnection(unsigned int idx) { return permDbConns[idx]; }
	DatabaseConnectionIterator getPermanentDatabaseConnectionsBegin() { return permDbConns.begin(); }
	DatabaseConnectionIterator getPermanentDatabaseConnectionsEnd() { return permDbConns.end(); }
	ConstDatabaseConnectionIterator getPermanentDatabaseConnectionsBegin() const { return permDbConns.begin(); }
	ConstDatabaseConnectionIterator getPermanentDatabaseConnectionsEnd() const { return permDbConns.end(); }
	bool isPermanentDatabaseConnection(DatabaseConnection* conn) const;
	void setStartupDatabaseConnection(DatabaseConnection* conn) { startupConn = conn; }
	DatabaseConnection* getStartupDatabaseConnection() { return startupConn; }
	bool connectDatabase(DatabaseConnection* conn, const QString& pw, bool dialogs = true, bool createTables = true);
	bool disconnect(bool ask = false);
	void emergencyDisconnect();
	void displayStatusMessage(const QString& msg, unsigned int timeout = 2500);
	bool hasValidDatabaseConnection() const;
	bool isEmergencyDisconnecting() const { return emergencyDisconnectRequested; }

	void addPartCategory(PartCategory* cat);
	PartCategoryIterator getPartCategoryBegin() { return partCats.begin(); }
	PartCategoryIterator getPartCategoryEnd() { return partCats.end(); }
	ConstPartCategoryIterator getPartCategoryBegin() const { return partCats.begin(); }
	ConstPartCategoryIterator getPartCategoryEnd() const { return partCats.end(); }
	PartCategoryList getPartCategories() const { return partCats; }

	void unhandeledException(Exception& ex);

	void savePermanentDatabaseConnectionSettings();

	void registerMainWindow(MainWindow* mw) { mainWin = mw; }

	void saveAll();

	void startTask(Task* task);
	void endTask(Task* task);

	bool hasQuit() const { return quitStatus; }

	void jumpToPart(PartCategory* cat, unsigned int id);

	void signalContainersChanged();

	void signalDropAccepted(Qt::DropAction action, const char* dragIdentifier);

	void createTables();

	void jumpToContainer(unsigned int cid);

public slots:
	void quit();
	void forceQuit();

private slots:
	void taskUpdated(int val);

private:
	void updateTaskStatus();

signals:
	void databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);
	void permanentDatabaseConnectionAdded(DatabaseConnection* conn);
	void permanentDatabaseConnectionRemoved(DatabaseConnection* conn);
	void permanentDatabaseConnectionSettingsSaved();
	void saveAllRequested();
	void containersChanged();
	void dropAccepted(Qt::DropAction action, const char* dragIdentifier);

private:
	System();

private:
	static System* instance;

private:
	MainWindow* mainWin;
	PartCategoryList partCats;
	DatabaseConnectionList permDbConns;
	DatabaseConnection* currentDbConn;
	DatabaseConnection* startupConn;
	SQLDatabase currentSqlDb;
	EditStack* editStack;
	QStack<Task*> tasks;
	bool quitStatus;
	bool currentlyConnecting;
	bool emergencyDisconnectRequested;
};

#endif /* SYSTEM_H_ */
