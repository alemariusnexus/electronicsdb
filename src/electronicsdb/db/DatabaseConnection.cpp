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

#include "DatabaseConnection.h"

#include <QUuid>
#include <nxcommon/exception/InvalidValueException.h>

namespace electronicsdb
{


DatabaseConnection::DatabaseConnection(const SQLDatabaseWrapper::DatabaseType& dbType, const QString& id)
        : dbType(dbType), id(id.isNull() ? QUuid::createUuid().toString() : id), keepPwInVault(false)
{
}

DatabaseConnection::DatabaseConnection(const DatabaseConnection& other)
        : dbType(other.dbType), id(other.id), name(other.name), fileRoot(other.fileRoot),
          keepPwInVault(other.keepPwInVault)
{
}

QString DatabaseConnection::getPasswordVaultID() const
{
    return QString("dbconn_pw:%1").arg(id);
}

void DatabaseConnection::closeDatabase(const QString& connName)
{
    QSqlDatabase::removeDatabase(connName);
}

void DatabaseConnection::saveToSettings(QSettings& s)
{
    s.setValue("id", id);
    s.setValue("driver", dbType.driverName);
    s.setValue("name", name);
    s.setValue("file_root", fileRoot);
    s.setValue("pw_in_vault", keepPwInVault);
}

void DatabaseConnection::loadFromSettings(const QSettings& s, const QString& dbNamePlaceholderValue)
{
    if (s.value("driver").toString() != dbType.driverName) {
        throw InvalidValueException("Invalid driver for DatabaseConnection::loadFromSettings()", __FILE__, __LINE__);
    }
    if (s.contains("id")) {
        id = s.value("id").toString();
    }
    setName(s.value("name").toString());
    setFileRoot(s.value("file_root").toString());
    setKeepPasswordInVault(s.value("pw_in_vault", false).toBool());
}

QString DatabaseConnection::replaceDbNamePlaceholder(const QString& str, const QString& value) const
{
    if (value.isNull()) {
        return str;
    }
    return QString(str).replace("{PLACEHOLDER}", value);
}


}
