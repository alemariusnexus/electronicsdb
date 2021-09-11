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

#include "SQLUpdateCommand.h"

#include <memory>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/SQLException.h"

namespace electronicsdb
{


SQLUpdateCommand::SQLUpdateCommand(const QString& tableName, const QString& idField, dbid_t id)
        : tableName(tableName), idField(idField), id(id)
{
}


void SQLUpdateCommand::setValues(const DataMap& data)
{
    values = data;
}


void SQLUpdateCommand::addFieldValue(const QString& fieldName, const QVariant& newValue)
{
    values.insert(fieldName, newValue);
}


void SQLUpdateCommand::doCommit()
{
    if (values.empty()) {
        return;
    }

    QSqlDatabase db = getSQLConnection();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QString queryStr("SELECT ");

    for (auto it = values.begin() ; it != values.end() ; it++) {
        QString fieldName = it.key();

        if (it != values.begin()) {
            queryStr += ",";
        }
        queryStr += dbw->escapeIdentifier(fieldName);
    }

    queryStr += QString(" FROM %1 WHERE %2=%3")
            .arg(dbw->escapeIdentifier(tableName), dbw->escapeIdentifier(idField)).arg(id);

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, queryStr, "Error selecting old values for SQLUpdateCommand");

    if (!query.next()) {
        throw InvalidValueException (
                QString("Invalid number of rows returned for SQLUpdateCommand::commit() SELECT query").toUtf8().constData(),
                __FILE__, __LINE__);
    }

    revertQuery = QString("UPDATE %1 SET ").arg(dbw->escapeIdentifier(tableName));
    revertBindParamValues.clear();

    int i = 0;
    for (auto it = values.begin() ; it != values.end() ; it++, i++) {
        QString fieldName = it.key();

        if (it != values.begin()) {
            revertQuery += ",";
        }

        revertBindParamValues << query.value(i);
        revertQuery += QString("%1=?").arg(dbw->escapeIdentifier(fieldName));
    }

    revertQuery += QString(" WHERE %1=%2").arg(dbw->escapeIdentifier(idField)).arg(id);

    queryStr = QString("UPDATE %1 SET ").arg(dbw->escapeIdentifier(tableName));
    QList<QVariant> bindParamValues;

    i = 0;
    for (auto it = values.begin() ; it != values.end() ; it++, i++) {
        QString fieldName = it.key();
        QVariant val = it.value();

        if (it != values.begin()) {
            queryStr += ",";
        }

        bindParamValues << val;
        queryStr += QString("%1=?").arg(dbw->escapeIdentifier(fieldName));
    }

    queryStr += QString(" WHERE %1=%2").arg(dbw->escapeIdentifier(idField)).arg(id);

    if (!query.prepare(queryStr)) {
        throw SQLException(query, "Error preparing SQLUpdateCommand", __FILE__, __LINE__);
    }
    for (int i = 0 ; i < bindParamValues.size() ; i++) {
        query.bindValue(i, bindParamValues[i]);
    }
    dbw->execAndCheckPreparedQuery(query, "Error executing SQLUpdateCommand");
}


void SQLUpdateCommand::doRevert()
{
    if (revertQuery.isNull()) {
        return;
    }

    QSqlDatabase db = getSQLConnection();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);

    if (!query.prepare(revertQuery)) {
        throw SQLException(query, "Error preparing SQLUpdateCommand revert query", __FILE__, __LINE__);
    }
    for (int i = 0 ; i < revertBindParamValues.size() ; i++) {
        query.bindValue(i, revertBindParamValues[i]);
    }
    dbw->execAndCheckPreparedQuery(query, "Error reverting SQLUpdateCommand");
}


}
