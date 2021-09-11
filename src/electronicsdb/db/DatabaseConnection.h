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

#include "../global.h"

#include <QObject>
#include <QSettings>
#include <QSqlDatabase>
#include <QString>
#include "SQLDatabaseWrapper.h"

namespace electronicsdb
{


class DatabaseConnection : public QObject
{
    Q_OBJECT

public:
    DatabaseConnection(const SQLDatabaseWrapper::DatabaseType& dbType, const QString& id = QString());
    DatabaseConnection(DatabaseConnection&& other) = delete;

    SQLDatabaseWrapper::DatabaseType getDatabaseType() const { return dbType; }
    QString getDriverName() const { return dbType.driverName; }

    QString getID() const { return id; }

    QString getName() const { return name; }
    QString getFileRoot() const { return fileRoot; }

    void setName(const QString& name) { this->name = name; }
    void setFileRoot(const QString& path) { fileRoot = path; }

    void setKeepPasswordInVault(bool keep) { keepPwInVault = keep; }
    bool isKeepPasswordInVault() const { return keepPwInVault; }

    QString getPasswordVaultID() const;

    virtual bool needsPassword() = 0;

    // Do NOT call these yourself to connect/disconnect. Use the methods in the System class instead!
    virtual QSqlDatabase openDatabase(const QString& pw, const QString& connName = QString()) = 0;
    virtual void closeDatabase(const QString& connName);

    virtual QString getDescription() = 0;

    virtual DatabaseConnection* clone() const = 0;

    virtual void saveToSettings(QSettings& s);
    virtual void loadFromSettings(const QSettings& s, const QString& dbNamePlaceholderValue = QString());

protected:
    DatabaseConnection(const DatabaseConnection& other);

    QString replaceDbNamePlaceholder(const QString& str, const QString& value) const;

protected:
    SQLDatabaseWrapper::DatabaseType dbType;

    QString id;
    QString name;
    QString fileRoot;
    bool keepPwInVault;
};


}
