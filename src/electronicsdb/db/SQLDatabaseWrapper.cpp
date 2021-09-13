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

#include "SQLDatabaseWrapper.h"

#include <limits>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/log.h>
#include "../exception/SQLException.h"
#include "../util/elutil.h"
#include "../System.h"
#include "DatabaseConnection.h"
#include "DatabaseConnectionWidget.h"

namespace electronicsdb
{


SQLDatabaseWrapper::SQLDatabaseWrapper()
{
}

SQLDatabaseWrapper::SQLDatabaseWrapper(const QSqlDatabase& db)
        : db(db)
{
}

const QSqlDatabase& SQLDatabaseWrapper::getDatabase() const
{
    return db;
}

void SQLDatabaseWrapper::setDatabase(const QSqlDatabase& db)
{
    this->db = db;
}

void SQLDatabaseWrapper::initSession()
{
}

void SQLDatabaseWrapper::createSchema(bool transaction)
{
    QSqlQuery query(db);

    auto trans = transaction ? beginDDLTransaction() : SQLTransaction();


    // **********************************************
    // *                MAIN TABLES                 *
    // **********************************************

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS electronicsdb_settings (\n"
            "    keyid VARCHAR(64) PRIMARY KEY NOT NULL,\n"
            "    value TEXT\n"
            ")"
            ));

    QMap<QString, QString> dbSettings = queryDatabaseSettings();

    if (dbSettings.contains("db_version")) {
        int32_t dbVersion = dbSettings.value("db_version").toInt();

        if (dbVersion != programDbVersion) {
            throw SQLException(QString("Unsupported database version %1 (program expects %2)")
                    .arg(dbVersion).arg(programDbVersion).toUtf8().constData(), __FILE__, __LINE__);
        }
    }

    dbSettings["db_version"] = QString::number(programDbVersion);

    writeDatabaseSettings(dbSettings);

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS container (\n"
            "    %1,\n"
            "    name VARCHAR(%2) NOT NULL\n"
            ")"
            ).arg(generateIntPrimaryKeyCode("id")).arg(MAX_CONTAINER_NAME),
            "Error creating table 'container'");

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS part_category (\n"
            "    id VARCHAR(%1) PRIMARY KEY NOT NULL,\n"
            "    name VARCHAR(128) NOT NULL,\n"
            "    desc_pattern VARCHAR(512),\n"
            "    sortidx INTEGER DEFAULT 10000\n"
            ")"
            ).arg(MAX_PART_CATEGORY_TABLE_NAME),
            "Error creating table 'part_category'");

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS container_part (\n"
            "    %1,\n"
            "    cid INTEGER NOT NULL REFERENCES container(id),\n"
            "    ptype VARCHAR(%2) NOT NULL REFERENCES part_category(id),\n"
            "    pid INTEGER NOT NULL CHECK(pid >= 0),\n"
            "    cidx SMALLINT NOT NULL DEFAULT 0,\n"
            "    pidx SMALLINT NOT NULL DEFAULT 0"
            ")"
            ).arg(generateIntPrimaryKeyCode("id")).arg(MAX_PART_CATEGORY_TABLE_NAME),
            "Error creating table 'container_part'");
    createIndexIfNotExists("container_part", "cid");
    createIndexIfNotExists("container_part", "ptype");
    createIndexIfNotExists("container_part", "pid");

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS meta_type (\n"
            "    id VARCHAR(32) PRIMARY KEY NOT NULL,\n"
            "    \n"
            "%1\n"
            ")"
            ).arg(generateMetaTypeFieldDefCode()),
            "Error creating table 'meta_type'");

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS partlink_type (\n"
            "    id VARCHAR(64) PRIMARY KEY NOT NULL,\n"
            "\n"
            "    pcat_type CHAR(1) NOT NULL CHECK(pcat_type IN ('a', 'b', '+')),\n"
            "\n"
            "    name_a VARCHAR(128) NOT NULL,\n"
            "    name_b VARCHAR(128) NOT NULL,\n"
            "\n"
            "    flag_show_in_a BOOLEAN NOT NULL DEFAULT TRUE,\n"
            "    flag_show_in_b BOOLEAN NOT NULL DEFAULT TRUE,\n"
            "    flag_edit_in_a BOOLEAN NOT NULL DEFAULT TRUE,\n"
            "    flag_edit_in_b BOOLEAN NOT NULL DEFAULT TRUE,\n"
            "	 flag_display_selection_list_a BOOLEAN NOT NULL DEFAULT TRUE,\n"
            "	 flag_display_selection_list_b BOOLEAN NOT NULL DEFAULT TRUE\n"
            ")"
            ),
            "Error creating table 'partlink_type'");

    // NOTE: Depending on partlink_type.pcat_type, entries in this table could be inverted, i.e. they could indicate
    // the categories that a part link type is NOT part of. For this reason, you shouldn't store per-category config
    // data here. Do that in partlink_type_pcat instead.
    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS partlink_type_patternpcat (\n"
            "    lid VARCHAR(64) NOT NULL REFERENCES partlink_type(id),\n"
            "    side CHAR(1) NOT NULL CHECK(side IN ('a', 'b')),\n"
            "    pcat VARCHAR(%1) NOT NULL REFERENCES part_category(id),\n"
            "    PRIMARY KEY(lid, side, pcat)\n"
            ")"
            ).arg(MAX_PART_CATEGORY_TABLE_NAME),
            "Error creating table ''");

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS partlink_type_pcat (\n"
            "    lid VARCHAR(64) NOT NULL REFERENCES partlink_type(id),\n"
            "    side CHAR(1) NOT NULL CHECK(side IN ('a', 'b')),\n"
            "    pcat VARCHAR(%1) NOT NULL REFERENCES part_category(id),\n"
            "    sortidx INTEGER NOT NULL DEFAULT 10000000,\n"
            "    PRIMARY KEY(lid, side, pcat)\n"
            ")"
            ).arg(MAX_PART_CATEGORY_TABLE_NAME));

    execAndCheckQuery(query, QString(
            "CREATE TABLE IF NOT EXISTS partlink (\n"
            "    %1,\n"
            "    type VARCHAR(64) NOT NULL REFERENCES partlink_type(id),\n"
            "    ptype_a VARCHAR(%2) NOT NULL REFERENCES part_category(id),\n"
            "    pid_a INTEGER NOT NULL CHECK(pid_a >= 0),\n"
            "    ptype_b VARCHAR(%2) NOT NULL REFERENCES part_category(id),\n"
            "    pid_b INTEGER NOT NULL CHECK(pid_b >= 0),\n"
            "    idx_a SMALLINT NOT NULL DEFAULT 0,\n"
            "    idx_b SMALLINT NOT NULL DEFAULT 0\n"
            ")"
            ).arg(generateIntPrimaryKeyCode("id")).arg(MAX_PART_CATEGORY_TABLE_NAME),
            "Error creating table 'partlink'");


    createLogSchema();

    createAppModelDependentSchema();

    trans.commit();
}

