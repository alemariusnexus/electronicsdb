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

#include "MySQLDatabaseWrapper.h"

#include <limits>
#include <memory>
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


MySQLDatabaseWrapper::MySQLDatabaseWrapper()
        : SQLDatabaseWrapper()
{
}

MySQLDatabaseWrapper::MySQLDatabaseWrapper(const QSqlDatabase& db)
        : SQLDatabaseWrapper(db)
{
}

DatabaseConnection* MySQLDatabaseWrapper::createConnection(SQLDatabaseWrapper::DatabaseType dbType)
{
    if (dbType.driverName == QString("QMYSQL")) {
        DefaultDatabaseConnection* conn = new DefaultDatabaseConnection(dbType);
        conn->setPort(3306);
        return conn;
    }
    return nullptr;
}

DatabaseConnectionWidget* MySQLDatabaseWrapper::createWidget(SQLDatabaseWrapper::DatabaseType dbType, QWidget* parent)
{
    if (dbType.driverName == QString("QMYSQL")) {
        DefaultDatabaseConnectionWidget* w = new DefaultDatabaseConnectionWidget(dbType, parent);
        w->setHost("localhost");
        w->setPort(3306);
        w->setDatabase("electronicsdb");
        w->setUser("electronicsdb");
        w->rememberDefaultValues();
        return w;
    }
    return nullptr;
}

void MySQLDatabaseWrapper::dropDatabaseIfExists(DatabaseConnection* conn, const QString& pw)
{
    DefaultDatabaseConnection* connToDrop = dynamic_cast<DefaultDatabaseConnection*>(conn);
    if (!connToDrop) {
        throw SQLException("Invalid database connection: Not a MySQL connection", __FILE__, __LINE__);
    }
    execInTempConnection(conn, pw, [=](QSqlDatabase& serviceDb) {
        QSqlQuery query(serviceDb);
        execAndCheckQuery(query, QString("DROP DATABASE IF EXISTS %1")
                .arg(escapeIdentifier(connToDrop->getDatabaseName())));
    });
}

void MySQLDatabaseWrapper::createDatabaseIfNotExists(DatabaseConnection* conn, const QString& pw)
{
    DefaultDatabaseConnection* connToDrop = dynamic_cast<DefaultDatabaseConnection*>(conn);
    if (!connToDrop) {
        throw SQLException("Invalid database connection: Not a MySQL connection", __FILE__, __LINE__);
    }
    execInTempConnection(conn, pw, [=](QSqlDatabase& serviceDb) {
        QSqlQuery query(serviceDb);
        execAndCheckQuery(query, QString("CREATE DATABASE IF NOT EXISTS %1")
                .arg(escapeIdentifier(connToDrop->getDatabaseName())));
    });
}

void MySQLDatabaseWrapper::execInTempConnection (
        DatabaseConnection* conn, const QString& pw,
        const std::function<void (QSqlDatabase&)>& func
) {
    std::unique_ptr<DatabaseConnection> serviceConnUncast(conn->clone());
    DefaultDatabaseConnection* serviceConn = dynamic_cast<DefaultDatabaseConnection*>(serviceConnUncast.get());
    assert(serviceConn);

    serviceConn->setDatabaseName(QString());

    QString serviceDbConnName;

    {
        QSqlDatabase serviceDb = serviceConn->openDatabase(pw, "mysqlServiceConnTemp");
        serviceDbConnName = serviceDb.connectionName();

        func(serviceDb);
    }

    QSqlDatabase::removeDatabase(serviceDbConnName);
}

