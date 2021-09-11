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

#include "MainWindow.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <QActionGroup>
#include <QDir>
#include <QDockWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QSqlDatabase>
#include <QtAlgorithms>
#include <QTimer>
#include <QTranslator>
#include <QTableView>
#include <nxcommon/exception/InvalidValueException.h>
#include "../db/SQLDatabaseWrapperFactory.h"
#include "../model/PartCategory.h"
#include "../model/StaticModelManager.h"
#include "../System.h"
#include "container/ContainerWidget.h"
#include "part/FilterWidget.h"
#include "part/PartCategoryWidget.h"
#include "settings/ConnectDialog.h"
#include "settings/SettingsDialog.h"

using std::fstream;

namespace electronicsdb
{


bool _PartCategoryUserReadableNameSortLess(PartCategory* cat1, PartCategory* cat2)
{
    return cat1->getUserReadableName().toLower() < cat2->getUserReadableName().toLower();
}



MainWindow::MainWindow()
        : QMainWindow(nullptr), pcatEditDlg(nullptr), ltypeEditDlg(nullptr)
{
    ui.setupUi(this);

    System* sys = System::getInstance();

    sys->registerMainWindow(this);


    addStatusBarStretcher(1);


    QWidget* progressWidget = new QWidget(ui.statusBar);
    QHBoxLayout* progressLayout = new QHBoxLayout(progressWidget);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressWidget->setLayout(progressLayout);

    progressLabel = new QLabel("", progressWidget);
    progressLabel->hide();
    progressLayout->addWidget(progressLabel);

    progressBar = new QProgressBar(progressWidget);
    progressBar->setFixedWidth(200);
    progressBar->hide();
    progressLayout->addWidget(progressBar);

    ui.statusBar->addPermanentWidget(progressWidget);


    //addStatusBarStretcher(1);
    addStatusBarSpacer(100);


    connectionStatusLabel = new QLabel(tr("Not Connected"), ui.statusBar);
    ui.statusBar->addPermanentWidget(connectionStatusLabel);


    ui.disconnectAction->setEnabled(false);


    QSettings s;

    for (unsigned int i = 0 ; s.childGroups().contains((QString("db%1").arg(i))) ; i++) {
        s.beginGroup(QString("db%1").arg(i));

        DatabaseConnection* conn = sys->loadDatabaseConnectionFromSettings(s);

        if (conn) {
            sys->addPermanentDatabaseConnection(conn);
        }

        s.endGroup();
    }

    QString startupDbID = s.value("main/startup_db").toString();

    if (!startupDbID.isNull()) {
        DatabaseConnection* conn = sys->getPermanentDatabaseConnection(startupDbID);
        sys->setStartupDatabaseConnection(conn);
    }

    updateConnectMenu();


    mainTabber = new QTabWidget(this);
    mainTabber->setContentsMargins(0, 0, 0, 0);
    ui.centralWidget->layout()->addWidget(mainTabber);


    s.beginGroup("gui/main_window");
    unsigned int numContWidgets = s.value("num_container_widgets").toUInt();
    s.endGroup();

    for (unsigned int i = 0 ; i < numContWidgets ; i++) {
        addContainerWidget();
    }


    ui.saveAction->setShortcut(QKeySequence(QKeySequence::Save));
    ui.quitAction->setShortcut(QKeySequence(QKeySequence::Quit));

    ui.undoAction->setShortcut(QKeySequence(QKeySequence::Undo));
    ui.redoAction->setShortcut(QKeySequence(QKeySequence::Redo));

    ui.prevPartAction->setShortcut(QKeySequence("Alt+PgUp"));
    ui.nextPartAction->setShortcut(QKeySequence("Alt+PgDown"));
    ui.nextContainerAction->setShortcut(QKeySequence("Alt+Space"));


    connect(ui.connectAction, &QAction::triggered, this, &MainWindow::connectRequested);
    connect(ui.disconnectAction, &QAction::triggered, this, &MainWindow::disconnectRequested);
    connect(ui.settingsAction, &QAction::triggered, this, &MainWindow::settingsRequested);
    connect(ui.editPcatsAction, &QAction::triggered, this, &MainWindow::editPcatsRequested);
    connect(ui.editLtypesAction, &QAction::triggered, this, &MainWindow::editLtypesRequested);
    connect(ui.quitAction, &QAction::triggered, sys, &System::quit);
    connect(ui.saveAction, &QAction::triggered, this, &MainWindow::saveRequested);

    connect(ui.prevPartAction, &QAction::triggered, this, &MainWindow::previousPartRequested);
    connect(ui.nextPartAction, &QAction::triggered, this, &MainWindow::nextPartRequested);
    connect(ui.nextContainerAction, &QAction::triggered, this, &MainWindow::nextContainerRequested);

    connect(ui.addContainerWidgetAction, &QAction::triggered, this, &MainWindow::addContainerWidgetRequested);

    DatabaseConnection* dbConn = sys->getCurrentDatabaseConnection();

    if (dbConn) {
        databaseConnectionChanged(nullptr, dbConn);
    }

    EditStack* editStack = sys->getEditStack();

    connect(sys, &System::databaseConnectionChanged, this, &MainWindow::databaseConnectionChanged);
    connect(sys, &System::permanentDatabaseConnectionSettingsSaved, this, &MainWindow::updateConnectMenu);
    connect(sys, &System::appModelAboutToBeReset, this, &MainWindow::appModelAboutToBeReset);
    connect(sys, &System::appModelReset, this, &MainWindow::appModelReset);
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::aboutToQuit);