void SQLDatabaseWrapper::destroySchema(bool transaction)
{
    QStringList tables = listTables();

    auto trans = transaction ? beginDDLTransaction() : SQLTransaction();

    auto dropTablesStartingWith = [&](const QString& prefix) {
        for (QString table : tables) {
            if (table.startsWith(prefix)) {
                dropTableIfExists(table);
            }
        }
    };

    for (auto trigger : listTriggers()) {
        dropTriggerIfExists(trigger["name"].toString(), trigger["table"].toString());
    }

    dropTablesStartingWith("clog_");
    dropTableIfExists("container_part");
    dropTableIfExists("partlink");
    dropTableIfExists("container");
    dropTablesStartingWith("pcatmv_");
    dropTablesStartingWith("pcatmeta_");
    dropTablesStartingWith("pcat_");
    dropTablesStartingWith("partlink_type_");
    dropTableIfExists("partlink_type");
    dropTableIfExists("part_category");
    dropTableIfExists("meta_type");
    dropTableIfExists("electronicsdb_settings");

    trans.commit();
}

void SQLDatabaseWrapper::createLogSchema()
{
    // **********************************************
    // *                LOG TABLES                  *
    // **********************************************

    // NOTE: These tables should NOT have foreign key references to dynamic data (e.g. to PIDs or CIDs), because they
    // will deliberately store these IDs even after the parent key was deleted.

    dropTableIfExists("clog_pcat");
    execAndCheckQuery(QString(
            "CREATE TABLE clog_pcat (\n"
            "    %1,\n"
            "    ptype VARCHAR(%2) NOT NULL REFERENCES part_category(id),\n"
            "    pid INTEGER NOT NULL CHECK(pid >= 0),\n"
            "    state VARCHAR(3) NOT NULL CHECK(state IN ('ins', 'upd', 'del')),\n"
            "    UNIQUE(ptype, pid)\n"
            ")"
            ).arg(generateIntPrimaryKeyCode("lid")).arg(MAX_PART_CATEGORY_TABLE_NAME),
            "Error creating table 'clog_pcat'");

    dropTableIfExists("clog_container");
    execAndCheckQuery(QString(
            "CREATE TABLE clog_container (\n"
            "    %1,\n"
            "    cid INTEGER NOT NULL UNIQUE CHECK(cid >= 0),\n"
            "    state VARCHAR(3) NOT NULL CHECK(state IN ('ins', 'upd', 'del'))\n"
            ")"
            ).arg(generateIntPrimaryKeyCode("lid")), "Error creating table 'clog_container'");


    // **********************************************
    // *                LOG TRIGGERS                *
    // **********************************************

    // TODO: Update linked items on main table update (e.g.: update linked parts when container name changes)

    for (auto evt : {SQLInsert, SQLUpdate, SQLDelete}) {
        createLogTrigger("container", evt,
                generateLogStateChangeCode(evt, evt, "clog_container", {"cid"}, {"id"})
        );
        createLogTrigger("container_part", evt,
                 generateLogStateChangeCode(evt, SQLUpdate, "clog_container", {"cid"}, {"cid"}) +
                 generateLogStateChangeCode(evt, SQLUpdate, "clog_pcat", {"ptype", "pid"}, {"ptype", "pid"})
        );
    }

    for (auto evt : {SQLInsert, SQLUpdate, SQLDelete}) {
        createLogTrigger("partlink", evt,
                 generateLogStateChangeCode(evt, SQLUpdate, "clog_pcat", {"ptype", "pid"}, {"ptype_a", "pid_a"}) +
                 generateLogStateChangeCode(evt, SQLUpdate, "clog_pcat", {"ptype", "pid"}, {"ptype_b", "pid_b"})
        );
    }
}

