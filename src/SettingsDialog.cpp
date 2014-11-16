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

#include "SettingsDialog.h"
#include "System.h"
#include "PartCategory.h"
#include <QtCore/QSettings>
#include <QtGui/QFileDialog>
#include <cstdio>


SettingsDialog::SettingsDialog(QWidget* parent)
		: QDialog(parent), curEdConn(NULL)
{
	ui.setupUi(this);

	System* sys = System::getInstance();

	System::DatabaseConnectionIterator it;
	for (it = sys->getPermanentDatabaseConnectionsBegin() ; it != sys->getPermanentDatabaseConnectionsEnd() ; it++) {
		DatabaseConnection* conn = *it;

		LocalPermDbConn* lconn = new LocalPermDbConn;
		lconn->globalConn = conn;
		lconn->localConn = new DatabaseConnection(*conn);

		localPermDbConns.push_back(lconn);

		ui.connList->addItem(conn->getName());
	}


	connect(ui.connList, SIGNAL(currentRowChanged(int)), this, SLOT(connListRowChanged(int)));
	connect(ui.connAddButton, SIGNAL(clicked()), this, SLOT(connAddRequested()));
	connect(ui.connRemoveButton, SIGNAL(clicked()), this, SLOT(connRemoveRequested()));

	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));

	connect(ui.connEditor, SIGNAL(connectionNameChanged(const QString&)), this, SLOT(connNameChanged(const QString&)));

	ui.connAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
	ui.connRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));

	if (localPermDbConns.empty()) {
		ui.connEditor->setEnabled(false);
	} else {
		ui.connList->setCurrentRow(0);
	}

	rebuildStartupConnectionBox();


	QSettings s;

	int startupDbIdx = s.value("main/startup_db", -1).toInt();

	if (startupDbIdx >= 0  &&  startupDbIdx < sys->getPermanentDatabaseConnectionSize()) {
		DatabaseConnection* conn = sys->getPermanentDatabaseConnection(startupDbIdx);

		unsigned int i = 0;
		for (LocalPermDbConnList::iterator it = localPermDbConns.begin() ; it != localPermDbConns.end() ; it++, i++) {
			LocalPermDbConn* lconn = *it;

			if (lconn->globalConn == conn) {
				ui.connStartupBox->setCurrentIndex(i+1);
				break;
			}
		}
	}

	ui.siBinPrefixDefaultBox->setChecked(s.value("main/si_binary_prefixes_default", true).toBool());
}


SettingsDialog::~SettingsDialog()
{
	for (LocalPermDbConnList::iterator it = localPermDbConns.begin() ; it != localPermDbConns.end() ; it++) {
		delete (*it)->localConn;
		delete *it;
	}
}


void SettingsDialog::applyCurrentConnectionChanges()
{
	if (curEdConn) {
		//QString dbType = ui.connTypeBox->itemData(ui.connTypeBox->currentIndex(), Qt::UserRole).toString();
		DatabaseConnection::Type dbType = ui.connEditor->getConnectionType();

		if (dbType == DatabaseConnection::MySQL) {
			curEdConn->localConn->setType(DatabaseConnection::MySQL);
			curEdConn->localConn->setMySQLHost(ui.connEditor->getMySQLHost());
			curEdConn->localConn->setMySQLUser(ui.connEditor->getMySQLUser());
			curEdConn->localConn->setMySQLPort(ui.connEditor->getMySQLPort());
			curEdConn->localConn->setMySQLDatabaseName(ui.connEditor->getMySQLDatabaseName());
		} else {
			curEdConn->localConn->setSQLiteFilePath(ui.connEditor->getSQLiteFilePath());
		}

		curEdConn->localConn->setName(ui.connEditor->getConnectionName());

		curEdConn->localConn->setFileRoot(ui.connEditor->getFileRootPath());

		ui.connList->item(localPermDbConns.indexOf(curEdConn))->setText(curEdConn->localConn->getName());

		rebuildStartupConnectionBox();
	}
}


void SettingsDialog::connListRowChanged(int row)
{
	System* sys = System::getInstance();

	if (curEdConn) {
		applyCurrentConnectionChanges();
	}

	if (row != -1  &&  row < localPermDbConns.size()) {
		LocalPermDbConn* lconn = localPermDbConns[row];
		DatabaseConnection* conn = lconn->localConn;

		curEdConn = lconn;

		ui.connEditor->setConnectionName(conn->getName());
		ui.connEditor->setConnectionType(conn->getType());

		ui.connEditor->setFileRootPath(conn->getFileRoot());

		ui.connEditor->setEnabled(true);

		if (conn->getType() == DatabaseConnection::MySQL) {

			ui.connEditor->setMySQLHost(conn->getMySQLHost());
			ui.connEditor->setMySQLPort(conn->getMySQLPort());
			ui.connEditor->setMySQLUser(conn->getMySQLUser());
			ui.connEditor->setMySQLDatabaseName(conn->getMySQLDatabaseName());
		} else {
			ui.connEditor->setSQLiteFilePath(conn->getSQLiteFilePath());
		}

		ui.connRemoveButton->setEnabled(true);
	} else {
		curEdConn = NULL;

		ui.connEditor->clear();
		ui.connEditor->setEnabled(false);

		ui.connRemoveButton->setEnabled(false);
	}
}