    connect(editStack, &EditStack::canUndoStatusChanged, ui.undoAction, &QAction::setEnabled);
    connect(editStack, &EditStack::canRedoStatusChanged, ui.redoAction, &QAction::setEnabled);

    connect(ui.undoAction, &QAction::triggered, editStack, &EditStack::undo);
    connect(ui.redoAction, &QAction::triggered, editStack, &EditStack::redo);

    connect(ui.aboutAction, &QAction::triggered, this, &MainWindow::aboutRequested);
    connect(ui.aboutQtAction, &QAction::triggered, this, &MainWindow::aboutQtRequested);

    QActionGroup* langGroup = new QActionGroup(this);

    for (QString langCode : sys->getInstalledLanguages()) {
        QString langName = sys->getLanguageName(langCode);

        QAction* langAction = new QAction(langName, this);
        connect(langAction, &QAction::triggered, this, &MainWindow::langChangeRequested);
        langAction->setData(langCode);
        langGroup->addAction(langAction);
        langAction->setCheckable(true);
        ui.menuLanguage->addAction(langAction);

        if (langCode == sys->getActiveLanguage()) {
            langAction->setChecked(true);
        }
    }


    updateStaticModelMenu();

    // Must be executed before restoreGeometry(), because some stupid X11 window managers don't restore the maximized
    // state otherwise.
    show();

    s.beginGroup("gui/main_window");
    restoreGeometry(s.value("geometry").toByteArray());
    s.endGroup();


    QTimer::singleShot(0, this, &MainWindow::startup);

    int loadStateDelay = s.value("gui/load_window_state_delay", 500).toInt();
    QTimer::singleShot(loadStateDelay, this, &MainWindow::loadWindowState);
}

MainWindow::~MainWindow()
{
}

void MainWindow::startup()
{
    System* sys = System::getInstance();

    DatabaseConnection* conn = sys->getStartupDatabaseConnection();

    if (conn) {
        bool ok;
        QString pw = sys->askConnectDatabasePassword(conn, &ok);
        if (ok) {
            sys->connectDatabase(conn, pw);
        }
    }
}

void MainWindow::loadWindowState()
{
    QSettings s;
    s.beginGroup("gui/main_window");
    restoreState(s.value("state").toByteArray());
    s.endGroup();
}

void MainWindow::appModelAboutToBeReset()
{
    System* sys = System::getInstance();

    mainTabber->clear();

    for (PartCategoryWidget* widget : catWidgets.values()) {
        delete widget;
    }
    catWidgets.clear();
}

void MainWindow::appModelReset()
{
    for (PartCategory* cat : System::getInstance()->getPartCategories()) {
        addPartCategoryWidget(cat);
    }

    updateStaticModelMenu();
}

void MainWindow::addPartCategoryWidget(PartCategory* partCat)
{
    PartCategoryWidget* partWidget = new PartCategoryWidget(partCat, this);
    connect(partWidget->getPartDisplayWidget(), &PartDisplayWidget::hasLocalChangesStatusChanged,
            this, &MainWindow::localChangesStateChanged);
    mainTabber->addTab(partWidget, partCat->getUserReadableName());

    catWidgets[partCat] = partWidget;
}

void MainWindow::addStatusBarStretcher(int stretch)
{
    QWidget* stretchWidget = new QWidget(ui.statusBar);
    ui.statusBar->addPermanentWidget(stretchWidget, stretch);
}

