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

#include "../../global.h"

#include "../DatabaseConnection.h"
#include "../SQLDatabaseWrapper.h"

namespace electronicsdb
{


class DefaultDatabaseConnection : public DatabaseConnection
{
    Q_OBJECT

public:
    DefaultDatabaseConnection(const SQLDatabaseWrapper::DatabaseType& dbType, const QString& id = QString());
    DefaultDatabaseConnection(const DefaultDatabaseConnection& other);

    QString getHost() const { return host; }
    QString getUser() const { return user; }
    int getPort() const { return port; }
    QString getDatabaseName() const { return dbName; }


    void setHost(const QString& host) { this->host = host; }
    void setUser(const QString& user) { this->user = user; }
    void setPort(int port) { this->port = port; }
    void setDatabaseName(const QString& db) { this->dbName = db; }

    bool needsPassword() override;
    QSqlDatabase openDatabase(const QString& pw, const QString& connName) override;

    QString getDescription() override;

    DatabaseConnection* clone() const override;

    void saveToSettings(QSettings& s) override;
    void loadFromSettings(const QSettings& s, const QString& dbNamePlaceholderValue) override;

private:
    QString host;
    QString user;
    int port;
    QString dbName;
    bool keepPwInVault;
};


}