void MySQLDatabaseWrapper::initSession()
{
    QSqlQuery query(db);
    execAndCheckQuery(query, "SELECT version()", "Error fetching MySQL/MariaDB version");
    if (!query.next()) {
        throw SQLException("Error fetching MySQL/MariaDB version", __FILE__, __LINE__);
    }

    QString versionStr = query.value(0).toString();

    db.driver()->setProperty("edb_dbVersionText", versionStr);

    int verMajor, verMinor, verPatch;
    ParseVersionNumber(versionStr, &verMajor, &verMinor, &verPatch);
    int version = EDB_BUILD_VERSION(verMajor, verMinor, verPatch);

    LogDebug("MySQL server version: %s", versionStr.toUtf8().constData());

    if (versionStr.contains("mariadb", Qt::CaseInsensitive)) {
        if (version < mariadbMinVersion) {
            throw SQLException(QString("Unsupported MariaDB version %1.%2.%3 (required: >= %4.%5.%6)")
                    .arg(verMajor).arg(verMinor).arg(verPatch)
                    .arg(EDB_EXTRACT_VERSION_MAJOR(mariadbMinVersion))
                    .arg(EDB_EXTRACT_VERSION_MINOR(mariadbMinVersion))
                    .arg(EDB_EXTRACT_VERSION_PATCH(mariadbMinVersion))
                    .toUtf8().constData(),
                    __FILE__, __LINE__);
        }
    } else {
        if (version < mysqlMinVersion) {
            throw SQLException(QString("Unsupported MySQL version %1.%2.%3 (required: >= %4.%5.%6)")
                    .arg(verMajor).arg(verMinor).arg(verPatch)
                    .arg(EDB_EXTRACT_VERSION_MAJOR(mysqlMinVersion))
                    .arg(EDB_EXTRACT_VERSION_MINOR(mysqlMinVersion))
                    .arg(EDB_EXTRACT_VERSION_PATCH(mysqlMinVersion))
                    .toUtf8().constData(),
                    __FILE__, __LINE__);
        }
    }

    execAndCheckQuery(query, "SET sql_mode='PIPES_AS_CONCAT,NO_AUTO_VALUE_ON_ZERO'", "Error setting sql_mode");

    execAndCheckQuery(query, "SHOW VARIABLES LIKE 'max_allowed_packet'");
    if (query.next()) {
        int64_t maxAllowedPacket64 = query.value("Value").toLongLong();
        int maxAllowedPacket = maxAllowedPacket64 > std::numeric_limits<int>::max()
                ? std::numeric_limits<int>::max() : static_cast<int>(maxAllowedPacket64);
        db.driver()->setProperty("edb_maxAllowedPacket", maxAllowedPacket);
    }
}

QString MySQLDatabaseWrapper::generateUpsertPrefixCode(const QStringList& conflictTarget)
{
    // MySQL doesn't provide a way to specify conflict targets, but that's fine: They are there only to support
    // databases where a conflict target is MANDATORY. The UPSERTs used throughout the program don't rely on the
    // conflict target being honored.
    return "ON DUPLICATE KEY UPDATE";
}

QList<SQLDatabaseWrapper::DatabaseType> MySQLDatabaseWrapper::getSupportedDatabaseTypes()
{
    QList<DatabaseType> dbTypes;

    DatabaseType type;
    type.driverName = "QMYSQL";
    type.userReadableName = tr("MySQL/MariaDB");
    dbTypes << type;

    return dbTypes;
}

QString MySQLDatabaseWrapper::generateIntPrimaryKeyCode(const QString& fieldName)
{
    // NOTE: MySQL/MariaDB don't support CHECK constraints in AUTO_INCREMENT columns.
    return QString("%1 INTEGER PRIMARY KEY NOT NULL AUTO_INCREMENT")
            .arg(escapeIdentifier(fieldName));
}

QString MySQLDatabaseWrapper::groupConcatCode(const QString& expr, const QString& sep)
{
    return QString("group_concat(%1 SEPARATOR %2)").arg(expr, escapeString(sep));
}

QString MySQLDatabaseWrapper::groupJsonArrayCode(const QString& expr)
{
    return QString("json_arrayagg(%1)").arg(expr);
}

void MySQLDatabaseWrapper::createIndexIfNotExists(const QString& table, const QString& column)
{
    checkDatabaseValid();

    // MySQL doesn't support CREATE INDEX IF NOT EXISTS...
    // Adapted from https://dba.stackexchange.com/a/62997
    QSqlQuery query(db);
    execAndCheckQuery(QString(
            "SELECT IF (\n"
            "    EXISTS(\n"
            "        SELECT DISTINCT index_name from information_schema.statistics\n"
            "        WHERE  table_schema = '%1'\n"
            "           AND table_name = '%2'\n"
            "           AND index_name = '%3'\n"
            "    )\n"
            "    ,'SELECT ''exists'' _______;'\n"
            "    ,'CREATE INDEX %2_%3_idx on %2(%3)') into @a;\n"
            "PREPARE stmt1 FROM @a;\n"
            "EXECUTE stmt1;\n"
            "DEALLOCATE PREPARE stmt1;"
            ).arg(db.databaseName(), table, column),
            QString("Error creating index '%1_%2_idx'").arg(table, column));
}

