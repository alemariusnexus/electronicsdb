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


class PSQLDatabaseWrapper : public SQLDatabaseWrapper
{
    Q_OBJECT

private:
    // Required for UPSERT
    static constexpr int psqlMinVersion = EDB_BUILD_VERSION(9, 5, 0);

public:
    PSQLDatabaseWrapper();
    PSQLDatabaseWrapper(const QSqlDatabase& db);

    DatabaseConnection* createConnection(DatabaseType dbType) override;
    DatabaseConnectionWidget* createWidget(DatabaseType dbType, QWidget* parent) override;

    bool supportsTransactionalDDL() override { return true; }
    bool supportsVirtualGeneratedColumns() const override { return false; }

    void dropDatabaseIfExists(DatabaseConnection* conn, const QString& pw) override;
    void createDatabaseIfNotExists(DatabaseConnection* conn, const QString& pw) override;

    QString groupConcatCode(const QString& expr, const QString& sep = ",") override;
    QString groupJsonArrayCode(const QString& expr) override;
    QString jsonArrayCode(const QStringList& elems) override;
    QString generateDropTriggerIfExistsCode(const QString& triggerName, const QString& tableName) override;

    void initSession() override;

    void createRowTrigger (
            const QString& triggerName,
            const QString& tableName,
            SQLEvent triggerEvt,
            const QString& body
            ) override;
    void adjustAutoIncrementColumn(const QString& tableName, const QString& colName) override;

    QList<DatabaseType> getSupportedDatabaseTypes() override;
    QString generateIntPrimaryKeyCode(const QString& fieldName) override;
    QString generateDoubleTypeCode() override;
    void createIndexIfNotExists(const QString& table, const QString& column) override;
    QList<QMap<QString, QVariant>> listTriggers() override;
    QList<QMap<QString, QVariant>> listColumns(const QString& table, bool fullMetadata) override;
    void insertMultipleDefaultValuesWithReturnID(const QString& tableName, const QString& idField, int count,
            QList<dbid_t>& ids) override;

    QString getDatabaseDescription() override;
    int getDatabaseLimit(LimitType type) override;
    QString getPartPropertySQLType(PartProperty* prop) override;

private:
    void execInTempConnection(DatabaseConnection* conn, const QString& pw, const std::function<void (QSqlDatabase&)>& func);
};


}
