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
#include <cstdio>


ConnectDialog::ConnectDialog(QWidget* parent)
		: QDialog(parent)
{
	ui.setupUi(this);

	ui.typeBox->addItem(tr("SQLite"), "sqlite");

#ifdef EDB_MYSQL_ENABLED
	ui.typeBox->addItem(tr("MySQL"), "mysql");
#endif

	connect(ui.connectButton, SIGNAL(clicked()), this, SLOT(connectRequested()));
	connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(ui.profileBox, SIGNAL(toggled(bool)), ui.nameField, SLOT(setEnabled(bool)));
	connect(ui.typeBox, SIGNAL(activated(int)), this, SLOT(typeBoxActivated(int)));
	connect(ui.sqliteChooseButton, SIGNAL(clicked()), this, SLOT(sqliteFileChosen()));
}


void ConnectDialog::connectRequested()
{
	System* sys = System::getInstance();

	QString typeStr = ui.typeBox->itemData(ui.typeBox->currentIndex(), Qt::UserRole).toString();

	DatabaseConnection* conn;

	if (typeStr == "mysql") {
		conn = new DatabaseConnection(DatabaseConnection::MySQL);

		conn->setMySQLHost(ui.serverField->text());
		conn->setMySQLPort(ui.portBox->value());
		conn->setMySQLUser(ui.userField->text());
		conn->setMySQLDatabaseName(ui.dbField->text());
	} else {
		conn = new DatabaseConnection(DatabaseConnection::SQLite);

		conn->setSQLiteFilePath(ui.sqliteFileField->text());
	}

	if (ui.profileBox->isChecked()) {
		conn->setName(ui.nameField->text());
	}

	ui.connectButton->setEnabled(false);
	ui.cancelButton->setEnabled(false);

	if (sys->connectDatabase(conn, ui.pwField->text())) {
		if (ui.profileBox->isChecked()) {
			sys->addPermanentDatabaseConnection(conn);
			sys->savePermanentDatabaseConnectionSettings();
		}

		accept();
	} else {
		delete conn;

		ui.connectButton->setEnabled(true);
		ui.cancelButton->setEnabled(true);
	}
}


void ConnectDialog::typeBoxActivated(int idx)
{
	QString typeStr = ui.typeBox->itemData(idx, Qt::UserRole).toString();

	if (typeStr == "mysql") {
		ui.stackedWidget->setCurrentIndex(0);
	} else {
		ui.stackedWidget->setCurrentIndex(1);
	}
}


void ConnectDialog::sqliteFileChosen()
{
	QString fpath = QFileDialog::getOpenFileName(this, tr("Choose a database file"), ui.sqliteFileField->text(),
			tr("SQLite Databases (*.db)"));

	if (fpath.isNull())
		return;

	ui.sqliteFileField->setText(fpath);
}
