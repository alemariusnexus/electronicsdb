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

#include "SQLInsertCommand.h"

#include <memory>
#include <QHash>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "../../System.h"

namespace electronicsdb
{


SQLInsertCommand::SQLInsertCommand(const QString& tableName, const QString& idField, const QString& connName)
        : SQLCommand(connName), tableName(tableName), idField(idField)
{
}


SQLInsertCommand::SQLInsertCommand (
        const QString& tableName, const QString& idField, const DataMapList& data, const QString& connName
)       : SQLCommand(connName), tableName(tableName), idField(idField), data(data)
{
}


SQLInsertCommand::SQLInsertCommand (
        const QString& tableName, const QString& idField, const DataMap& data, const QString& connName
)       : SQLCommand(connName), tableName(tableName), idField(idField), data({data})
{
}


void SQLInsertCommand::setData(const DataMapList& data)
{
    this->data = data;
}


void SQLInsertCommand::doCommit()
{
    if (data.empty()) {
        return;
    }

    QSqlDatabase db = getSQLConnection();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    // Partition input data by the set of fields they each have. This is necessary because all rows in a bulk INSERT
    // statement must have values for all fields. While MySQL and PostgreSQL have a DEFAULT keyword that can be used
    // to skip such values, SQLite does not have it, so we need one INSERT statement per such partition.
    QHash<QSet<QString>, QList<int>> dataIndicesByFields;
    for (int i = 0 ; i < data.size() ; i++) {
        QSet<QString> fields(data[i].keyBegin(), data[i].keyEnd());
        dataIndicesByFields[fields] << i;
    }

    QList<dbid_t> newInsIDs;
    for (int i = 0 ; i < data.size() ; i++) {
        newInsIDs << -1;
    }

    for (auto dit = dataIndicesByFields.begin() ; dit != dataIndicesByFields.end() ; ++dit) {
        const QSet<QString>& fieldSet = dit.key();
        const QList<int>& dataIndices = dit.value();
        assert(!dataIndices.empty());

        QList<QVariant> boundParamValues;

        // All rows given by dataIndices are guaranteed to have the same set of fields.

        QList<QString> fields = data[dataIndices[0]].keys();

        QString nameCode;
        for (const QString& field : fields) {
            if (!nameCode.isEmpty()) {
                nameCode += ",";
            }
            nameCode += dbw->escapeIdentifier(field);
        }

        QString valueCode("");

        bool appendIDs = false;

        if (!fieldSet.contains(idField)) {
            // If this command was committed (and reverted) before, we'll use the IDs from the first commit instead of
            // letting the DBMS choose new ones.
            if (!insIDs.empty()) {
                appendIDs = true;
                if (!nameCode.isEmpty()) {
                    nameCode += ",";
                }
                nameCode += dbw->escapeIdentifier(idField);
            }
        }

        if (nameCode.isEmpty()) {
            // Special case when we don't have any column values. This requires a special syntax in many SQL backends.
            assert(insIDs.empty());

            // We don't have IDs for the new values (neither explicitly specified ones, nor automatically assigned ones
            // from a previously reverted commit).
            QList<dbid_t> multiIDs;
            dbw->insertMultipleDefaultValuesWithReturnID(tableName, idField, dataIndices.size(), multiIDs);
            assert(multiIDs.size() == dataIndices.size());

            for (int i = 0 ; i < dataIndices.size() ; i++) {
                newInsIDs[dataIndices[i]] = multiIDs[i];
            }
        } else {
            for (int dataIdx : dataIndices) {
                const DataMap & d = data[dataIdx];

                if (!valueCode.isEmpty()) {
                    valueCode += ",";
                }
                valueCode += "(";

                bool first = true;
                for (const QString& field : fields) {
                    if (!first) {
                        valueCode += ",";
                    }
                    const QVariant& v = d[field];
                    boundParamValues << v;
                    valueCode += "?";
                    first = false;
                }

                if (appendIDs) {
                    if (!first) {
                        valueCode += ",";
                    }
                    valueCode += QString::number(insIDs[dataIdx]);
                }

                valueCode += ")";
            }

            QString queryStr = QString("INSERT INTO %1 (%2) VALUES %3")
                    .arg(dbw->escapeIdentifier(tableName), nameCode, valueCode);

            if (!fieldSet.contains(idField)  &&  insIDs.empty()) {
                // We don't have IDs for the new values (neither explicitly specified ones, nor automatically assigned ones
                // from a previously reverted commit).
                QList<dbid_t> multiIDs;
                dbw->insertMultipleWithReturnID (
                        queryStr,
                        idField,
                        multiIDs,
                        [&](QSqlQuery& query) {
                            for (int i = 0 ; i < boundParamValues.size() ; i++) {
                                query.bindValue(i, boundParamValues[i]);
                            }
                        }
                        );
                assert(multiIDs.size() == dataIndices.size());

                for (int i = 0 ; i < dataIndices.size() ; i++) {
                    newInsIDs[dataIndices[i]] = multiIDs[i];
                }
            } else {
                QSqlQuery query(db);

                if (!query.prepare(queryStr)) {
                    throw SQLException(query, "Error preparing insert command in SQLInsertCommand", __FILE__, __LINE__);
                }
                for (int i = 0 ; i < boundParamValues.size() ; i++) {
                    query.bindValue(i, boundParamValues[i]);
                }
                dbw->execAndCheckPreparedQuery(query, "Error inserting data in SQLInsertCommand");

                if (insIDs.isEmpty()) {
                    for (int dataIdx : dataIndices) {
                        newInsIDs[dataIdx] = VariantToDBID(data[dataIdx][idField]);
                    }
                }
            }
        }
    }

    if (insIDs.empty()) {
        insIDs = newInsIDs;
    }
}


void SQLInsertCommand::doRevert()
{
    if (data.empty()) {
        return;
    }

    QSqlDatabase db = getSQLConnection();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    if (insIDs.empty()) {
        throw InvalidStateException("Attempting to revert an SQLInsertCommand that wasn't committed!",
                __FILE__, __LINE__);
    }

    QString queryStr = QString("DELETE FROM %1 WHERE %2 IN (").arg(dbw->escapeIdentifier(tableName), dbw->escapeIdentifier(idField));

    bool first = true;
    for (dbid_t id : insIDs) {
        if (!first) {
            queryStr += ",";
        }
        queryStr += QString::number(id);
        first = false;
    }

    queryStr += ")";

    dbw->execAndCheckQuery(queryStr, "Error reverting SQLInsertCommand");
}


}
