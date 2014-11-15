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

#ifndef CONNECTDIALOG_H_
#define CONNECTDIALOG_H_

#include "global.h"
#include "System.h"
#include <QtGui/QDialog>
#include <ui_ConnectDialog.h>


class ConnectDialog : public QDialog
{
	Q_OBJECT

public:
	ConnectDialog(QWidget* parent = NULL);

private slots:
	void connectRequested();
	void typeBoxActivated(int idx);
	void sqliteFileChosen();

private:
	Ui_ConnectDialog ui;
};

#endif /* CONNECTDIALOG_H_ */
