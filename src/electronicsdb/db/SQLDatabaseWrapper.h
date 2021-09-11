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

#include "../global.h"

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include "../model/PartProperty.h"
#include "SQLTransaction.h"

namespace electronicsdb
{

class DatabaseConnection;
class DatabaseConnectionWidget;


class SQLDatabaseWrapper : public QObject
{
    Q_OBJECT

public:
    struct DatabaseType
    {
        QString driverName;
        QString userReadableName;
    };

    enum SQLEvent
    {
        SQLInsert,
        SQLUpdate,
        SQLDelete
    };

    enum LimitType
    {
        LimitMaxBindParams,
        LimitMaxQueryLength
    };

public:
    static constexpr int32_t programDbVersion = 1;

public:
    SQLDatabaseWrapper();
    SQLDatabaseWrapper(const QSqlDatabase& db);

    const QSqlDatabase& getDatabase() const;
    void setDatabase(const QSqlDatabase& db);

    virtual void dropDatabaseIfExists(DatabaseConnection* conn, const QString& pw) = 0;
    virtual void createDatabaseIfNotExists(DatabaseConnection* conn, const QString& pw) = 0;

    virtual void initSession();
    void createSchema();
    void createLogSchema();
    void createAppModelDependentSchema();
    virtual QString generateMetaTypeFieldDefCode();

    QMap<QString, QString> queryDatabaseSettings();
    void writeDatabaseSettings(const QMap<QString, QString>& settings);

    virtual QList<DatabaseType> getSupportedDatabaseTypes() = 0;

    virtual DatabaseConnection* createConnection(DatabaseType dbType) = 0;
    virtual DatabaseConnectionWidget* createWidget(DatabaseType dbType, QWidget* parent = nullptr) = 0;

    virtual bool supportsTransactionalDDL() { return false; }
    virtual bool supportsExecWithMultipleStatements() { return true; }
    virtual bool supportsVirtualGeneratedColumns() const { return true; }

    virtual QString generateIntPrimaryKeyCode(const QString& fieldName) = 0;
    virtual QString generateDoubleTypeCode();
    virtual QString generateUpsertPrefixCode(const QStringList& conflictTarget);
    virtual QString groupConcatCode(const QString& expr, const QString& sep = ",");
    virtual QString groupJsonArrayCode(const QString& expr) = 0;
    virtual QString jsonArrayCode(const QStringList& elems);
    virtual QString generateDropTriggerIfExistsCode(const QString& triggerName, const QString& tableName);
    virtual QString beginDDLTransactionCode() { return "BEGIN;"; }
    virtual QString commitDDLTransactionCode() { return "COMMIT;"; }
    virtual QString rollbackDDLTransactionCode() { return "ROLLBACK;"; }

    virtual void createIndexIfNotExists(const QString& table, const QString& column) = 0;
    virtual void dropTableIfExists(const QString& tableName);
    virtual void dropTriggerIfExists(const QString& triggerName, const QString& tableName);
    virtual void createRowTrigger (
            const QString& triggerName,
            const QString& tableName,
            SQLEvent triggerEvt,
            const QString& body
            );
    void createLogTrigger (
            const QString& tableName,
            SQLEvent triggerEvt,
            const QString& body
            );

    virtual void execAndCheckQuery(QSqlQuery& query, const QString& queryStr, const QString& errmsg = QString());
    virtual void execAndCheckPreparedQuery(QSqlQuery& query, const QString& errmsg = QString());
    virtual void execAndCheckQuery(const QString& queryStr, const QString& errmsg = QString());
    virtual void execAndCheckQueries(QSqlQuery& query, const QStringList& queryStrs, const QString& errmsg = QString());
    virtual void execAndCheckQueries(const QStringList& queryStrs, const QString& errmsg = QString());
    SQLTransaction beginTransaction();
    SQLTransaction beginDDLTransaction();

    virtual QList<QString> listTables();
    virtual QList<QMap<QString, QVariant>> listTriggers() = 0;
    virtual QList<QMap<QString, QVariant>> listColumns(const QString& table, bool fullMetadata = false) = 0;
    virtual QString escapeIdentifier(const QString& str);
    virtual QString escapeString(const QString& str);
    virtual void insertMultipleWithReturnID(const QString& insStmt, const QString& idField, QList<dbid_t>& ids,
            const std::function<void (QSqlQuery&)>& postPrepare = {});
    virtual void insertMultipleDefaultValuesWithReturnID(const QString& tableName, const QString& idField, int count,
            QList<dbid_t>& ids) = 0;
    virtual void truncateTable(const QString& tableName);

    virtual QString getDatabaseDescription() = 0;
    virtual int getDatabaseLimit(LimitType type) = 0;
    virtual QString getPartPropertySQLType(PartProperty* prop) = 0;
    virtual PartProperty::DefaultValueFunction parsePartPropertyDefaultValueFunction(
            PartProperty* prop, const QVariant& sqlDefaultVal);

protected:
    void checkDatabaseValid();
    bool getIntegerTypeRange(const QString& type, bool unsignedType, int64_t& min, int64_t& max);

    QString generateLogStateChangeCode (
            SQLEvent triggerEvt,
            SQLEvent logEvt,
            const QString& logTable,
            const QStringList& idFields,
            const QStringList& idValues
            );

private:
    void maybeLogSQLQuery(const QString& query);

protected:
    QSqlDatabase db;
};


}
