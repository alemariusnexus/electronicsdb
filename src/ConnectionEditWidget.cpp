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

#include "ConnectionEditWidget.h"
#include <QtGui/QFileDialog>



ConnectionEditWidget::ConnectionEditWidget(QWidget* parent)
		: QWidget(parent)
{
	ui.setupUi(this);

	ui.connTypeBox->addItem(tr("SQLite"), DatabaseConnection::SQLite);

#ifdef EDB_MYSQL_ENABLED
	ui.connTypeBox->addItem(tr("MySQL"), DatabaseConnection::MySQL);
#endif

	connect(ui.connTypeBox, SIGNAL(activated(int)), this, SLOT(connTypeBoxActivated(int)));
	connect(ui.connSqliteChooseButton, SIGNAL(clicked(bool)), this, SLOT(connSqliteFileChooseButtonClicked()));
	connect(ui.connFileRootChooseButton, SIGNAL(clicked(bool)), this, SLOT(connFileRootPathChooseButtonClicked()));

	connect(ui.connNameField, SIGNAL(textEdited(const QString&)), this, SIGNAL(connectionNameChanged(const QString&)));

	setConnectionType(DatabaseConnection::SQLite);
}


ConnectionEditWidget::~ConnectionEditWidget()
{
}


void ConnectionEditWidget::setConnectionName(const QString& name)
{
	ui.connNameField->setText(name);
}


void ConnectionEditWidget::setConnectionType(DatabaseConnection::Type type)
{
	int index = ui.connTypeBox->findData(type);
	ui.connTypeBox->setCurrentIndex(index);

	if (type == DatabaseConnection::MySQL) {
		ui.connStackWidget->setCurrentIndex(0);
	} else {
		ui.connStackWidget->setCurrentIndex(1);
	}
}


void ConnectionEditWidget::setMySQLHost(const QString& host)
{
	ui.connServerField->setText(host);
}


void ConnectionEditWidget::setMySQLPort(unsigned int port)
{
	ui.connPortField->setValue(port);
}


void ConnectionEditWidget::setMySQLDatabaseName(const QString& db)
{
	ui.connDbField->setText(db);
}


void ConnectionEditWidget::setMySQLUser(const QString& user)
{
	ui.connUserField->setText(user);
}


void ConnectionEditWidget::setSQLiteFilePath(const QString& path)
{
	ui.connSqliteFileField->setText(path);
}


void ConnectionEditWidget::setFileRootPath(const QString& path)
{
	ui.connFileRootField->setText(path);
}


void ConnectionEditWidget::connTypeBoxActivated(int idx)
{
	if (idx == -1)
		return;

	unsigned int dbType = ui.connTypeBox->itemData(idx, Qt::UserRole).toUInt();

	if (dbType == DatabaseConnection::MySQL) {
		ui.connStackWidget->setCurrentIndex(0);
	} else {
		ui.connStackWidget->setCurrentIndex(1);
	}
}


void ConnectionEditWidget::connSqliteFileChooseButtonClicked()
{
	QString fpath = QFileDialog::getOpenFileName(this, tr("Choose a database file"), ui.connSqliteFileField->text(),
			tr("SQLite Databases (*.db *.sqlite)"));

	if (fpath.isNull())
		return;

	setSQLiteFilePath(fpath);
}


void ConnectionEditWidget::connFileRootPathChooseButtonClicked()
{
	QString fpath = QFileDialog::getExistingDirectory(this, tr("Choose a root directory"), ui.connFileRootField->text());

	if (fpath.isNull())
		return;

	setFileRootPath(fpath);
}


void ConnectionEditWidget::changeEvent(QEvent* evt)
{
	if (evt->type() == QEvent::EnabledChange) {
		bool enabled = isEnabled();

		ui.connNameField->setEnabled(enabled);
		ui.connTypeBox->setEnabled(enabled);

		ui.connServerField->setEnabled(enabled);
		ui.connPortField->setEnabled(enabled);
		ui.connDbField->setEnabled(enabled);
		ui.connUserField->setEnabled(enabled);

		ui.connSqliteFileField->setEnabled(enabled);
		ui.connSqliteChooseButton->setEnabled(enabled);

		ui.connFileRootField->setEnabled(enabled);
		ui.connFileRootChooseButton->setEnabled(enabled);
	}
}


void ConnectionEditWidget::clear()
{
	setConnectionName("");
	setMySQLHost("");
	setMySQLPort(0);
	setMySQLUser("");
	setMySQLDatabaseName("");
	setSQLiteFilePath("");
	setFileRootPath("");
}


