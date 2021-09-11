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

#include "SQLTransaction.h"

#include "../exception/SQLException.h"

namespace electronicsdb
{


SQLTransaction::SQLTransaction()
        : active(false)
{
}

SQLTransaction::SQLTransaction(const QSqlDatabase& db)
        : db(db), active(false)
{
}

SQLTransaction::SQLTransaction(SQLTransaction&& other)
        : db(other.db), active(other.active)
{
    other.db = QSqlDatabase();
    other.active = false;
}

SQLTransaction::~SQLTransaction()
{
    rollback();
}

SQLTransaction& SQLTransaction::begin()
{
    if (active) {
        throw SQLException("Attempt to begin transaction while one is already active", __FILE__, __LINE__);
    }
    if (!db.transaction()) {
        throw SQLException("Error starting database transaction", __FILE__, __LINE__);
    }
    active = true;
    return *this;
}

SQLTransaction& SQLTransaction::commit()
{
    if (active) {
        db.commit();
        active = false;
    }
    return *this;
}

SQLTransaction& SQLTransaction::rollback()
{
    if (active) {
        db.rollback();
        active = false;
    }
    return *this;
}


}
