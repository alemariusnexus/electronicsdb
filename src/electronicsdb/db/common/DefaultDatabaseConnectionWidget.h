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

#include <ui_DefaultDatabaseConnectionWidget.h>
#include "../DatabaseConnectionWidget.h"
#include "../SQLDatabaseWrapper.h"

namespace electronicsdb
{


class DefaultDatabaseConnectionWidget : public DatabaseConnectionWidget
{
    Q_OBJECT

public:
    DefaultDatabaseConnectionWidget(const SQLDatabaseWrapper::DatabaseType& dbType, QWidget* parent = nullptr);

    void setHost(const QString& host) { ui.hostEdit->setText(host); }
    void setPort(uint16_t port) { ui.portSpinner->setValue(port); }
    void setDatabase(const QString& dbName) { ui.dbEdit->setText(dbName); }
    void setUser(const QString& user) { ui.userEdit->setText(user); }

    QString getHost() const { return ui.hostEdit->text(); }
    uint16_t getPort() const { return static_cast<uint16_t>(ui.portSpinner->value()); }
    QString getDatabase() const { return ui.dbEdit->text(); }
    QString getUser() const { return ui.userEdit->text(); }

    void rememberDefaultValues();

    void display(DatabaseConnection* conn) override;
    void clear() override;

    DatabaseConnection* createConnection() override;

private slots:
    void pwVaultBoxStateChanged();

private:
    Ui_DefaultDatabaseConnectionWidget ui;

    SQLDatabaseWrapper::DatabaseType dbType;
    QString connID;

    QString hostDefault;
    uint16_t portDefault;
    QString dbDefault;
    QString userDefault;
    bool keepPwInVaultDefault;
};


}
