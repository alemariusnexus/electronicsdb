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

#include <QList>
#include <QString>
#include "SQLCommand.h"

namespace electronicsdb
{


class SQLDeleteCommand : public SQLCommand
{
public:
    SQLDeleteCommand(const QString& tableName, const QString& whereClause, const QList<QVariant>& bindParamVals = {},
                     const QString& connName = QString());
    SQLDeleteCommand(const QString& tableName, const QString& idField, const QList<dbid_t>& ids,
            const QString& connName = QString());

    void setWhereClause(const QString& whereClause, const QList<QVariant>& bindParamVals = {});
    void setWhereClauseFromIDs(const QString& idField, const QList<dbid_t>& ids);

protected:
    void doCommit() override;
    void doRevert() override;

protected:
    QString tableName;
    QString whereClause;
    QList<QVariant> whereClauseBindParamValues;

    QString revertQuery;
    QList<QVariant> revertQueryBindParamValues;
};


}
