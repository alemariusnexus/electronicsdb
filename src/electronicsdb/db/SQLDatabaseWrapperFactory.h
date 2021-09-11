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

#include <tuple>
#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include "SQLDatabaseWrapper.h"

#include "mysql/MySQLDatabaseWrapper.h"
#include "psql/PSQLDatabaseWrapper.h"
#include "sqlite/SQLiteDatabaseWrapper.h"

namespace electronicsdb
{


class SQLDatabaseWrapperFactory
{
private:
    using WrapperTuple = std::tuple<
            MySQLDatabaseWrapper,
            PSQLDatabaseWrapper,
            SQLiteDatabaseWrapper
            >;
    using WrapperDefault = SQLiteDatabaseWrapper;

public:
    static SQLDatabaseWrapperFactory& getInstance();

public:
    QList<SQLDatabaseWrapper*> createAll();
    SQLDatabaseWrapper* create(const QSqlDatabase& db = QSqlDatabase::database());
    SQLDatabaseWrapper* create(const QString& driverName);
    SQLDatabaseWrapper* createDefault();

private:
    template <size_t idx = 0>
    std::enable_if_t<idx < std::tuple_size_v<WrapperTuple>, void> createInternal (
            QList<SQLDatabaseWrapper*>& wrappers
    ) {
        using WrapperT = std::tuple_element_t<idx, WrapperTuple>;
        wrappers << new WrapperT;
        createInternal<idx+1>(wrappers);
    }

    template <size_t idx = 0>
    std::enable_if_t<idx >= std::tuple_size_v<WrapperTuple>, void> createInternal (
            QList<SQLDatabaseWrapper*>& wrappers
    ) {}
};


}
