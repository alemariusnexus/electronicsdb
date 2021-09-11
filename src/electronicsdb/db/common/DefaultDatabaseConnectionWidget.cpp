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

#include "DefaultDatabaseConnectionWidget.h"

#include <nxcommon/log.h>
#include "../../util/KeyVault.h"
#include "DefaultDatabaseConnection.h"

namespace electronicsdb
{


DefaultDatabaseConnectionWidget::DefaultDatabaseConnectionWidget (
        const SQLDatabaseWrapper::DatabaseType& dbType, QWidget* parent
)       : DatabaseConnectionWidget(parent), dbType(dbType), keepPwInVaultDefault(false)
{
    ui.setupUi(this);

    connect(ui.pwVaultBox, &QCheckBox::stateChanged, this, &DefaultDatabaseConnectionWidget::pwVaultBoxStateChanged);

    DefaultDatabaseConnection* conn = new DefaultDatabaseConnection(dbType);

    setHost(conn->getHost());
    setPort(conn->getPort());
    setDatabase(conn->getDatabaseName());
    setUser(conn->getUser());

    delete conn;
}

void DefaultDatabaseConnectionWidget::rememberDefaultValues()
{
    hostDefault = getHost();
    portDefault = getPort();
    dbDefault = getDatabase();
    userDefault = getUser();
    keepPwInVaultDefault = ui.pwVaultBox->isChecked();
}

void DefaultDatabaseConnectionWidget::display(DatabaseConnection* conn)
{
    DefaultDatabaseConnection* dconn = dynamic_cast<DefaultDatabaseConnection*>(conn);

    if (!dconn) {
        clear();
        return;
    }

    connID = conn->getID();

    setHost(dconn->getHost());
    setPort(dconn->getPort());
    setDatabase(dconn->getDatabaseName());
    setUser(dconn->getUser());

    ui.pwEdit->clear();

    if (dconn->isKeepPasswordInVault()) {
        KeyVault& vault = KeyVault::getInstance();

        if (vault.containsKey(conn->getPasswordVaultID())) {
            ui.pwVaultBox->setChecked(true);

            if (vault.containsKey(dconn->getPasswordVaultID())) {
                ui.pwEdit->setText("????????");
            }
        } else {
            ui.pwVaultBox->setChecked(false);
        }
    } else {
        ui.pwVaultBox->setChecked(false);
    }
}

void DefaultDatabaseConnectionWidget::clear()
{
    connID = QString();

    setHost(hostDefault);
    setPort(portDefault);
    setDatabase(dbDefault);
    setUser(userDefault);

    ui.pwEdit->clear();
    ui.pwVaultBox->setChecked(keepPwInVaultDefault);
}

DatabaseConnection* DefaultDatabaseConnectionWidget::createConnection()
{
    DefaultDatabaseConnection* conn = new DefaultDatabaseConnection(dbType, connID);

    conn->setHost(getHost());
    conn->setPort(getPort());
    conn->setDatabaseName(getDatabase());
    conn->setUser(getUser());

    if (ui.pwVaultBox->isChecked()) {
        conn->setKeepPasswordInVault(true);

        if (ui.pwEdit->text() != "????????") {
            KeyVault& vault = KeyVault::getInstance();

            vault.setKeyMaybeWithUserPrompt(conn->getPasswordVaultID(), ui.pwEdit->text(), this);
        }
    } else {
        conn->setKeepPasswordInVault(false);
    }

    return conn;
}

void DefaultDatabaseConnectionWidget::pwVaultBoxStateChanged()
{
    ui.pwEdit->setEnabled(ui.pwVaultBox->isChecked());
}


}
