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

#pragma once

#include "../global.h"

#include <list>
#include <QCloseEvent>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QProgressBar>
#include <QResizeEvent>
#include <QSplitter>
#include <QTableView>
#include <QTabWidget>
#include <ui_MainWindow.h>
#include "../model/container/PartContainer.h"
#include "../model/part/Part.h"
#include "../model/PartCategory.h"
#include "../util/Filter.h"
#include "../db/DatabaseConnection.h"
#include "part/FilterWidget.h"
#include "part/ListingTable.h"
#include "part/PartCategoryWidget.h"
#include "part/PartDisplayWidget.h"
#include "part/PartTableModel.h"
#include "smodel/PartCategoryEditDialog.h"
#include "smodel/PartLinkTypeEditDialog.h"

namespace electronicsdb
{


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

    void jumpToPart(const Part& part);
    void jumpToContainer(const PartContainer& cont);

protected:
    void closeEvent(QCloseEvent* evt) override;

private:
    void addPartCategoryWidget(PartCategory* partCat);
    void addStatusBarStretcher(int stretch);
    void addStatusBarSpacer(int spacing);
    void addContainerWidget();

private slots:
    void startup();
    void loadWindowState();

    void updateConnectMenu();
    void updateStaticModelMenu();

    void staticModelEditDialogClosed();

    void aboutToQuit();

    void appModelAboutToBeReset();
    void appModelReset();
    void databaseConnectionChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);

    void localChangesStateChanged();

    void saveRequested();

    void connectRequested();
    void connectProfileRequested();
    void disconnectRequested();

    void addContainerWidgetRequested();
    void nextContainerRequested();
    void previousPartRequested();
    void nextPartRequested();

    void settingsRequested();
    void editPcatsRequested();
    void editLtypesRequested();

    void langChangeRequested();
    void aboutRequested();
    void aboutQtRequested();

private:
    Ui_MainWindow ui;

    std::list<ConnectMenuEntry> connectMenuEntries;

    QLabel* connectionStatusLabel;

    QTabWidget* mainTabber;

    QMap<PartCategory*, PartCategoryWidget*> catWidgets;

    QLabel* progressLabel;
    QProgressBar* progressBar;

    PartCategoryEditDialog* pcatEditDlg;
    PartLinkTypeEditDialog* ltypeEditDlg;
};


}