void SQLDatabaseWrapper::createAppModelDependentSchema()
{
    QStringList tableNames = listTables();

    for (QString tableName : tableNames) {
        if (!tableName.startsWith("pcat_")) {
            continue;
        }
        QString ptype = tableName.mid(static_cast<int>(strlen("pcat_")));

        for (auto evt : {SQLInsert, SQLUpdate, SQLDelete}) {
            createLogTrigger(tableName, evt,
                             generateLogStateChangeCode(evt, evt, "clog_pcat", {"ptype", "pid"},
                                                        {QString("'%1'").arg(ptype), "id"})
            );

            for (QString mvTableName : tableNames) {
                if (!mvTableName.startsWith(QString("pcatmv_%1_").arg(ptype))) {
                    continue;
                }
                createLogTrigger(mvTableName, evt,
                                 generateLogStateChangeCode(evt, SQLUpdate, "clog_pcat", {"ptype", "pid"},
                                                            {QString("'%1'").arg(ptype), "pid"})
                );
            }
        }
    }
}

QString SQLDatabaseWrapper::generateMetaTypeFieldDefCode()
{
    return QString(
            "    name VARCHAR(128),\n"
            "    type VARCHAR(32) DEFAULT 'string' NOT NULL,\n"
            "    \n"
            "    unit_suffix VARCHAR(32),\n"
            "    display_unit_affix VARCHAR(16),\n"
            "    \n"
            "    int_range_min BIGINT,\n"
            "    int_range_max BIGINT,\n"
            "    \n"
            "    decimal_range_min %1,\n"
            "    decimal_range_max %1,\n"
            "    \n"
            "    string_max_length INTEGER,\n"
            "    \n"
            "    flag_full_text_indexed BOOLEAN,\n"
            "    flag_multi_value BOOLEAN,\n"
            "    flag_si_prefixes_default_to_base2 BOOLEAN,\n"
            "    flag_display_with_units BOOLEAN,\n"
            "    flag_display_selection_list BOOLEAN,\n"
            "    flag_display_hide_from_listing_table BOOLEAN,\n"
            "    flag_display_text_area BOOLEAN,\n"
            "    flag_display_multi_in_single_field BOOLEAN,\n"
            "    flag_display_dynamic_enum BOOLEAN,\n"
            "    \n"
            "    bool_true_text VARCHAR(128),\n"
            "    bool_false_text VARCHAR(128),\n"
            "    \n"
            "    ft_user_prefix VARCHAR(64),\n"
            "    \n"
            "    textarea_min_height SMALLINT,\n"
            "    \n"
            "    sql_natural_order_code VARCHAR(512),\n"
            "    sql_ascending_order_code VARCHAR(512),\n"
            "    sql_descending_order_code VARCHAR(512)"
            ).arg(generateDoubleTypeCode());
}

