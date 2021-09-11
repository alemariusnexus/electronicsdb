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

#include <functional>
#include "../SQLDatabaseWrapper.h"

namespace electronicsdb
{


class MySQLDatabaseWrapper : public SQLDatabaseWrapper
{
    Q_OBJECT

private:
    // Both are required for json_arrayagg().
    static constexpr int mysqlMinVersion = EDB_BUILD_VERSION(5, 7, 22);
    static constexpr int mariadbMinVersion = EDB_BUILD_VERSION(10, 5, 0);

public:
    MySQLDatabaseWrapper();
    MySQLDatabaseWrapper(const QSqlDatabase& db);

    DatabaseConnection* createConnection(DatabaseType dbType) override;
    DatabaseConnectionWidget* createWidget(DatabaseType dbType, QWidget* parent) override;

    void dropDatabaseIfExists(DatabaseConnection* conn, const QString& pw) override;
    void createDatabaseIfNotExists(DatabaseConnection* conn, const QString& pw) override;

    void initSession() override;

    QString generateUpsertPrefixCode(const QStringList& conflictTarget) override;
    QString groupConcatCode(const QString& expr, const QString& sep = ",") override;
    QString groupJsonArrayCode(const QString& expr) override;

    QList<DatabaseType> getSupportedDatabaseTypes() override;
    QString generateIntPrimaryKeyCode(const QString& fieldName) override;
    void createIndexIfNotExists(const QString& table, const QString& column) override;

    QList<QMap<QString, QVariant>> listTriggers() override;
    QList<QMap<QString, QVariant>> listColumns(const QString& table, bool fullMetadata) override;
    void insertMultipleWithReturnID(const QString& insStmt, const QString& idField, QList<dbid_t>& ids,
            const std::function<void (QSqlQuery&)>& postPrepare = {}) override;
    void insertMultipleDefaultValuesWithReturnID(const QString& tableName, const QString& idField, int count,
            QList<dbid_t>& ids) override;

    QString getDatabaseDescription() override;
    int getDatabaseLimit(LimitType type) override;
    QString getPartPropertySQLType(PartProperty* prop) override;
    PartProperty::DefaultValueFunction parsePartPropertyDefaultValueFunction(PartProperty* prop,
            const QVariant& sqlDefaultVal) override;

private:
    void execInTempConnection(DatabaseConnection* conn, const QString& pw, const std::function<void (QSqlDatabase&)>& func);

private:

};


}
