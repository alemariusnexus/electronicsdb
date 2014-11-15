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

	ui.connTypeBox->addItem(tr("SQLite"), "sqlite");

#ifdef EDB_MYSQL_ENABLED
	ui.connTypeBox->addItem(tr("MySQL"), "mysql");
#endif


	connect(ui.connList, SIGNAL(currentRowChanged(int)), this, SLOT(connListRowChanged(int)));
	connect(ui.connAddButton, SIGNAL(clicked()), this, SLOT(connAddRequested()));
	connect(ui.connRemoveButton, SIGNAL(clicked()), this, SLOT(connRemoveRequested()));

	connect(ui.connTypeBox, SIGNAL(activated(int)), this, SLOT(connTypeBoxActivated(int)));
	connect(ui.connSqliteChooseButton, SIGNAL(clicked(bool)), this, SLOT(connSqliteFileChooseButtonClicked()));

	connect(ui.connFileRootChooseButton, SIGNAL(clicked()), this, SLOT(connFileRootChosen()));

	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));

	if (localPermDbConns.empty()) {
		ui.connNameField->setEnabled(false);
		ui.connTypeBox->setEnabled(false);

		ui.connServerField->setEnabled(false);
		ui.connPortField->setEnabled(false);
		ui.connUserField->setEnabled(false);
		ui.connDbField->setEnabled(false);

		ui.connSqliteFileField->setEnabled(false);
		ui.connSqliteChooseButton->setEnabled(false);
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
		QString dbType = ui.connTypeBox->itemData(ui.connTypeBox->currentIndex(), Qt::UserRole).toString();

		if (dbType == "mysql") {
			curEdConn->localConn->setType(DatabaseConnection::MySQL);
			curEdConn->localConn->setMySQLHost(ui.connServerField->text());
			curEdConn->localConn->setMySQLUser(ui.connUserField->text());
			curEdConn->localConn->setMySQLPort(ui.connPortField->value());
			curEdConn->localConn->setMySQLDatabaseName(ui.connDbField->text());
		} else {
			curEdConn->localConn->setSQLiteFilePath(ui.connSqliteFileField->text());
		}

		curEdConn->localConn->setName(ui.connNameField->text());

		curEdConn->localConn->setFileRoot(ui.connFileRootField->text());

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

		ui.connNameField->setText(conn->getName());

		ui.connFileRootField->setText(conn->getFileRoot());

		ui.connNameField->setEnabled(true);
		ui.connTypeBox->setEnabled(true);

		ui.connServerField->setEnabled(true);
		ui.connUserField->setEnabled(true);
		ui.connPortField->setEnabled(true);
		ui.connDbField->setEnabled(true);

		ui.connSqliteFileField->setEnabled(true);
		ui.connSqliteChooseButton->setEnabled(true);

		ui.connFileRootField->setEnabled(true);
		ui.connFileRootChooseButton->setEnabled(true);

		QString typeStr;

		if (conn->getType() == DatabaseConnection::MySQL) {
			typeStr = "mysql";

			ui.connServerField->setText(conn->getMySQLHost());
			ui.connPortField->setValue(conn->getMySQLPort());
			ui.connUserField->setText(conn->getMySQLUser());
			ui.connDbField->setText(conn->getMySQLDatabaseName());

			ui.connStackWidget->setCurrentIndex(0);
		} else {
			typeStr = "sqlite";

			ui.connSqliteFileField->setText(conn->getSQLiteFilePath());

			ui.connStackWidget->setCurrentIndex(1);
		}

		for (unsigned int i = 0 ; i < ui.connTypeBox->count() ; i++) {
			if (ui.connTypeBox->itemData(i, Qt::UserRole).toString() == typeStr) {
				ui.connTypeBox->setCurrentIndex(i);
				break;
			}
		}

		ui.connRemoveButton->setEnabled(true);
	} else {
		curEdConn = NULL;

		ui.connNameField->setText("");
		ui.connServerField->setText("");
		ui.connDbField->setText("");
		ui.connUserField->setText("");
		ui.connPortField->setValue(0);

		ui.connSqliteFileField->clear();

		ui.connFileRootField->clear();

		ui.connNameField->setEnabled(false);
		ui.connTypeBox->setEnabled(false);

		ui.connServerField->setEnabled(false);
		ui.connUserField->setEnabled(false);
		ui.connDbField->setEnabled(false);
		ui.connPortField->setEnabled(false);

		ui.connSqliteFileField->setEnabled(false);
		ui.connSqliteChooseButton->setEnabled(false);

		ui.connFileRootField->setEnabled(false);
		ui.connFileRootChooseButton->setEnabled(false);

		ui.connRemoveButton->setEnabled(false);
	}
}


void SettingsDialog::connAddRequested()
{
	applyCurrentConnectionChanges();

	curEdConn = NULL;

	DatabaseConnection* conn = new DatabaseConnection(DatabaseConnection::SQLite);
	conn->setName(tr("New Connection"));

	ui.connServerField->setText("localhost");
	ui.connPortField->setValue(3306);
	ui.connDbField->setText("electronics");
	ui.connUserField->setText("electronics");

	ui.connSqliteFileField->clear();

	ui.connFileRootField->clear();

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


void SettingsDialog::connTypeBoxActivated(int idx)
{
	if (idx == -1)
		return;

	QString dbType = ui.connTypeBox->itemData(idx, Qt::UserRole).toString();

	if (dbType == "mysql") {
		ui.connStackWidget->setCurrentIndex(0);
	} else {
		ui.connStackWidget->setCurrentIndex(1);
	}
}


void SettingsDialog::connSqliteFileChooseButtonClicked()
{
	QString fpath = QFileDialog::getOpenFileName(this, tr("Choose a database file"), ui.connSqliteFileField->text(),
			tr("SQLite Databases (*.db)"));

	if (fpath.isNull())
		return;

	ui.connSqliteFileField->setText(fpath);
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


void SettingsDialog::connFileRootChosen()
{
	QString fpath = QFileDialog::getExistingDirectory(this, tr("Choose the root path"), ui.connFileRootField->text());

	if (fpath.isNull())
		return;

	ui.connFileRootField->setText(fpath);
}


