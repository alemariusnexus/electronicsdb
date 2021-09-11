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

#include "SQLDatabaseWrapperFactory.h"

#include "../exception/SQLException.h"

namespace electronicsdb
{


SQLDatabaseWrapperFactory& SQLDatabaseWrapperFactory::getInstance()
{
    static SQLDatabaseWrapperFactory inst;
    return inst;
}



QList<SQLDatabaseWrapper*> SQLDatabaseWrapperFactory::createAll()
{
    QList<SQLDatabaseWrapper*> wrappers;
    createInternal(wrappers);
    return wrappers;
}

SQLDatabaseWrapper* SQLDatabaseWrapperFactory::create(const QSqlDatabase& db)
{
    if (!db.isValid()) {
        throw SQLException("Database not opened", __FILE__, __LINE__);
    }

    SQLDatabaseWrapper* dbw = create(db.driverName());
    dbw->setDatabase(db);
    return dbw;
}

SQLDatabaseWrapper* SQLDatabaseWrapperFactory::create(const QString& driverName)
{
    QList<SQLDatabaseWrapper*> wrappers;
    createInternal(wrappers);

    SQLDatabaseWrapper* wrapper = nullptr;

    for (SQLDatabaseWrapper* wrapperCand : wrappers) {
        if (!wrapper) {
            for (auto dbType : wrapperCand->getSupportedDatabaseTypes()) {
                if (dbType.driverName == driverName) {
                    wrapper = wrapperCand;
                    break;
                }
            }
        }

        if (wrapper != wrapperCand) {
            delete wrapperCand;
        }
    }

    if (!wrapper) {
        throw SQLException("Unsupported database", __FILE__, __LINE__);
    }

    return wrapper;
}

SQLDatabaseWrapper* SQLDatabaseWrapperFactory::createDefault()
{
    return new WrapperDefault;
}


}
