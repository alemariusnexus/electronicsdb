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

#include "SQLiteDatabaseWrapper.h"

#include <QFileInfo>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <nxcommon/log.h>
#include "../../exception/IOException.h"
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "SQLiteDatabaseConnection.h"
#include "SQLiteDatabaseConnectionWidget.h"

namespace electronicsdb
{


SQLiteDatabaseWrapper::SQLiteDatabaseWrapper()
        : SQLDatabaseWrapper()
{
}

SQLiteDatabaseWrapper::SQLiteDatabaseWrapper(const QSqlDatabase& db)
        : SQLDatabaseWrapper(db)
{
}

DatabaseConnection* SQLiteDatabaseWrapper::createConnection(SQLDatabaseWrapper::DatabaseType dbType)
{
    if (dbType.driverName == QString("QSQLITE")) {
        SQLiteDatabaseConnection* conn = new SQLiteDatabaseConnection(dbType);
        return conn;
    }
    return nullptr;
}

DatabaseConnectionWidget* SQLiteDatabaseWrapper::createWidget(SQLDatabaseWrapper::DatabaseType dbType, QWidget* parent)
{
    if (dbType.driverName == QString("QSQLITE")) {
        SQLiteDatabaseConnectionWidget* w = new SQLiteDatabaseConnectionWidget(dbType, parent);
        return w;
    }
    return nullptr;
}

void SQLiteDatabaseWrapper::dropDatabaseIfExists(DatabaseConnection* conn, const QString& pw)
{
    SQLiteDatabaseConnection* sqliteConn = dynamic_cast<SQLiteDatabaseConnection*>(conn);
    if (!sqliteConn) {
        throw SQLException("Invalid database connection: Not an SQLite database", __FILE__, __LINE__);
    }

    QString dbPath = sqliteConn->getDatabasePath();
    if (dbPath == QString(":memory:")) {
        return;
    }

    QFile dbFile(dbPath);

    if (dbFile.exists()) {
        if (!dbFile.remove()) {
            throw IOException("Error removing SQLite database file", __FILE__, __LINE__);
        }
    }
}

void SQLiteDatabaseWrapper::createDatabaseIfNotExists(DatabaseConnection* conn, const QString& pw)
{
    SQLiteDatabaseConnection* sqliteConn = dynamic_cast<SQLiteDatabaseConnection*>(conn);
    if (!sqliteConn) {
        throw SQLException("Invalid database connection: Not an SQLite database", __FILE__, __LINE__);
    }

    QString dbPath = sqliteConn->getDatabasePath();
    if (dbPath == QString(":memory:")) {
        return;
    }

    QFile dbFile(dbPath);

    if (!dbFile.exists()) {
        if (!dbFile.open(QIODevice::WriteOnly)) {
            throw IOException("Error creating SQLite database file", __FILE__, __LINE__);
        }
    }
}

QString SQLiteDatabaseWrapper::groupJsonArrayCode(const QString& expr)
{
    // TODO: Check that the Json1 extension is loaded (and load it with the program)
    return QString("json_group_array(%1)").arg(expr);
}

void SQLiteDatabaseWrapper::initSession()
{
    QSqlQuery query(db);
    execAndCheckQuery(query, "SELECT sqlite_version()", "Error fetching SQLite version");
    if (!query.next()) {
        throw SQLException("Error fetching SQLite version");
    }

    QString versionStr = query.value(0).toString();

    db.driver()->setProperty("edb_dbVersionText", QString("SQLite %1").arg(versionStr));

    int verMajor, verMinor, verPatch;
    ParseVersionNumber(versionStr, &verMajor, &verMinor, &verPatch);
    int version = EDB_BUILD_VERSION(verMajor, verMinor, verPatch);

    LogDebug("SQLite library version: %s", versionStr.toUtf8().constData());

    if (version < sqliteMinVersion) {
        throw SQLException(QString("Unsupported SQLite version %1.%2.%3 (required: >= %4.%5.%6)")
                .arg(verMajor).arg(verMinor).arg(verPatch)
                .arg(EDB_EXTRACT_VERSION_MAJOR(sqliteMinVersion))
                .arg(EDB_EXTRACT_VERSION_MINOR(sqliteMinVersion))
                .arg(EDB_EXTRACT_VERSION_PATCH(sqliteMinVersion))
                .toUtf8().constData(),
                __FILE__, __LINE__);
    }

    execAndCheckQuery("PRAGMA foreign_keys=TRUE", "Error enabling SQLite foreign key support");

    SQLDatabaseWrapper::initSession();
}

QList<SQLDatabaseWrapper::DatabaseType> SQLiteDatabaseWrapper::getSupportedDatabaseTypes()
{
    QList<DatabaseType> dbTypes;

    DatabaseType type;
    type.driverName = "QSQLITE";
    type.userReadableName = tr("SQLite");
    dbTypes << type;

    return dbTypes;
}

QString SQLiteDatabaseWrapper::generateIntPrimaryKeyCode(const QString& fieldName)
{
    return QString("%1 INTEGER PRIMARY KEY NOT NULL CHECK(%1 >= 0)")
            .arg(escapeIdentifier(fieldName));
}

void SQLiteDatabaseWrapper::createIndexIfNotExists(const QString& table, const QString& column)
{
    checkDatabaseValid();

    execAndCheckQuery(QString(
            "CREATE INDEX IF NOT EXISTS %1_%2_idx ON %1 (%2)"
            ).arg(table, column),
            QString("Error creating index %1_%2_idx").arg(table, column));
}

QList<QMap<QString, QVariant>> SQLiteDatabaseWrapper::listTriggers()
{
    checkDatabaseValid();

    QList<QMap<QString, QVariant>> triggers;

    QSqlQuery query(db);
    execAndCheckQuery(query, "SELECT name, tbl_name FROM sqlite_master WHERE type='trigger'", "Error listing triggers");
    while (query.next()) {
        QMap<QString, QVariant> t;
        t["name"] = query.value(0).toString();
        t["table"] = query.value(1).toString();
        triggers << t;
    }

    return triggers;
}

QList<QMap<QString, QVariant>> SQLiteDatabaseWrapper::listColumns(const QString& table, bool fullMetadata)
{
    checkDatabaseValid();

    QSqlQuery query(db);
    execAndCheckQuery(query, QString("PRAGMA table_info(%1)").arg(escapeIdentifier(table)),
                      "Error listing table columns");

    QSqlRecord record = db.record(table);

    QList<QMap<QString, QVariant>> data;
    while (query.next()) {
        QMap<QString, QVariant> cdata;

        cdata["name"] = query.value("name").toString();
        QSqlField field = record.field(cdata["name"].toString());

        if (fullMetadata) {
            QString type = query.value("type").toString().toLower();
            
            cdata["default"] = field.defaultValue();

            // SQLite has no inherent maximum length for a string (apart from storage limits of course)

            // While SQLite stores all integer as int64_t, we'll still use tighter limits based on the declared
            // type, to be compatible with other DBMSes.
            int64_t intMin, intMax;
            if (getIntegerTypeRange(type, false, intMin, intMax)) {
                cdata["intMin"] = static_cast<qlonglong>(intMin);
                cdata["intMax"] = static_cast<qlonglong>(intMax);
            }
        }

        data << cdata;
    }

    return data;
}

void SQLiteDatabaseWrapper::insertMultipleWithReturnID (
        const QString& insStmt, const QString& idField, QList<dbid_t>& ids,
        const std::function<void (QSqlQuery&)>& postPrepare
) {
    checkDatabaseValid();

    QSqlQuery query(db);

    if (postPrepare) {
        if (!query.prepare(insStmt)) {
            throw SQLException(query, "Error preparing multi-insert query", __FILE__, __LINE__);
        }
        postPrepare(query);
        execAndCheckPreparedQuery(query, "Error executing prepared multi-insert query");
    } else {
        execAndCheckQuery(query, insStmt, "Error executing multi-insert query");
    }

    // SQLite supports INSERT ... RETURNING only since 3.35, and e.g. Qt 5.15.2 ships an older version. It should be
    // safe to assume that the returned IDs are consecutive in SQLite, and lastInsertId() returns the ID of the LAST
    // row that was inserted.
    // See: https://stackoverflow.com/a/53352324/1566841
    dbid_t lastID = VariantToDBID(query.lastInsertId());
    for (int i = 0 ; i < query.numRowsAffected() ; i++) {
        ids << lastID + i - (query.numRowsAffected()-1);
    }
}

void SQLiteDatabaseWrapper::insertMultipleDefaultValuesWithReturnID (
        const QString& tableName, const QString& idField,
        int count,
        QList<dbid_t>& ids
) {
    QSqlQuery query(db);
    QString queryStr = QString("INSERT INTO %1 DEFAULT VALUES").arg(escapeIdentifier(tableName));

    if (!query.prepare(queryStr)) {
        throw SQLException(query, "Error preparing default value insert query", __FILE__, __LINE__);
    }

    // There might be creative ways to do it in one statement (e.g. recursive CTEs), but since SQLite runs in the local
    // process this should be fast enough.
    while (count != 0) {
        execAndCheckPreparedQuery(query, "Error executing default value insert query");
        ids << VariantToDBID(query.lastInsertId());
        count--;
    }
}

void SQLiteDatabaseWrapper::truncateTable(const QString& tableName)
{
    execAndCheckQuery(QString("DELETE FROM %1").arg(escapeIdentifier(tableName)),
                      QString("Error truncating table '%1'").arg(tableName));
}

QString SQLiteDatabaseWrapper::getDatabaseDescription()
{
    return db.driver()->property("edb_dbVersionText").toString();
}

int SQLiteDatabaseWrapper::getDatabaseLimit(SQLDatabaseWrapper::LimitType type)
{
    // See: https://www.sqlite.org/limits.html
    // It would be better to query values at runtime, but then we'd have to link against SQLite ourselves, so we'll
    // just go with the default values and trust that nobody tinkers with them.
    switch (type) {
    case LimitMaxBindParams:
        // That's true only for SQLite >= 3.32.0
        return 32766;
    case LimitMaxQueryLength:
        return 1000000000;
    default:
        assert(false);
        return -1;
    }
}

QString SQLiteDatabaseWrapper::getPartPropertySQLType(PartProperty* prop)
{
    PartProperty::Type type = prop->getType();

    // SQLite has dynamic typing, so the exact types here aren't that important.
    if (type == PartProperty::String) {
        return "TEXT";
    } else if (type == PartProperty::Integer) {
        // While INTEGER would have the same effect for SQLite, the code in SQLDatabaseWrapper::getIntegerTypeRange()
        // returns different values for them.
        return "BIGINT";
    } else if (type == PartProperty::Float) {
        return "DOUBLE";
    } else if (type == PartProperty::Decimal) {
        return "DECIMAL";
    } else if (type == PartProperty::Boolean) {
        return "BOOLEAN";
    } else if (type == PartProperty::File) {
        return "TEXT";
    } else if (type == PartProperty::Date) {
        return "TEXT";
    } else if (type == PartProperty::Time) {
        return "TEXT";
    } else if (type == PartProperty::DateTime) {
        return "TEXT";
    } else {
        assert(false);
    }

    return QString("__MISSING_TYPE__");
}


}