void SQLDatabaseWrapper::createRowTrigger (
        const QString& triggerName,
        const QString& tableName,
        SQLEvent triggerEvt,
        const QString& body
) {
    QSqlQuery query(db);

    QString triggerEvtUpper;
    switch (triggerEvt) {
    case SQLInsert: triggerEvtUpper = "INSERT"; break;
    case SQLUpdate: triggerEvtUpper = "UPDATE"; break;
    case SQLDelete: triggerEvtUpper = "DELETE"; break;
    default: assert(false);
    }

    dropTriggerIfExists(triggerName, tableName);
    execAndCheckQuery(query, QString(
            "CREATE TRIGGER %1\n"
            "AFTER %2 ON %3\n"
            "FOR EACH ROW\n"
            "BEGIN\n"
            "%4"
            "END"
            ).arg(triggerName,
                  triggerEvtUpper,
                  tableName,
                  body),
            QString("Error creating trigger '%1'").arg(triggerName));
}

void SQLDatabaseWrapper::createLogTrigger (
        const QString& tableName,
        SQLEvent triggerEvt,
        const QString& body
) {
    QString triggerEvtLower;
    switch (triggerEvt) {
    case SQLInsert: triggerEvtLower = "insert"; break;
    case SQLUpdate: triggerEvtLower = "update"; break;
    case SQLDelete: triggerEvtLower = "delete"; break;
    default: assert(false);
    }
    QString triggerName = QString("trigger_clog_%1_%2").arg(tableName, triggerEvtLower);
    createRowTrigger(triggerName, tableName, triggerEvt, body);
}

QString SQLDatabaseWrapper::generateLogStateChangeCode (
        SQLEvent triggerEvt,
        SQLEvent logEvt,
        const QString& logTable,
        const QStringList& idFields,
        const QStringList& idValuesArg
) {
    assert(!idFields.empty());
    assert(idFields.size() == idValuesArg.size());

    QStringList idValues;

    for (const QString& val : idValuesArg) {
        if (!val.isEmpty()  &&  (val[0].isLetter()  ||  val[0] == QChar('_'))) {
            if (triggerEvt == SQLDelete) {
                idValues << QString("OLD.%1").arg(val);
            } else {
                idValues << QString("NEW.%1").arg(val);
            }
        } else {
            idValues << val;
        }
    }

    QString upsertPrefix = generateUpsertPrefixCode(idFields);

    if (logEvt == SQLInsert) {
        return QString(
                "    INSERT INTO %1 (%2, state) VALUES (%3, 'ins') %4 state='upd';\n"
                ).arg(logTable,
                      idFields.join(", "),
                      idValues.join(", "),
                      upsertPrefix);
    } else if (logEvt == SQLUpdate) {
        return QString(
                "    INSERT INTO %1 (%2, state) VALUES (%3, 'upd') %4 state="
                    "(CASE %1.state WHEN 'ins' THEN 'ins' ELSE 'upd' END);\n"
                ).arg(logTable,
                      idFields.join(", "),
                      idValues.join(", "),
                      upsertPrefix);
    } else if (logEvt == SQLDelete) {
        QString idWhereCode;
        for (int i = 0 ; i < idFields.size() ; i++) {
            if (!idWhereCode.isEmpty()) {
                idWhereCode += " AND ";
            }
            idWhereCode += QString("%1=%2").arg(idFields[i], idValues[i]);
        }
        return QString(
                "    INSERT INTO %1 (%2, state) VALUES (%3, 'del') %4 state="
                    "(CASE %1.state WHEN 'ins' THEN 'ins' ELSE 'del' END);\n"
                "    DELETE FROM %1 WHERE %5 AND %1.state='ins';\n"
                ).arg(logTable,
                      idFields.join(", "),
                      idValues.join(", "),
                      upsertPrefix,
                      idWhereCode);
    }
    assert(false);
    return QString();
}

void SQLDatabaseWrapper::execAndCheckQuery(QSqlQuery& query, const QString& queryStr, const QString& errmsg)
{
    maybeLogSQLQuery(queryStr);
    if (!query.exec(queryStr)) {
        throw SQLException(query, !errmsg.isNull() ? errmsg.toUtf8().constData() : "Error executing query", __FILE__, __LINE__);
    }
}

