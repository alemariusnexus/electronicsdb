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

#ifndef DATABASECONNECTION_H_
#define DATABASECONNECTION_H_

#include "global.h"
#include <QtCore/QObject>
#include <QtCore/QString>


class DatabaseConnection : public QObject
{
	Q_OBJECT

public:
	enum Type
	{
		MySQL,
		SQLite
	};

public:
	DatabaseConnection(Type type) : type(type), mysqlPort(0) {}
	DatabaseConnection(const DatabaseConnection& other);


	QString getName() const { return name; }
	QString getFileRoot() const { return fileRoot; }

	Type getType() const { return type; }

	QString getMySQLHost() const { return mysqlHost; }
	QString getMySQLUser() const { return mysqlUser; }
	unsigned int getMySQLPort() const { return mysqlPort; }
	QString getMySQLDatabaseName() const { return mysqlDB; }

	QString getSQLiteFilePath() const { return sqliteFilePath; }


	void setName(const QString& name) { this->name = name; }
	void setFileRoot(const QString& path) { fileRoot = path; }

	void setType(Type type) { this->type = type; }

	void setMySQLHost(const QString& host) { mysqlHost = host; }
	void setMySQLUser(const QString& user) { mysqlUser = user; }
	void setMySQLPort(unsigned int port) { mysqlPort = port; }
	void setMySQLDatabaseName(const QString& db) { this->mysqlDB = db; }

	void setSQLiteFilePath(const QString& path) { sqliteFilePath = path; }


	void setFrom(const DatabaseConnection* other);


protected:
	QString name;
	QString fileRoot;

	Type type;

	QString mysqlHost;
	QString mysqlUser;
	unsigned int mysqlPort;
	QString mysqlDB;

	QString sqliteFilePath;
};

#endif /* DATABASECONNECTION_H_ */
