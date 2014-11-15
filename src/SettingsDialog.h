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

#ifndef SETTINGSDIALOG_H_
#define SETTINGSDIALOG_H_

#include "global.h"
#include "System.h"
#include <QtGui/QDialog>
#include <QtCore/QList>
#include <ui_SettingsDialog.h>
#include "DatabaseConnection.h"


class SettingsDialog : public QDialog
{
	Q_OBJECT

private:
	struct LocalPermDbConn
	{
		DatabaseConnection* globalConn;
		DatabaseConnection* localConn;
	};

	typedef	QList<LocalPermDbConn*> LocalPermDbConnList;

public:
	SettingsDialog(QWidget* parent = NULL);
	~SettingsDialog();

private:
	void applyCurrentConnectionChanges();
	void rebuildStartupConnectionBox();
	void apply();

private slots:
	void connListRowChanged(int row);
	void buttonBoxClicked(QAbstractButton* b);
	void connAddRequested();
	void connRemoveRequested();
	void connTypeBoxActivated(int idx);
	void connSqliteFileChooseButtonClicked();
	void connFileRootChosen();

private:
	Ui_SettingsDialog ui;
	LocalPermDbConnList localPermDbConns;
	LocalPermDbConn* curEdConn;

};

#endif /* SETTINGSDIALOG_H_ */