void SQLDatabaseWrapper::execAndCheckPreparedQuery(QSqlQuery& query, const QString& errmsg)
{
    maybeLogSQLQuery(query.lastQuery());
    if (!query.exec()) {
        throw SQLException(query, !errmsg.isNull() ? errmsg.toUtf8().constData() : "Error executing query", __FILE__, __LINE__);
    }
}

void SQLDatabaseWrapper::execAndCheckQuery(const QString& queryStr, const QString& errmsg)
{
    QSqlQuery query(db);
    maybeLogSQLQuery(queryStr);
    if (!query.exec(queryStr)) {
        throw SQLException(query, !errmsg.isNull() ? errmsg.toUtf8().constData() : "Error executing query", __FILE__, __LINE__);
    }
}

void SQLDatabaseWrapper::execAndCheckQueries(QSqlQuery& query, const QStringList& queryStrs, const QString& errmsg)
{
    for (const QString& queryStr : queryStrs) {
        execAndCheckQuery(query, queryStr, errmsg);
    }
}

void SQLDatabaseWrapper::execAndCheckQueries(const QStringList& queryStrs, const QString& errmsg)
{
    QSqlQuery query(db);
    execAndCheckQueries(query, queryStrs, errmsg);
}

void SQLDatabaseWrapper::maybeLogSQLQuery(const QString& query)
{
    if (IsLogLevelActive(LOG_LEVEL_VERBOSE)) {
        LogVerbose("--- SQL BEGIN ---");

        LogMultiVerbose("%s", IndentString(query, "  ").toUtf8().constData());
    }
}

SQLTransaction SQLDatabaseWrapper::beginTransaction()
{
    return std::move(SQLTransaction(db).begin());
}

SQLTransaction SQLDatabaseWrapper::beginDDLTransaction()
{
    return supportsTransactionalDDL() ? beginTransaction() : SQLTransaction();
}

QMap<QString, QString> SQLDatabaseWrapper::queryDatabaseSettings()
{
    QMap<QString, QString> settings;

    QSqlQuery query(db);
    execAndCheckQuery(query, "SELECT * FROM electronicsdb_settings", "Error querying database settings");

    while (query.next()) {
        settings[query.value("keyid").toString()] = query.value("value").toString();
    }

    return settings;
}

void SQLDatabaseWrapper::writeDatabaseSettings(const QMap<QString, QString>& settings)
{
    for (auto it = settings.begin() ; it != settings.end() ; ++it) {
        const QString& key = it.key();
        const QString& value = it.value();

        execAndCheckQuery(QString(
                "INSERT INTO electronicsdb_settings (keyid, value) VALUES (%1, %2) "
                "%3 value=%2"
                ).arg(escapeString(key), escapeString(value), generateUpsertPrefixCode(QStringList{"keyid"})),
                "Error changing database settings");
    }
}

void SQLDatabaseWrapper::dropTableIfExists(const QString& tableName)
{
    execAndCheckQuery(QString("DROP TABLE IF EXISTS %1").arg(escapeIdentifier(tableName)),
                      QString("Error dropping table '%1'").arg(tableName));
}

void SQLDatabaseWrapper::dropTriggerIfExists(const QString& triggerName, const QString& tableName)
{
    execAndCheckQuery(generateDropTriggerIfExistsCode(triggerName, tableName),
                      QString("Error dropping trigger: '%1'").arg(triggerName));
}

QString SQLDatabaseWrapper::generateDoubleTypeCode()
{
    return "DOUBLE";
}

QString SQLDatabaseWrapper::generateUpsertPrefixCode(const QStringList& conflictTarget)
{
    // Unfortunately, a conflict target was mandatory until SQLite 3.35.0, and at least Qt 5.15.2 still ships an older
    // version of SQLite.
    QString targetStr;
    for (const QString& t : conflictTarget) {
        if (!targetStr.isEmpty()) {
            targetStr += ",";
        }
        targetStr += escapeIdentifier(t);
    }
    return QString("ON CONFLICT (%1) DO UPDATE SET").arg(targetStr);
}

QString SQLDatabaseWrapper::groupConcatCode(const QString& expr, const QString& sep)
{
    return QString("group_concat(%1,%2)").arg(expr, escapeString(sep));
}

QString SQLDatabaseWrapper::jsonArrayCode(const QStringList& elems)
{
    return QString("json_array(%1)").arg(elems.join(','));
}

QString SQLDatabaseWrapper::generateDropTriggerIfExistsCode(const QString& triggerName, const QString& tableName)
{
    return QString("DROP TRIGGER IF EXISTS %1").arg(escapeIdentifier(triggerName));
}