void MainWindow::addStatusBarSpacer(int spacing)
{
    QWidget* spacerWidget = new QWidget(ui.statusBar);
    QHBoxLayout* spacerLayout = new QHBoxLayout(spacerWidget);
    spacerLayout->setContentsMargins(0, 0, 0, 0);
    spacerWidget->setLayout(spacerLayout);
    spacerLayout->addSpacing(spacing);
    ui.statusBar->addPermanentWidget(spacerWidget);
}

void MainWindow::updateConnectMenu()
{
    for (ConnectMenuEntry& entry : connectMenuEntries) {
        QAction* action = entry.action;
        ui.connectMenu->removeAction(action);
        delete action;
    }

    connectMenuEntries.clear();

    System* sys = System::getInstance();

    for (DatabaseConnection* conn : sys->getPermanentDatabaseConnections()) {
        QAction* action = new QAction(conn->getName(), ui.connectMenu);
        ConnectMenuEntry e;
        e.action = action;
        e.conn = conn;

        connect(action, &QAction::triggered, this, &MainWindow::connectProfileRequested);

        ui.connectMenu->addAction(action);

        connectMenuEntries.push_back(e);
    }
}

void MainWindow::updateStaticModelMenu()
{
    System* sys = System::getInstance();

    bool enabled = true;

    if (pcatEditDlg  ||  ltypeEditDlg) {
        enabled = false;
    }
    if (!sys->hasValidDatabaseConnection()  ||  !sys->isAppModelLoaded()) {
        enabled = false;
    }

    ui.editPcatsAction->setEnabled(enabled);
    ui.editLtypesAction->setEnabled(enabled);
}

void MainWindow::staticModelEditDialogClosed()
{
    pcatEditDlg = nullptr;
    ltypeEditDlg = nullptr;

    updateStaticModelMenu();
}

void MainWindow::connectRequested()
{
    ConnectDialog dlg(this);
    dlg.exec();
}

void MainWindow::connectProfileRequested()
{
    QAction* action = (QAction*) sender();

    DatabaseConnection* conn = nullptr;

    for (auto it = connectMenuEntries.begin() ; it != connectMenuEntries.end() ; it++) {
        ConnectMenuEntry e = *it;

        if (action == e.action) {
            conn = e.conn;
            break;
        }
    }

    if (!conn)
        return;

    System* sys = System::getInstance();

    bool ok;
    QString pw = sys->askConnectDatabasePassword(conn, &ok);
    if (ok) {
        sys->connectDatabase(conn, pw);
    }
}

void MainWindow::disconnectRequested()
{
    System* sys = System::getInstance();
    if (sys->getCurrentDatabaseConnection()) {
        sys->disconnect(false);
    }
}

void MainWindow::settingsRequested()
{
    SettingsDialog dlg;
    dlg.exec();
}

void MainWindow::editPcatsRequested()
{
    pcatEditDlg = new PartCategoryEditDialog(this);
    pcatEditDlg->show();

    connect(pcatEditDlg, &PartCategoryEditDialog::dialogClosed, this, &MainWindow::staticModelEditDialogClosed);

    updateStaticModelMenu();
}

void MainWindow::editLtypesRequested()
{
    ltypeEditDlg = new PartLinkTypeEditDialog(this);
    ltypeEditDlg->show();

    connect(ltypeEditDlg, &PartLinkTypeEditDialog::dialogClosed, this, &MainWindow::staticModelEditDialogClosed);

    updateStaticModelMenu();
}

void MainWindow::databaseConnectionChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
    System* sys = System::getInstance();

    if (newConn  &&  sys->hasValidDatabaseConnection()) {
        QString serverDesc = newConn->getDescription();
        connectionStatusLabel->setText(QString(tr("Connected to %1")).arg(serverDesc));
        ui.disconnectAction->setEnabled(true);
    } else if (!newConn  &&  oldConn) {
        connectionStatusLabel->setText(tr("Not Connected"));
        ui.disconnectAction->setEnabled(false);
    }

    updateStaticModelMenu();
}

void MainWindow::aboutToQuit()
{
    QSettings s;

    s.beginGroup("gui/main_window");

    s.setValue("geometry", saveGeometry());
    s.setValue("state", saveState());
    s.setValue("num_container_widgets", findChildren<QDockWidget*>().size());

    s.endGroup();

    s.sync();
}

void MainWindow::saveRequested()
{
    for (auto it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
        PartCategoryWidget* cw = it.value();
        cw->getPartDisplayWidget()->saveChanges();
    }
}

void MainWindow::localChangesStateChanged()
{
    bool hasChanges = false;

    for (auto it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
        PartCategoryWidget* cw = it.value();

        if (cw->getPartDisplayWidget()->hasLocalChanges()) {
            hasChanges = true;
            break;
        }
    }

    ui.saveAction->setEnabled(hasChanges);
}

