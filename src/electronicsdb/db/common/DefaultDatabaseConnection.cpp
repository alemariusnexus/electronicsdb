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

#include "DefaultDatabaseConnection.h"

#include <nxcommon/log.h>
#include "../../exception/SQLException.h"

namespace electronicsdb
{


DefaultDatabaseConnection::DefaultDatabaseConnection(const SQLDatabaseWrapper::DatabaseType& dbType, const QString& id)
        : DatabaseConnection(dbType, id), port(-1), keepPwInVault(false)
{
}

DefaultDatabaseConnection::DefaultDatabaseConnection(const DefaultDatabaseConnection& other)
        : DatabaseConnection(other), host(other.host), user(other.user), port(other.port), dbName(other.dbName),
          keepPwInVault(other.keepPwInVault)
{
}

bool DefaultDatabaseConnection::needsPassword()
{
    return true;
}

QSqlDatabase DefaultDatabaseConnection::openDatabase(const QString& pw, const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(getDriverName(),
                                                connName.isNull() ? QSqlDatabase::defaultConnection : connName);
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(pw);
    if (!db.open()) {
        QString errmsg = QString("Error opening database: %1").arg(db.lastError().text());
        throw SQLException(errmsg.toUtf8().constData(), __FILE__, __LINE__);
    }
    return db;
}

QString DefaultDatabaseConnection::getDescription()
{
    QString desc = QString(tr("%1 - %2@%3:%4/%5")).arg(dbType.userReadableName, user, host).arg(port).arg(dbName);
    if (!name.isNull()) {
        desc = QString(tr("%1 (%2)")).arg(name, desc);
    }
    return desc;
}

DatabaseConnection* DefaultDatabaseConnection::clone() const
{
    return new DefaultDatabaseConnection(*this);
}

void DefaultDatabaseConnection::saveToSettings(QSettings& s)
{
    DatabaseConnection::saveToSettings(s);

    s.setValue("host", host);
    s.setValue("port", port);
    s.setValue("user", user);
    s.setValue("db", dbName);
}

void DefaultDatabaseConnection::loadFromSettings(const QSettings& s, const QString& dbNamePlaceholderValue)
{
    DatabaseConnection::loadFromSettings(s);

    setHost(s.value("host", host).toString());
    setPort(s.value("port", port).toUInt());
    setUser(s.value("user", user).toString());
    setDatabaseName(replaceDbNamePlaceholder(s.value("db", dbName).toString(), dbNamePlaceholderValue));
}


}