void SettingsDialog::connAddRequested()
{
	applyCurrentConnectionChanges();

	curEdConn = NULL;

	DatabaseConnection* conn = new DatabaseConnection(DatabaseConnection::SQLite);
	conn->setName(tr("New Connection"));

	ui.connEditor->clear();

	ui.connEditor->setMySQLHost("localhost");
	ui.connEditor->setMySQLPort(3306);
	ui.connEditor->setMySQLDatabaseName("electronics");
	ui.connEditor->setMySQLUser("electronics");

	LocalPermDbConn* lconn = new LocalPermDbConn;
	lconn->globalConn = NULL;
	lconn->localConn = conn;

	localPermDbConns.push_back(lconn);

	ui.connList->addItem(conn->getName());
	ui.connList->setCurrentRow(ui.connList->count()-1);

	rebuildStartupConnectionBox();
}


void SettingsDialog::connRemoveRequested()
{
	if (curEdConn) {
		LocalPermDbConn* lconn = curEdConn;

		curEdConn = NULL;

		int crow = ui.connList->currentRow();

		if (crow == ui.connList->count()-1) {
			ui.connList->setCurrentRow(crow-1);
		} else {
			ui.connList->setCurrentRow(crow+1);
		}

		localPermDbConns.removeOne(lconn);

		delete lconn->localConn;
		delete lconn;

		ui.connList->takeItem(crow);

		rebuildStartupConnectionBox();
	}
}


void SettingsDialog::buttonBoxClicked(QAbstractButton* b)
{
	if (ui.buttonBox->standardButton(b) == QDialogButtonBox::Apply) {
		apply();
	} else {
		reject();
	}
}


void SettingsDialog::apply()
{
	QSettings s;

	System* sys = System::getInstance();

	applyCurrentConnectionChanges();


	QVariant selData = ui.connStartupBox->itemData(ui.connStartupBox->currentIndex(), Qt::UserRole);
	LocalPermDbConn* selConn = NULL;

	if (selData.isValid()) {
		selConn = *((LocalPermDbConn**) selData.toByteArray().constData());
	} else {
		sys->setStartupDatabaseConnection(NULL);
	}


	LocalPermDbConnList conns = localPermDbConns;

	System::DatabaseConnectionIterator it;
	LocalPermDbConnList::iterator lit;

	System::DatabaseConnectionList removedConns;

	for (it = sys->getPermanentDatabaseConnectionsBegin() ; it != sys->getPermanentDatabaseConnectionsEnd() ; it++) {
		DatabaseConnection* conn = *it;

		bool found = false;

		for (lit = conns.begin() ; lit != conns.end() ; lit++) {
			LocalPermDbConn* lconn = *lit;

			if (lconn->globalConn == conn) {
				// Connection changed (or still the same)

				conn->setFrom(lconn->localConn);

				if (selConn == lconn)
					sys->setStartupDatabaseConnection(conn);

				conns.erase(lit);
				found = true;

				break;
			}
		}

		if (!found) {
			// Connection removed

			removedConns << conn;
		}
	}

	for (it = removedConns.begin() ; it != removedConns.end() ; it++) {
		DatabaseConnection* conn = *it;

		sys->removePermanentDatabaseConnection(conn);

		if (conn != sys->getCurrentDatabaseConnection())
			delete conn;
	}

	for (lit = conns.begin() ; lit != conns.end() ; lit++) {
		LocalPermDbConn* lconn = *lit;

		// Connection added
		DatabaseConnection* newConn = new DatabaseConnection(*lconn->localConn);

		sys->addPermanentDatabaseConnection(newConn);

		if (selConn == lconn)
			sys->setStartupDatabaseConnection(newConn);
	}

	sys->savePermanentDatabaseConnectionSettings();

	bool siBinPrefixes = ui.siBinPrefixDefaultBox->isChecked();
	s.setValue("main/si_binary_prefixes_default", ui.siBinPrefixDefaultBox->isChecked());

	for (System::PartCategoryIterator it = sys->getPartCategoryBegin() ; it != sys->getPartCategoryEnd() ; it++) {
		PartCategory* cat = *it;

		for (PartCategory::PropertyIterator pit = cat->getPropertyBegin() ; pit != cat->getPropertyEnd() ; pit++) {
			PartProperty* prop = *pit;

			if ((prop->getFlags() & PartProperty::DisplayUnitPrefixSIBase2)  !=  0) {
				if (siBinPrefixes) {
					prop->setFlags(prop->getFlags() | PartProperty::SIPrefixesDefaultToBase2);
				} else {
					prop->setFlags(prop->getFlags() & ~PartProperty::SIPrefixesDefaultToBase2);
				}
			}
		}
	}

	accept();
}


void SettingsDialog::rebuildStartupConnectionBox()
{
	QVariant selData = ui.connStartupBox->itemData(ui.connStartupBox->currentIndex(), Qt::UserRole);
	LocalPermDbConn* selConn = NULL;

	if (selData.isValid()) {
		selConn = *((LocalPermDbConn**) selData.toByteArray().constData());
	}

	ui.connStartupBox->clear();

	ui.connStartupBox->addItem(tr("(None)"), QVariant());

	unsigned int selIdx = 0;
	for (LocalPermDbConnList::iterator it = localPermDbConns.begin() ; it != localPermDbConns.end() ; it++) {
		LocalPermDbConn* lconn = *it;

		QByteArray sPtr((char*) &lconn, sizeof(LocalPermDbConn*));
		ui.connStartupBox->addItem(lconn->localConn->getName(), sPtr);

		if (lconn == selConn) {
			selIdx = ui.connStartupBox->count()-1;
		}
	}

	ui.connStartupBox->setCurrentIndex(selIdx);
}


void SettingsDialog::connNameChanged(const QString& name)
{
	ui.connList->currentItem()->setText(name);
}


