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

#include "../SQLDatabaseWrapper.h"

namespace electronicsdb
{


class SQLiteDatabaseWrapper : public SQLDatabaseWrapper
{
    Q_OBJECT

private:
    // 3.24 required for basic UPSERT
    // 3.32 required for higher limit on bound parameters
    // 3.35 required for ALTER TABLE DROP COLUMN
    static constexpr int sqliteMinVersion = EDB_BUILD_VERSION(3, 35, 0);

public:
    SQLiteDatabaseWrapper();
    SQLiteDatabaseWrapper(const QSqlDatabase& db);

    DatabaseConnection* createConnection(DatabaseType dbType) override;
    DatabaseConnectionWidget* createWidget(DatabaseType dbType, QWidget* parent) override;

    bool supportsTransactionalDDL() override { return true; }
    bool supportsExecWithMultipleStatements() override { return false; }

    void dropDatabaseIfExists(DatabaseConnection* conn, const QString& pw) override;
    void createDatabaseIfNotExists(DatabaseConnection* conn, const QString& pw) override;

    QString groupJsonArrayCode(const QString& expr) override;

    void initSession() override;

    QList<DatabaseType> getSupportedDatabaseTypes() override;
    QString generateIntPrimaryKeyCode(const QString& fieldName) override;
    void createIndexIfNotExists(const QString& table, const QString& column) override;
    QList<QMap<QString, QVariant>> listTriggers() override;
    QList<QMap<QString, QVariant>> listColumns(const QString& table, bool fullMetadata) override;
    void insertMultipleWithReturnID(const QString& insStmt, const QString& idField, QList<dbid_t>& ids,
            const std::function<void (QSqlQuery&)>& postPrepare = {}) override;
    void insertMultipleDefaultValuesWithReturnID(const QString& tableName, const QString& idField, int count,
            QList<dbid_t>& ids) override;
    void truncateTable(const QString& tableName) override;

    QString getDatabaseDescription() override;
    int getDatabaseLimit(LimitType type) override;
    QString getPartPropertySQLType(PartProperty* prop) override;
};


}
