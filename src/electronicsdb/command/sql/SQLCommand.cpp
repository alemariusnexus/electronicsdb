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

#include "SQLCommand.h"

#include "../../exception/NoDatabaseConnectionException.h"
#include "../../System.h"

namespace electronicsdb
{


SQLCommand::SQLCommand(const QString& connName)
        : connName(connName), committed(false), reverted(false)
{
}

QSqlDatabase SQLCommand::getSQLConnection()
{
    QString actualConnName = connName;

    if (connName.isNull()) {
        System* sys = System::getInstance();

        if (!sys->hasValidDatabaseConnection()) {
            throw NoDatabaseConnectionException("Database connection needed to use SQL command", __FILE__, __LINE__);
        }

        actualConnName = QSqlDatabase::defaultConnection;
    }

    QSqlDatabase sql = QSqlDatabase::database(actualConnName);

    if (!sql.isValid()  ||  !sql.isOpen()) {
        throw NoDatabaseConnectionException("Database connection needed to use SQL command", __FILE__, __LINE__);
    }

    return sql;
}

void SQLCommand::commit()
{
    doCommit();
    if (listener) {
        listener(this, OpCommit);
    }
    committed = true;
}

void SQLCommand::revert()
{
    doRevert();
    if (listener) {
        listener(this, OpRevert);
    }
    reverted = true;
}


}
