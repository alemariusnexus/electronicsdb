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

#include "SQLiteDatabaseConnection.h"

#include "../../exception/SQLException.h"

namespace electronicsdb
{


SQLiteDatabaseConnection::SQLiteDatabaseConnection(const SQLDatabaseWrapper::DatabaseType& dbType, const QString& id)
        : DatabaseConnection(dbType, id)
{
}

SQLiteDatabaseConnection::SQLiteDatabaseConnection(const SQLiteDatabaseConnection& other)
        : DatabaseConnection(other), dbPath(other.dbPath)
{
}

bool SQLiteDatabaseConnection::needsPassword()
{
    return false;
}

QSqlDatabase SQLiteDatabaseConnection::openDatabase(const QString& pw, const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
                                                connName.isNull() ? QSqlDatabase::defaultConnection : connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        QString errmsg = QString("Error opening database: %1").arg(db.lastError().text());
        throw SQLException(errmsg.toUtf8().constData(), __FILE__, __LINE__);
    }
    return db;
}

QString SQLiteDatabaseConnection::getDescription()
{
    QString desc = QString(tr("SQLite - %1")).arg(dbPath);
    if (!name.isNull()) {
        desc = QString(tr("%1 (%2)")).arg(name, desc);
    }
    return desc;
}

DatabaseConnection* SQLiteDatabaseConnection::clone() const
{
    return new SQLiteDatabaseConnection(*this);
}

void SQLiteDatabaseConnection::saveToSettings(QSettings& s)
{
    DatabaseConnection::saveToSettings(s);

    s.setValue("db", dbPath);
}

void SQLiteDatabaseConnection::loadFromSettings(const QSettings& s, const QString& dbNamePlaceholderValue)
{
    DatabaseConnection::loadFromSettings(s);

    setDatabasePath(replaceDbNamePlaceholder(s.value("db", dbPath).toString(), dbNamePlaceholderValue));
}


}