QList<QMap<QString, QVariant>> MySQLDatabaseWrapper::listTriggers()
{
    checkDatabaseValid();

    QList<QMap<QString, QVariant>> triggers;

    QSqlQuery query(db);
    execAndCheckQuery(query, "SHOW TRIGGERS", "Error listing triggers");
    while (query.next()) {
        QMap<QString, QVariant> t;
        t["name"] = query.value("Trigger").toString();
        t["table"] = query.value("Table").toString();
        triggers << t;
    }

    return triggers;
}

QList<QMap<QString, QVariant>> MySQLDatabaseWrapper::listColumns(const QString& table, bool fullMetadata)
{
    checkDatabaseValid();

    QSqlQuery query(db);
    execAndCheckQuery(query, QString(
            "SELECT COLUMN_NAME, COLUMN_DEFAULT, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH, COLUMN_TYPE\n"
            "FROM   information_schema.columns\n"
            "WHERE  table_name=%1"
            ).arg(escapeString(table)),
            "Error listing table columns");

    QList<QMap<QString, QVariant>> data;
    while (query.next()) {
        QMap<QString, QVariant> cdata;

        cdata["name"] = query.value("COLUMN_NAME").toString();

        if (fullMetadata) {
            if (!query.isNull("CHARACTER_MAXIMUM_LENGTH")) {
                cdata["stringLenMax"] = query.value("CHARACTER_MAXIMUM_LENGTH").toUInt();
            }

            QString type = query.value("DATA_TYPE").toString().toLower();

            bool unsignedType = query.value("COLUMN_TYPE").toString().toLower().contains("unsigned");
            int64_t intMin, intMax;
            if (getIntegerTypeRange(type, unsignedType, intMin, intMax)) {
                cdata["intMin"] = static_cast<qlonglong>(intMin);
                cdata["intMax"] = static_cast<qlonglong>(intMax);
            }

            // QSqlField::field() is not available in the QMYSQL driver, so we get it from information_schema. This
            // means we have to do the parsing ourselves. We'll see when it starts breaking...
            QString defVal = query.value("COLUMN_DEFAULT").toString();

            if (defVal == QLatin1String("NULL")) {
                defVal = QString();
            }

            // It might have single quotes surrounding it. The code below is used in both the QSQLITE and QPSQL driver,
            // so it should be alright.
            if (!defVal.isEmpty() && defVal.at(0) == QLatin1Char('\'')) {
                const int end = defVal.lastIndexOf(QLatin1Char('\''));
                if (end > 0) {
                    defVal = defVal.mid(1, end - 1);
                }
            }

            cdata["default"] = defVal;
        }

        data << cdata;
    }

    return data;
}

void MySQLDatabaseWrapper::insertMultipleWithReturnID (
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

    // NOTE: MySQL doesn't support INSERT ... RETURNING, but there's another way: As long as the storage engine
    // is InnoDB (which has been the default for a long time), a single bulk-insert is guaranteed to assign consecutive
    // values for AUTO_INCREMENT fields, as long as innodb_autoinc_lock_mode is left at its default. In that case,
    // MySQL will return the ID of the FIRST inserted record with QSqlQuery::lastInsertId().
    // See: https://dev.mysql.com/doc/refman/8.0/en/innodb-auto-increment-handling.html
    dbid_t firstID = VariantToDBID(query.lastInsertId());
    for (int i = 0 ; i < query.numRowsAffected() ; i++) {
        ids << firstID + i;
    }
}

void MySQLDatabaseWrapper::insertMultipleDefaultValuesWithReturnID (
        const QString& tableName, const QString& idField,
        int count,
        QList<dbid_t>& ids
) {
    if (count <= 0) {
        return;
    }
    // MySQL/MariaDB actually supports empty column lists in INSERT. Finally something it does better than the others...
    QString queryStr = QString("INSERT INTO %1 () VALUES ").arg(escapeIdentifier(tableName));
    for (int i = 0 ; i < count ; i++) {
        if (i != 0) {
            queryStr += ',';
        }
        queryStr += "()";
    }
    insertMultipleWithReturnID(queryStr, idField, ids);
}

