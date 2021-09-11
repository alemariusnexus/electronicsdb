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

#include <ui_SQLiteDatabaseConnectionWidget.h>
#include "../DatabaseConnectionWidget.h"

namespace electronicsdb
{


class SQLiteDatabaseConnectionWidget : public DatabaseConnectionWidget
{
    Q_OBJECT

public:
    SQLiteDatabaseConnectionWidget(const SQLDatabaseWrapper::DatabaseType& dbType, QWidget* parent = nullptr);

    void setDatabaseFile(const QString& dbFile) { ui.dbFileEdit->setText(dbFile); }
    QString getDatabaseFile() const { return ui.dbFileEdit->text(); }

    void display(DatabaseConnection* conn) override;
    void clear() override;

    DatabaseConnection* createConnection() override;

private slots:
    void onDbFileChoose();

private:
    Ui_SQLiteDatabaseConnectionWidget ui;

    SQLDatabaseWrapper::DatabaseType dbType;
    QString connID;
};


}
