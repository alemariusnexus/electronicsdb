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

#include "ConnectDialog.h"

#include <cstdio>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include "../../db/DatabaseConnection.h"

namespace electronicsdb
{


ConnectDialog::ConnectDialog(QWidget* parent)
        : QDialog(parent)
{
    ui.setupUi(this);

    connect(ui.connectButton, &QPushButton::clicked, this, &ConnectDialog::connectRequested);
    connect(ui.cancelButton, &QPushButton::clicked, this, &ConnectDialog::reject);
}


void ConnectDialog::connectRequested()
{
    System* sys = System::getInstance();

    DatabaseConnection* conn = ui.connEditor->createConnection();

    ui.connectButton->setEnabled(false);
    ui.cancelButton->setEnabled(false);

    bool ok;
    QString pw = sys->askConnectDatabasePassword(conn, &ok);
    if (!ok) {
        delete conn;
        return;
    }

    if (sys->connectDatabase(conn, pw)) {
        sys->addPermanentDatabaseConnection(conn);
        sys->savePermanentDatabaseConnectionSettings();

        accept();
    } else {
        delete conn;

        ui.connectButton->setEnabled(true);
        ui.cancelButton->setEnabled(true);
    }
}


}
