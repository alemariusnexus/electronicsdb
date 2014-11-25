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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "global.h"
#include <QtGui/QMainWindow>
#include <QtGui/QLabel>
#include <QtGui/QResizeEvent>
#include <QtGui/QTableView>
#include <QtGui/QSplitter>
#include <QtGui/QTabWidget>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QCloseEvent>
#include <QtGui/QDockWidget>
#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <ui_MainWindow.h>
#include "DatabaseConnection.h"
#include "FilterWidget.h"
#include "PartCategory.h"
#include "Filter.h"
#include "PartTableModel.h"
#include "ListingTable.h"
#include "PartDisplayWidget.h"
#include "PartCategoryWidget.h"



class MainWindow : public QMainWindow
{
	Q_OBJECT

private:
	struct ConnectMenuEntry
	{
		QAction* action;
		DatabaseConnection* conn;
	};

public:
	MainWindow();
	~MainWindow();

	void setProgressBarVisible(bool visible);
	void setProgressBarRange(int min, int max);
	void setProgressBarDescription(const QString& desc);
	void updateProgressBar(int value);
	int getProgressBarValue() const { return progressBar->value(); }

	void jumpToPart(PartCategory* cat, unsigned int id);
	void jumpToContainer(unsigned int cid);

protected:
	void closeEvent(QCloseEvent* evt);

private:
	void addPartCategoryWidget(PartCategory* partCat);
	void addStatusBarStretcher(int stretch);
	void addStatusBarSpacer(int spacing);
	void addContainerWidget();

private slots:
	void startup();
	void loadWindowState();
	void connectRequested();
	void connectProfileRequested();
	void disconnectRequested();
	void partCategoriesAboutToChange();
	void partCategoriesChanged();
	void databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);
	void settingsRequested();
	void updateConnectMenu();
	void aboutToQuit();
	void saveRequested();
	void localChangesStateChanged();
	void addContainerWidgetRequested();
	void nextContainerRequested();
	void previousPartRequested();
	void nextPartRequested();
	void aboutRequested();
	void aboutQtRequested();
	void langChangeRequested();
	void sqlGeneratorRequested();

private:
	Ui_MainWindow ui;

	QLinkedList<ConnectMenuEntry> connectMenuEntries;

	QLabel* connectionStatusLabel;

	QTabWidget* mainTabber;

	QMap<PartCategory*, PartCategoryWidget*> catWidgets;

	QLabel* progressLabel;
	QProgressBar* progressBar;
};

#endif /* MAINWINDOW_H_ */