QList<QString> SQLDatabaseWrapper::listTables()
{
    return db.tables();
}

QString SQLDatabaseWrapper::escapeIdentifier(const QString& str)
{
    // In all QtSql drivers I've checked, the type parameter is actually ignored.
    return db.driver()->escapeIdentifier(str, QSqlDriver::FieldName);
}

QString SQLDatabaseWrapper::escapeString(const QString& str)
{
    checkDatabaseValid();

    // TODO: Is this REALLY safe? There doesn't seem to be a better way with QtSql...
    QSqlField field(QString(), QVariant::String);
    field.setValue(str);
    return db.driver()->formatValue(field);
}

void SQLDatabaseWrapper::insertMultipleWithReturnID (
        const QString& insStmt, const QString& idField, QList<dbid_t>& ids,
        const std::function<void (QSqlQuery&)>& postPrepare
) {
    checkDatabaseValid();

    QSqlQuery query(db);
    QString queryStr = QString("%1 RETURNING %2").arg(insStmt, escapeIdentifier(idField));
    if (postPrepare) {
        if (!query.prepare(queryStr)) {
            throw SQLException(query, "Error preparing multi-insert", __FILE__, __LINE__);
        }
        postPrepare(query);
        execAndCheckPreparedQuery(query, "Error executing prepared multi-insert");
    } else {
        execAndCheckQuery(query, queryStr, "Error executing multi-insert");
    }
    while (query.next()) {
        ids << VariantToDBID(query.value(0));
    }
}

void SQLDatabaseWrapper::truncateTable(const QString& tableName)
{
    QSqlQuery query(db);
    execAndCheckQuery(QString("TRUNCATE TABLE %1").arg(escapeIdentifier(tableName)),
                      QString("Error truncating table '%1'").arg(tableName));
}

void SQLDatabaseWrapper::checkDatabaseValid()
{
    if (!db.isValid()) {
        throw InvalidStateException("No database specified for SQLDatabaseWrapper", __FILE__, __LINE__);
    }
}

bool SQLDatabaseWrapper::getIntegerTypeRange(const QString& type, bool unsignedType, int64_t& min, int64_t& max)
{
    QString t = type.toLower();

    if (t == "tinyint") {
        if (unsignedType) {
            min = std::numeric_limits<uint8_t>::min();
            max = std::numeric_limits<uint8_t>::max();
        } else {
            min = std::numeric_limits<int8_t>::min();
            max = std::numeric_limits<int8_t>::max();
        }
    } else if (t == "smallint"  ||  t == "smallserial") {
        if (unsignedType) {
            min = std::numeric_limits<uint16_t>::min();
            max = std::numeric_limits<uint16_t>::max();
        } else {
            min = std::numeric_limits<int16_t>::min();
            max = std::numeric_limits<int16_t>::max();
        }
    } else if (t == "mediumint") {
        if (unsignedType) {
            min = 0;
            max = 16777215;
        } else {
            min = -8388608;
            max = 8388607;
        }
    } else if (t == "integer"  ||  t == "int"  ||  t == "serial") {
        if (unsignedType) {
            min = std::numeric_limits<uint32_t>::min();
            max = std::numeric_limits<uint32_t>::max();
        } else {
            min = std::numeric_limits<int32_t>::min();
            max = std::numeric_limits<int32_t>::max();
        }
    } else if (t == "bigint"  ||  t == "bigserial") {
        if (unsignedType) {
            // Higher values are possible, but not representable as int64_t
            min = 0;
            max = std::numeric_limits<int64_t>::max();
        } else {
            min = std::numeric_limits<int64_t>::min();
            max = std::numeric_limits<int64_t>::max();
        }
    } else {
        return false;
    }

    return true;
}

PartProperty::DefaultValueFunction SQLDatabaseWrapper::parsePartPropertyDefaultValueFunction (
        PartProperty* prop, const QVariant& sqlDefaultVal
) {
    QString normVal = sqlDefaultVal.toString().toLower().trimmed();
    PartProperty::Type type = prop->getType();

    if (type == PartProperty::Date  &&  normVal == "current_date") {
        return PartProperty::DefaultValueNow;
    } else if (type == PartProperty::Time  &&  normVal == "current_time") {
        return PartProperty::DefaultValueNow;
    } else if (type == PartProperty::DateTime  &&  normVal == "current_timestamp") {
        return PartProperty::DefaultValueNow;
    }

    return PartProperty::DefaultValueConst;
}


}
