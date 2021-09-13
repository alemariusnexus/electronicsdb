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

#include "SQLDeleteCommand.h"

#include <memory>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QVariant>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/SQLException.h"

namespace electronicsdb
{


SQLDeleteCommand::SQLDeleteCommand (
        const QString& tableName,
        const QString& whereClause,
        const QList<QVariant>& bindParamVals,
        const QString& connName
)       : SQLCommand(connName), tableName(tableName), whereClause(whereClause),
          whereClauseBindParamValues(bindParamVals)
{
}

SQLDeleteCommand::SQLDeleteCommand (
        const QString& tableName,
        const QString& idField,
        const QList<dbid_t>& ids,
        const QString& connName
)       : SQLCommand(connName), tableName(tableName)
{
    setWhereClauseFromIDs(idField, ids);
}

void SQLDeleteCommand::setWhereClause(const QString& whereClause, const QList<QVariant>& bindParamVals)
{
    this->whereClause = whereClause;
    this->whereClauseBindParamValues = bindParamVals;
}

void SQLDeleteCommand::setWhereClauseFromIDs(const QString& idField, const QList<dbid_t>& ids)
{
    QSqlDatabase db = getSQLConnection();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    if (ids.empty()) {
        // Empty lists for an IN(...) expression are not allowed, so we just use an expression that evaluates to false.
        setWhereClause("1=0");
    } else {
        QString idListStr("");
        for (int i = 0 ; i < ids.size() ; i++) {
            if (i != 0)
                idListStr += ",";
            idListStr += QString::number(ids[i]);
        }
        setWhereClause(QString("%1 IN (%2)").arg(dbw->escapeIdentifier(idField), idListStr));
    }
}

void SQLDeleteCommand::doCommit()
{
    QSqlDatabase db = getSQLConnection();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlRecord tableRecord = db.record(tableName);

    QStringList revertFields;
    QString revertFieldsStr;

    for (int i = 0 ; i < tableRecord.count() ; i++) {
        QSqlField field = tableRecord.field(i);
        revertFields << field.name();

        if (i != 0) {
            revertFieldsStr += ",";
        }
        revertFieldsStr += dbw->escapeIdentifier(field.name());
    }

    QSqlQuery query(db);

    QString queryStr = QString(
            "SELECT %1 FROM %2 WHERE %3"
            ).arg(revertFieldsStr, dbw->escapeIdentifier(tableName), whereClause);

    if (whereClauseBindParamValues.empty()) {
        dbw->execAndCheckQuery(query, queryStr, "Error selecting old values for SQLDeleteCommand");
    } else {
        if (!query.prepare(queryStr)) {
            throw SQLException(query, "Error preparing select statement for SQLDeleteCommand", __FILE__, __LINE__);
        }
        for (int i = 0 ; i < whereClauseBindParamValues.size() ; i++) {
            query.bindValue(i, whereClauseBindParamValues[i]);
        }
        dbw->execAndCheckPreparedQuery(query, "Error selecting old values for SQLDeleteCommand");
    }

    revertQueryBindParamValues.clear();
    QString valueCode("");

    int numRemoved = 0;

    bool first = true;
    while (query.next()) {
        if (!first)
            valueCode +=",";

        valueCode += "(";

        for (int i = 0 ; i < tableRecord.count() ; i++) {
            if (i != 0) {
                valueCode += ",";
            }

            revertQueryBindParamValues << query.value(i);
            valueCode += "?";
        }

        valueCode += ")";

        first = false;
        numRemoved++;
    }

    if (numRemoved != 0) {
        revertQuery = QString("INSERT INTO %1 (%2) VALUES %3")
                .arg(dbw->escapeIdentifier(tableName), revertFieldsStr, valueCode);

        queryStr = QString("DELETE FROM %1 WHERE %2").arg(dbw->escapeIdentifier(tableName), whereClause);

        if (whereClauseBindParamValues.empty()) {
            dbw->execAndCheckQuery(query, queryStr, "Error deleting data for SQLDeleteCommand");
        } else {
            if (!query.prepare(queryStr)) {
                throw SQLException(query, "Error preparing delete statement for SQLDeleteCommand", __FILE__, __LINE__);
            }
            for (int i = 0 ; i < whereClauseBindParamValues.size() ; i++) {
                query.bindValue(i, whereClauseBindParamValues[i]);
            }
            dbw->execAndCheckPreparedQuery(query, "Error deleting data for SQLDeleteCommand");
        }

        int numActuallyRemoved = query.numRowsAffected();
        if (numActuallyRemoved >= 0  &&  numRemoved != numActuallyRemoved) {
            throw InvalidStateException("Number of deleted rows doesn't match size of previous SELECT.",
                                        __FILE__, __LINE__);
        }
    } else {
        revertQuery = QString();
    }
}

void SQLDeleteCommand::doRevert()
{
    if (revertQuery.isEmpty())
        return;

    QSqlDatabase db = getSQLConnection();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);

    if (revertQueryBindParamValues.empty()) {
        dbw->execAndCheckQuery(query, revertQuery, "Error reverting SQLDeleteCommand");
    } else {
        if (!query.prepare(revertQuery)) {
            throw SQLException(query, "Error preparing revert statement for SQLDeleteCommand", __FILE__, __LINE__);
        }
        for (int i = 0 ; i < revertQueryBindParamValues.size() ; i++) {
            query.bindValue(i, revertQueryBindParamValues[i]);
        }
        dbw->execAndCheckPreparedQuery(query, "Error reverting SQLDeleteCommand");
    }
}


}
