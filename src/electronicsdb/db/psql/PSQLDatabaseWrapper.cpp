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

#include "PSQLDatabaseWrapper.h"

#include <limits>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <nxcommon/log.h>
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "../common/DefaultDatabaseConnection.h"
#include "../common/DefaultDatabaseConnectionWidget.h"

namespace electronicsdb
{


PSQLDatabaseWrapper::PSQLDatabaseWrapper()
        : SQLDatabaseWrapper()
{
}

PSQLDatabaseWrapper::PSQLDatabaseWrapper(const QSqlDatabase& db)
        : SQLDatabaseWrapper(db)
{
}

DatabaseConnection* PSQLDatabaseWrapper::createConnection(SQLDatabaseWrapper::DatabaseType dbType)
{
    if (dbType.driverName == QString("QPSQL")) {
        DefaultDatabaseConnection* conn = new DefaultDatabaseConnection(dbType);
        conn->setPort(5432);
        return conn;
    }
    return nullptr;
}

DatabaseConnectionWidget* PSQLDatabaseWrapper::createWidget(SQLDatabaseWrapper::DatabaseType dbType, QWidget* parent)
{
    if (dbType.driverName == QString("QPSQL")) {
        DefaultDatabaseConnectionWidget* w = new DefaultDatabaseConnectionWidget(dbType, parent);
        w->setHost("localhost");
        w->setPort(5432);
        w->setUser("electronicsdb");
        w->setDatabase("electronicsdb");
        w->rememberDefaultValues();
        return w;
    }
    return nullptr;
}

void PSQLDatabaseWrapper::dropDatabaseIfExists(DatabaseConnection* conn, const QString& pw)
{
    DefaultDatabaseConnection* connToDrop = dynamic_cast<DefaultDatabaseConnection*>(conn);
    if (!connToDrop) {
        throw SQLException("Invalid database connection: Not a PostgreSQL connection", __FILE__, __LINE__);
    }
    execInTempConnection(conn, pw, [=](QSqlDatabase& serviceDb) {
        QSqlQuery query(serviceDb);
        execAndCheckQuery(query, QString("DROP DATABASE IF EXISTS %1")
                .arg(escapeIdentifier(connToDrop->getDatabaseName())));
    });
}

void PSQLDatabaseWrapper::createDatabaseIfNotExists(DatabaseConnection* conn, const QString& pw)
{
    DefaultDatabaseConnection* connToDrop = dynamic_cast<DefaultDatabaseConnection*>(conn);
    if (!connToDrop) {
        throw SQLException("Invalid database connection: Not a PostgreSQL connection", __FILE__, __LINE__);
    }
    execInTempConnection(conn, pw, [=](QSqlDatabase& serviceDb) {
        QSqlQuery query(serviceDb);

        execAndCheckQuery(query, QString("SELECT COUNT(*) FROM pg_database WHERE datname=%1")
                .arg(escapeString(connToDrop->getDatabaseName())));
        query.next();

        if (query.value(0).toUInt() == 0) {
            execAndCheckQuery(query, QString("CREATE DATABASE %1")
                    .arg(escapeIdentifier(connToDrop->getDatabaseName())));
        }
    });
}

void PSQLDatabaseWrapper::execInTempConnection (
        DatabaseConnection* conn, const QString& pw,
        const std::function<void (QSqlDatabase&)>& func
) {
    std::unique_ptr<DatabaseConnection> serviceConnUncast(conn->clone());
    DefaultDatabaseConnection* serviceConn = dynamic_cast<DefaultDatabaseConnection*>(serviceConnUncast.get());
    assert(serviceConn);

    serviceConn->setDatabaseName("postgres");

    QString serviceDbConnName;

    {
        QSqlDatabase serviceDb = serviceConn->openDatabase(pw, "mysqlServiceConnTemp");
        serviceDbConnName = serviceDb.connectionName();

        QSqlDatabase oldDb = getDatabase();
        setDatabase(serviceDb);

        try {
            func(serviceDb);
        } catch (std::exception&) {
            setDatabase(oldDb);
            throw;
        }

        setDatabase(oldDb);
    }

    QSqlDatabase::removeDatabase(serviceDbConnName);
}

QString PSQLDatabaseWrapper::groupConcatCode(const QString& expr, const QString& sep)
{
    return QString("string_agg(%1||'',%2)").arg(expr, escapeString(sep));
}

QString PSQLDatabaseWrapper::groupJsonArrayCode(const QString& expr)
{
    return QString("jsonb_agg(%1)").arg(expr);
}

QString PSQLDatabaseWrapper::jsonArrayCode(const QStringList& elems)
{
    return QString("jsonb_build_array(%1)").arg(elems.join(','));
}

QString PSQLDatabaseWrapper::generateDropTriggerIfExistsCode(const QString& triggerName, const QString& tableName)
{
    return QString("DROP TRIGGER IF EXISTS %1 ON %2").arg(escapeIdentifier(triggerName), escapeIdentifier(tableName));
}

void PSQLDatabaseWrapper::initSession()
{
    QSqlQuery query(db);
    execAndCheckQuery(query, "SELECT version()", "Error fetching PostgreSQL version");
    if (!query.next()) {
        throw SQLException("Error fetching PostgreSQL version", __FILE__, __LINE__);
    }

    QString versionStr = query.value(0).toString();

    db.driver()->setProperty("edb_dbVersionText", versionStr);

    QString shiftedVersionStr = versionStr.mid(versionStr.indexOf(QRegularExpression("\\d+\\.\\d+")));
    int verMajor, verMinor, verPatch;
    ParseVersionNumber(shiftedVersionStr, &verMajor, &verMinor, &verPatch);
    int version = EDB_BUILD_VERSION(verMajor, verMinor, verPatch);

    LogDebug("PostgreSQL server version: %s", versionStr.toUtf8().constData());

    if (version < psqlMinVersion) {
        throw SQLException(QString("Unsupported PostgreSQL version %1.%2.%3 (required: >= %4.%5.%6)")
                .arg(verMajor).arg(verMinor).arg(verPatch)
                .arg(EDB_EXTRACT_VERSION_MAJOR(psqlMinVersion))
                .arg(EDB_EXTRACT_VERSION_MINOR(psqlMinVersion))
                .arg(EDB_EXTRACT_VERSION_PATCH(psqlMinVersion))
                .toUtf8().constData(),
                __FILE__, __LINE__);
    }

    // Repress annoying log messages from libpq, e.g. when tables exist in CREATE TABLE IF NOT EXISTS.
    execAndCheckQuery(query, "SET client_min_messages TO WARNING",
                      "Error changing PSQL client log level");

    SQLDatabaseWrapper::initSession();
}

QList<SQLDatabaseWrapper::DatabaseType> PSQLDatabaseWrapper::getSupportedDatabaseTypes()
{
    QList<DatabaseType> dbTypes;

    DatabaseType type;
    type.driverName = "QPSQL";
    type.userReadableName = tr("PostgreSQL");
    dbTypes << type;

    return dbTypes;
}

QString PSQLDatabaseWrapper::generateIntPrimaryKeyCode(const QString& fieldName)
{
    return QString("%1 SERIAL PRIMARY KEY NOT NULL CHECK(%1 >= 0)")
            .arg(escapeIdentifier(fieldName));
}

QString PSQLDatabaseWrapper::generateDoubleTypeCode()
{
    // No, plain DOUBLE does not exist in PSQL. I swear to god...
    return "DOUBLE PRECISION";
}

void PSQLDatabaseWrapper::createRowTrigger (
        const QString& triggerName,
        const QString& tableName,
        SQLEvent triggerEvt,
        const QString& body
) {
    // PSQL doesn't support the BEGIN ... END type of triggers, so we have to define a separate function.

    QSqlQuery query(db);

    QString triggerEvtUpper;
    switch (triggerEvt) {
    case SQLInsert: triggerEvtUpper = "INSERT"; break;
    case SQLUpdate: triggerEvtUpper = "UPDATE"; break;
    case SQLDelete: triggerEvtUpper = "DELETE"; break;
    default: assert(false);
    }
    QString triggerEvtLower = triggerEvtUpper.toLower();

    execAndCheckQuery(query, QString(
            "CREATE OR REPLACE FUNCTION %1_func()\n"
            "RETURNS trigger AS $$\n"
            "BEGIN\n"
            "%2"
            "    RETURN NULL;\n"
            "END; $$\n"
            "LANGUAGE 'plpgsql'"
            ).arg(triggerName,
                  body),
            QString("Error creating trigger function '%1_func'").arg(triggerName));

    dropTriggerIfExists(triggerName, tableName);
    execAndCheckQuery(query, QString(
            "CREATE TRIGGER %1\n"
            "AFTER %2 ON %3\n"
            "FOR EACH ROW\n"
            "EXECUTE PROCEDURE %1_func()"
            ).arg(triggerName,
                  triggerEvtUpper,
                  tableName),
            QString("Error creating trigger '%1'").arg(triggerName));
}

void PSQLDatabaseWrapper::adjustAutoIncrementColumn(const QString& tableName, const QString& colName)
{
    execAndCheckQuery(QString(
            "SELECT setval(%1, (SELECT MAX(%3) FROM %2))"
            ).arg(escapeString(QString("%1_%2_seq").arg(tableName, colName)),
                  escapeIdentifier(tableName),
                  escapeIdentifier(colName)),
            "Error adjusting sequence value");
}

void PSQLDatabaseWrapper::createIndexIfNotExists(const QString& table, const QString& column)
{
    checkDatabaseValid();

    execAndCheckQuery(QString(
            "CREATE INDEX IF NOT EXISTS %1_%2_idx ON %1 (%2)"
            ).arg(table, column),
            QString("Error creating index '%1_%2_idx'").arg(table, column));
}

QList<QMap<QString, QVariant>> PSQLDatabaseWrapper::listTriggers()
{
    checkDatabaseValid();

    QList<QMap<QString, QVariant>> triggers;

    QSqlQuery query(db);
    execAndCheckQuery(query, "SELECT trigger_name, event_object_table FROM information_schema.triggers",
                      "Error listing triggers");
    while (query.next()) {
        QMap<QString, QVariant> t;
        t["name"] = query.value(0).toString();
        t["table"] = query.value(1).toString();
        triggers << t;
    }

    return triggers;
}

QList<QMap<QString, QVariant>> PSQLDatabaseWrapper::listColumns(const QString& table, bool fullMetadata)
{
    checkDatabaseValid();

    QSqlQuery query(db);
    execAndCheckQuery(query, QString(
            "SELECT column_name, data_type, character_maximum_length\n"
            "FROM   information_schema.columns\n"
            "WHERE  table_schema='public'\n"
            "   AND table_name=%1"
            ).arg(escapeString(table)),
                      "Error listing table columns");

    QSqlRecord record = db.record(table);

    QList<QMap<QString, QVariant>> data;
    while (query.next()) {
        QMap<QString, QVariant> cdata;

        cdata["name"] = query.value("column_name").toString();
        QSqlField field = record.field(cdata["name"].toString());

        if (fullMetadata) {
            if (!query.isNull("character_maximum_length")) {
                cdata["stringLenMax"] = query.value("character_maximum_length").toUInt();
            }

            QString type = query.value("data_type").toString().toLower();

            int64_t intMin, intMax;
            if (getIntegerTypeRange(type, false, intMin, intMax)) {
                cdata["intMin"] = static_cast<qlonglong>(intMin);
                cdata["intMax"] = static_cast<qlonglong>(intMax);
            }

            cdata["default"] = field.defaultValue();
        }

        data << cdata;
    }

    return data;
}

void PSQLDatabaseWrapper::insertMultipleDefaultValuesWithReturnID (
        const QString& tableName, const QString& idField,
        int count,
        QList<dbid_t>& ids
) {
    if (count <= 0) {
        return;
    }
    QString queryStr = QString("INSERT INTO %1 (%2) VALUES ").arg(escapeIdentifier(tableName),
                                                                  escapeIdentifier(idField));
    for (int i = 0 ; i < count ; i++) {
        if (i != 0) {
            queryStr += ',';
        }
        queryStr += "(DEFAULT)";
    }
    insertMultipleWithReturnID(queryStr, idField, ids);
}

QString PSQLDatabaseWrapper::getDatabaseDescription()
{
    return db.driver()->property("edb_dbVersionText").toString();
}

int PSQLDatabaseWrapper::getDatabaseLimit(SQLDatabaseWrapper::LimitType type)
{
    switch (type) {
    case LimitMaxBindParams:
        // https://stackoverflow.com/a/6583841/1566841
        return 34464;
    case LimitMaxQueryLength:
        // It might be even higher in newer versions, but we certainly don't need more than that.
        return 1073741823;
    default:
        assert(false);
        return -1;
    }
}

QString PSQLDatabaseWrapper::getPartPropertySQLType(PartProperty* prop)
{
    PartProperty::Type type = prop->getType();

    if (type == PartProperty::String) {
        // For PSQL, we don't get any space or performance benefits by choosing VARCHAR(...)
        return "TEXT";
    } else if (type == PartProperty::Integer) {
        int64_t imin = prop->getIntegerMinimum();
        int64_t imax = prop->getIntegerMaximum();

        if (imin >= std::numeric_limits<int16_t>::min()  &&  imax <= std::numeric_limits<int16_t>::max()) {
            return "SMALLINT";
        } else if (imin >= std::numeric_limits<int32_t>::min()  &&  imax <= std::numeric_limits<int32_t>::max()) {
            return "INTEGER";
        } else {
            return "BIGINT";
        }
    } else if (type == PartProperty::Float) {
        return "DOUBLE PRECISION";
    } else if (type == PartProperty::Decimal) {
        // In PSQL, storage requirements are based on the actual precision of each individual number, so it would be
        // pointless to specify arbitrary limits here. This way, we can store values of arbitrary precision (up to a
        // PSQL internal limit).
        return "DECIMAL";
    } else if (type == PartProperty::Boolean) {
        return "BOOLEAN";
    } else if (type == PartProperty::File) {
        return "TEXT";
    } else if (type == PartProperty::Date) {
        return "DATE";
    } else if (type == PartProperty::Time) {
        return "TIME";
    } else if (type == PartProperty::DateTime) {
        return "TIMESTAMP";
    } else {
        assert(false);
    }

    return QString("__MISSING_TYPE__");
}


}