void MainWindow::setProgressBarVisible(bool visible)
{
    progressLabel->setVisible(visible);
    progressBar->setVisible(visible);
}

void MainWindow::setProgressBarRange(int min, int max)
{
    progressBar->setRange(min, max);
}

void MainWindow::setProgressBarDescription(const QString& desc)
{
    progressLabel->setText(desc);
}

void MainWindow::updateProgressBar(int value)
{
    progressBar->setValue(value);
}

void MainWindow::closeEvent(QCloseEvent* evt)
{
    System::getInstance()->quit();
}

void MainWindow::jumpToPart(const Part& part)
{
    PartCategoryWidget* cw = catWidgets[const_cast<PartCategory*>(part.getPartCategory())];

    mainTabber->setCurrentWidget(cw);

    cw->jumpToPart(part);
}

void MainWindow::jumpToContainer(const PartContainer& cont)
{
    QList<QDockWidget*> dws = findChildren<QDockWidget*>();

    for (QDockWidget* dw : dws) {
        ContainerWidget* cw = (ContainerWidget*) dw->widget();
        cw->jumpToContainer(cont);
    }
}

void MainWindow::addContainerWidget()
{
    QDockWidget* dw = new QDockWidget(tr("Containers"), this);
    dw->setObjectName(QString("dockWidget%1").arg(findChildren<QDockWidget*>().size()));
    dw->setAttribute(Qt::WA_DeleteOnClose);

    ContainerWidget* cw = new ContainerWidget(dw);
    dw->setWidget(cw);

    cw->setConfigurationName(QString("dockwidget%1").arg(findChildren<QDockWidget*>().size()));

    addDockWidget(Qt::LeftDockWidgetArea, dw);
}

void MainWindow::addContainerWidgetRequested()
{
    addContainerWidget();
}

void MainWindow::nextContainerRequested()
{
    QList<QDockWidget*> dws = findChildren<QDockWidget*>();

    for (int i = 0 ; i < dws.size() ; i++) {
        QDockWidget* dw = dws[i];
        ContainerWidget* cw = (ContainerWidget*) dw->widget();
        cw->gotoNextContainer();
    }
}

void MainWindow::nextPartRequested()
{
    QWidget* currentWidget = mainTabber->currentWidget();

    PartCategory* cat = nullptr;
    PartCategoryWidget* cw = nullptr;

    for (auto it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
        if (currentWidget == it.value()) {
            cat = it.key();
            cw = it.value();
            break;
        }
    }

    if (!cat) {
        return;
    }

    cw->gotoNextPart();
}

void MainWindow::previousPartRequested()
{
    QWidget* currentWidget = mainTabber->currentWidget();

    PartCategory* cat = nullptr;
    PartCategoryWidget* cw = nullptr;

    for (auto it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
        if (currentWidget == it.value()) {
            cat = it.key();
            cw = it.value();
            break;
        }
    }

    if (!cat) {
        return;
    }

    cw->gotoPreviousPart();
}

void MainWindow::aboutRequested()
{
    QList<std::pair<QString, QString>> infoData;

    infoData.push_back({tr("Version"), QString(EDB_VERSION_STRING).toHtmlEscaped()});
    infoData.push_back({tr("SQL Drivers (Qt)"), QSqlDatabase::drivers().join(", ").toHtmlEscaped()});

    if (System::getInstance()->hasValidDatabaseConnection()) {
        QSqlDatabase db = QSqlDatabase::database();
        std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

        infoData.push_back({tr("Database"), dbw->getDatabaseDescription().toHtmlEscaped()});
    }

    QString infoText;
    for (auto it = infoData.begin() ; it != infoData.end() ; ++it) {
        const auto& d = *it;
        infoText += QString("<tr><td style=\"padding: 5px 0;\">%1</td><td style=\"padding: 5px 15px;\">%2</td></tr>")
                .arg(d.first, d.second);
    }

    infoText = QString("<table border=\"0\">%1</table>").arg(infoText);

    QString aboutText = QString(tr(
            "AboutText (infoTable: %1)"
            )).arg(infoText);

    QMessageBox::about(this, tr("About %1").arg(qApp->applicationDisplayName()), aboutText);
}

void MainWindow::aboutQtRequested()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::langChangeRequested()
{
    QAction* action = (QAction*) sender();

    QString langCode = action->data().toString();

    QSettings s;

    s.setValue("main/lang", langCode);

    QMessageBox::information(this, tr("Restart Needed"), tr("You need to restart the program for the new language to become active!"));
}


}
