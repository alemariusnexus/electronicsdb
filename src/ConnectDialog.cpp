/*
	Copyright 2010-2014 David "Alemarius Nexus" Lerch

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
#include "DatabaseConnection.h"
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <cstdio>


ConnectDialog::ConnectDialog(QWidget* parent)
		: QDialog(parent)
{
	ui.setupUi(this);

	connect(ui.connectButton, SIGNAL(clicked()), this, SLOT(connectRequested()));
	connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}


void ConnectDialog::connectRequested()
{
	System* sys = System::getInstance();

	//QString typeStr = ui.typeBox->itemData(ui.typeBox->currentIndex(), Qt::UserRole).toString();
	DatabaseConnection::Type type = ui.connEditor->getConnectionType();

	DatabaseConnection* conn;

	conn = new DatabaseConnection(type);

	if (type == DatabaseConnection::MySQL) {
		conn->setMySQLHost(ui.connEditor->getMySQLHost());
		conn->setMySQLPort(ui.connEditor->getMySQLPort());
		conn->setMySQLUser(ui.connEditor->getMySQLUser());
		conn->setMySQLDatabaseName(ui.connEditor->getMySQLDatabaseName());
	} else {
		conn->setSQLiteFilePath(ui.connEditor->getSQLiteFilePath());
	}

	conn->setName(ui.connEditor->getConnectionName());
	conn->setFileRoot(ui.connEditor->getFileRootPath());

	ui.connectButton->setEnabled(false);
	ui.cancelButton->setEnabled(false);

	QString pw;

	if (type == DatabaseConnection::MySQL) {
		bool ok;

		pw = QInputDialog::getText(this, tr("Enter Password"),
					QString(tr("Enter password for connection '%1':")).arg(conn->getName()),
					QLineEdit::Password, QString(), &ok);

		if (!ok) {
			delete conn;
			return;
		}
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