QString MySQLDatabaseWrapper::getDatabaseDescription()
{
    return db.driver()->property("edb_dbVersionText").toString();
}

int MySQLDatabaseWrapper::getDatabaseLimit(SQLDatabaseWrapper::LimitType type)
{
    switch (type) {
    case LimitMaxBindParams:
        // There doesn't seem to be a definite limit for MySQL, so we just guess. There is talk about a 16-bit limit,
        // so we'll be safe and assume a signed 16-bit value.
        return std::numeric_limits<int16_t>::max();
    case LimitMaxQueryLength:
        return db.driver()->property("edb_maxAllowedPacket").toInt();
    default:
        assert(false);
        return -1;
    }
}

QString MySQLDatabaseWrapper::getPartPropertySQLType(PartProperty* prop)
{
    PartProperty::Type type = prop->getType();

    if (type == PartProperty::String) {
        int maxLen = prop->getStringMaximumLength();

        // We don't use VARCHAR(...), because that just adds an arbitrary length limit without any storage or
        // performance benefits.
        if (maxLen < 0) {
            return "LONGTEXT";
        } else if (maxLen <= std::numeric_limits<uint8_t>::max()) {
            return "TINYTEXT";
        } else if (maxLen <= std::numeric_limits<uint16_t>::max()) {
            return "TEXT";
        } else if (maxLen <= 16777215) {
            return "MEDIUMTEXT";
        } else {
            return "LONGTEXT";
        }
    } else if (type == PartProperty::Integer) {
        int64_t imin = prop->getIntegerMinimum();
        int64_t imax = prop->getIntegerMaximum();

        // We could use unsigned types, but let's not complicate things even further.
        if (imin >= std::numeric_limits<int8_t>::min()  &&  imax <= std::numeric_limits<int8_t>::max()) {
            return "TINYINT";
        } else if (imin >= std::numeric_limits<int16_t>::min()  &&  imax <= std::numeric_limits<int16_t>::max()) {
            return "SMALLINT";
        } else if (imin >= -8388608  &&  imax <= 8388607) {
            return "MEDIUMINT";
        } else if (imin >= std::numeric_limits<int32_t>::min()  &&  imax <= std::numeric_limits<int32_t>::max()) {
            return "INTEGER";
        } else {
            return "BIGINT";
        }
    } else if (type == PartProperty::Float) {
        return "DOUBLE";
    } else if (type == PartProperty::Decimal) {
        // Yes, this is overkill. It consumes a whoppin 24 bytes per value. But if someone actively decides to use
        // Decimal instead of Float - which in itself is usually overkill -, I'd rather be very conservative by default.
        // This range covers the entire named SI prefix range (yocto to yotta) and still has some digits leftover. If
        // that's too much or too little, it can still be edited in the SQL manually.
        return "DECIMAL(54,27)";
    } else if (type == PartProperty::Boolean) {
        return "BOOLEAN";
    } else if (type == PartProperty::File) {
        return "TEXT";
    } else if (type == PartProperty::Date) {
        return "DATE";
    } else if (type == PartProperty::Time) {
        return "TIME";
    } else if (type == PartProperty::DateTime) {
        return "DATETIME";
    } else {
        assert(false);
    }

    return QString("__MISSING_TYPE__");
}

PartProperty::DefaultValueFunction MySQLDatabaseWrapper::parsePartPropertyDefaultValueFunction (
        PartProperty* prop, const QVariant& sqlDefaultVal
) {
    PartProperty::DefaultValueFunction func = SQLDatabaseWrapper::parsePartPropertyDefaultValueFunction(
            prop, sqlDefaultVal);
    if (func != PartProperty::DefaultValueConst) {
        return func;
    }

    QString normVal = sqlDefaultVal.toString().toLower().trimmed();
    PartProperty::Type type = prop->getType();

    if (type == PartProperty::Date  &&  normVal == "curdate()") {
        return PartProperty::DefaultValueNow;
    } else if (type == PartProperty::Time  &&  normVal == "curtime()") {
        return PartProperty::DefaultValueNow;
    } else if (type == PartProperty::DateTime  &&  normVal == "current_timestamp()") {
        return PartProperty::DefaultValueNow;
    }

    return PartProperty::DefaultValueConst;
}


}
