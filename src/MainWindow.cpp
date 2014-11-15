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

#include "MainWindow.h"
#include "ConnectDialog.h"
#include "System.h"
#include "SettingsDialog.h"
#include "FilterWidget.h"
#include "PartCategory.h"
#include "PartCategoryWidget.h"
#include "ContainerWidget.h"
#include "PartCategoryProvider.h"
#include <cstdio>
#include <fstream>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QRegExp>
#include <QtCore/QDir>
#include <QtCore/QTranslator>
#include <QtGui/QInputDialog>
#include <QtGui/QTableView>
#include <QtGui/QSplitter>
#include <QtGui/QMessageBox>
#include <QtGui/QDockWidget>
#include <QtCore/QtAlgorithms>
#include <nxcommon/sql/sql.h>

using std::fstream;




bool _PartCategoryUserReadableNameSortLess(PartCategory* cat1, PartCategory* cat2)
{
	return cat1->getUserReadableName().toLower() < cat2->getUserReadableName().toLower();
}



MainWindow::MainWindow()
		: QMainWindow(NULL)
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


	QList<PartCategory*> cats = PartCategoryProvider::getInstance()->buildCategories();

	for (QList<PartCategory*>::iterator it = cats.begin() ; it != cats.end() ; it++) {
		PartCategory* cat = *it;
		sys->addPartCategory(cat);
	}


	QSettings s;

	for (unsigned int i = 0 ; s.childGroups().contains((QString("db%1").arg(i))) ; i++) {
		s.beginGroup(QString("db%1").arg(i));

		QString type = s.value("type", "sqlite").toString();

		DatabaseConnection* conn;

		if (type == "mysql") {
			conn = new DatabaseConnection(DatabaseConnection::MySQL);

			QString host = s.value("host", "localhost").toString();
			unsigned int port = s.value("port", 3306).toUInt();
			QString user = s.value("user", "electronics").toString();
			QString db = s.value("db", "electronics").toString();
			QString connName = s.value("connname", QString("%1@%2:%3").arg(user).arg(host).arg(port)).toString();
			QString fileRoot = s.value("fileroot", QString()).toString();

			conn->setType(DatabaseConnection::MySQL);
			conn->setMySQLHost(host);
			conn->setMySQLPort(port);
			conn->setMySQLUser(user);
			conn->setMySQLDatabaseName(db);
			conn->setName(connName);
			conn->setFileRoot(fileRoot);
		} else {
			conn = new DatabaseConnection(DatabaseConnection::SQLite);

			QString path = s.value("path").toString();
			QString connName = s.value("connname", path).toString();
			QString fileRoot = s.value("fileroot", QString()).toString();

			conn->setType(DatabaseConnection::SQLite);
			conn->setSQLiteFilePath(path);
			conn->setName(connName);
			conn->setFileRoot(fileRoot);
		}

		sys->addPermanentDatabaseConnection(conn);

		s.endGroup();
	}

	int startupDbIdx = s.value("main/startup_db", -1).toInt();

	if (startupDbIdx >= 0  &&  startupDbIdx < sys->getPermanentDatabaseConnectionSize()) {
		DatabaseConnection* conn = sys->getPermanentDatabaseConnection(startupDbIdx);
		sys->setStartupDatabaseConnection(conn);
	}

	updateConnectMenu();


	bool siBinPrefixes = s.value("main/si_binary_prefixes_default", true).toBool();

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


	/*PartCategory* ucCat = new PartCategory("microcontroller", tr("Microcontrollers"));
	ucCat->setDescriptionPattern("uC: $model (#$id)");
	sys->addPartCategory(ucCat);

    PartProperty* modelProp = new PartProperty("model", PartProperty::String,
    		PartProperty::FullTextIndex | PartProperty::DisplayDynamicEnum | PartProperty::DescriptionProperty);
    modelProp->setUserReadableName(tr("Model"));
    modelProp->setStringMaximumLength(128);
    ucCat->addProperty(modelProp);
    ucCat->setDescriptionProperty(modelProp);

    PartProperty* pkgProp = new PartProperty("package", PartProperty::String, PartProperty::FullTextIndex);
    pkgProp->setUserReadableName(tr("Package"));
    pkgProp->setStringMaximumLength(32);
    ucCat->addProperty(pkgProp);

    PartProperty* pmProp = new PartProperty("progmemsize", PartProperty::Integer,
    		PartProperty::DisplayWithUnits | PartProperty::DisplayBinaryPrefixes | PartProperty::DisplayDynamicEnum
    		| siBinaryPrefixFlags);
    pmProp->setUnitSuffix("B");
    pmProp->setUserReadableName(tr("Program Memory"));
    ucCat->addProperty(pmProp);

    PartProperty* ramProp = new PartProperty("ramsize", PartProperty::Integer,
    		PartProperty::DisplayWithUnits | PartProperty::DisplayBinaryPrefixes | siBinaryPrefixFlags);
    ramProp->setUnitSuffix("B");
    ramProp->setUserReadableName(tr("RAM Size"));
    ucCat->addProperty(ramProp);

    PartProperty* v3Prop = new PartProperty("voltage", PartProperty::Decimal,
    		PartProperty::MultiValue | PartProperty::DisplayWithUnits | PartProperty::DisplayTextArea);
    v3Prop->setUserReadableName(tr("Voltage"));
    v3Prop->setMultiValueForeignTableName("voltage3");
    v3Prop->setMultiValueIDFieldName("mid");
    v3Prop->setUnitSuffix("V");
    v3Prop->setDecimalConstraint(18, 6);
    ucCat->addProperty(v3Prop);

    PartProperty* dsProp = new PartProperty("datasheet", PartProperty::PartLink);
    dsProp->setUserReadableName(tr("Datasheet"));
    ucCat->addProperty(dsProp);

    PartProperty* notesProp = new PartProperty("notes", PartProperty::String,
    		PartProperty::FullTextIndex | PartProperty::DisplayNoSelectionList
    		| PartProperty::DisplayHideFromListingTable | PartProperty::DisplayTextArea);
    notesProp->setUserReadableName(tr("Notes"));
    notesProp->setStringMaximumLength(4096);
    ucCat->addProperty(notesProp);

    PartProperty* coolProp = new PartProperty("iscool", PartProperty::Boolean, PartProperty::MultiValue);
    coolProp->setUserReadableName("Is Cool");
    coolProp->setMultiValueForeignTableName("microcontroller_iscool");
    coolProp->setMultiValueIDFieldName("mid");
    coolProp->setBooleanDisplayText("Yes", "No");
    ucCat->addProperty(coolProp);



    PartCategory* dsheetCat = new PartCategory("datasheet", tr("Datasheets"));
    dsheetCat->setDescriptionPattern("Datasheet");
    sys->addPartCategory(dsheetCat);

    PartProperty* titleProp = new PartProperty("title", PartProperty::String, PartProperty::FullTextIndex);
    titleProp->setUserReadableName(tr("Title"));
    dsheetCat->addProperty(titleProp);
    dsheetCat->setDescriptionProperty(titleProp);

    PartProperty* fnameProp = new PartProperty("filename", PartProperty::File, PartProperty::FullTextIndex);
    fnameProp->setUserReadableName(tr("File Name"));
    dsheetCat->addProperty(fnameProp);


    dsProp->setPartLinkCategory(dsheetCat);*/







    mainTabber = new QTabWidget(this);
    mainTabber->setContentsMargins(0, 0, 0, 0);
    ui.centralWidget->layout()->addWidget(mainTabber);




    cats = sys->getPartCategories();
    qSort(cats.begin(), cats.end(), &_PartCategoryUserReadableNameSortLess);

    for (System::PartCategoryIterator it = cats.begin() ; it != cats.end() ; it++) {
    	PartCategory* cat = *it;
    	addPartCategoryWidget(cat);
    }

    //addPartCategoryWidget(ucCat);
    //addPartCategoryWidget(dsheetCat);


    s.beginGroup("gui_geometry");
    unsigned int numContWidgets = s.value("main_window_num_container_widgets").toUInt();
    s.endGroup();

    for (unsigned int i = 0 ; i < numContWidgets ; i++)
    	addContainerWidget();


    ui.saveAction->setShortcut(QKeySequence(QKeySequence::Save));
    ui.quitAction->setShortcut(QKeySequence(QKeySequence::Quit));

    ui.undoAction->setShortcut(QKeySequence(QKeySequence::Undo));
    ui.redoAction->setShortcut(QKeySequence(QKeySequence::Redo));

    ui.prevPartAction->setShortcut(QKeySequence("Alt+PgUp"));
    ui.nextPartAction->setShortcut(QKeySequence("Alt+PgDown"));
    ui.nextContainerAction->setShortcut(QKeySequence("Alt+Space"));


	connect(ui.connectAction, SIGNAL(triggered()), this, SLOT(connectRequested()));
	connect(ui.disconnectAction, SIGNAL(triggered()), this, SLOT(disconnectRequested()));
	connect(ui.settingsAction, SIGNAL(triggered()), this, SLOT(settingsRequested()));
	connect(ui.quitAction, SIGNAL(triggered()), sys, SLOT(quit()));
	connect(ui.saveAction, SIGNAL(triggered()), this, SLOT(saveRequested()));

	connect(ui.prevPartAction, SIGNAL(triggered()), this, SLOT(previousPartRequested()));
	connect(ui.nextPartAction, SIGNAL(triggered()), this, SLOT(nextPartRequested()));
	connect(ui.nextContainerAction, SIGNAL(triggered()), this, SLOT(nextContainerRequested()));

	connect(ui.addContainerWidgetAction, SIGNAL(triggered()), this, SLOT(addContainerWidgetRequested()));

	DatabaseConnection* dbConn = sys->getCurrentDatabaseConnection();

	if (dbConn) {
		databaseConnectionStatusChanged(NULL, dbConn);
	}

	EditStack* editStack = sys->getEditStack();

	connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)));
	connect(sys, SIGNAL(permanentDatabaseConnectionSettingsSaved()), this, SLOT(updateConnectMenu()));
	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

	connect(editStack, SIGNAL(canUndoStatusChanged(bool)), ui.undoAction, SLOT(setEnabled(bool)));
	connect(editStack, SIGNAL(canRedoStatusChanged(bool)), ui.redoAction, SLOT(setEnabled(bool)));

	connect(ui.undoAction, SIGNAL(triggered()), editStack, SLOT(undo()));
	connect(ui.redoAction, SIGNAL(triggered()), editStack, SLOT(redo()));

	connect(ui.aboutAction, SIGNAL(triggered()), this, SLOT(aboutRequested()));
	connect(ui.aboutQtAction, SIGNAL(triggered()), this, SLOT(aboutQtRequested()));

	/*QDir resDir(":/");

	QStringList qmFiles = resDir.entryList(QStringList() << "electronics_*.qm", QDir::Files, QDir::NoSort);

	for (QString qmFile : qmFiles) {
		QTranslator ttrans;
		ttrans.load(QString(":/%1").arg(qmFile));

		QRegExp langRegex("electronics_(.*).qm", Qt::CaseSensitive, QRegExp::RegExp);
		langRegex.exactMatch(qmFile);
		QString langCode = langRegex.cap(1);
		QString langName = ttrans.translate("Global", "ThisLanguage");

		QAction* langAction = new QAction(langName, this);
		langAction->setCheckable(true);
		ui.menuLanguage->addAction(langAction);

		printf("Found: %s (%s, %s)\n", qmFile.toLocal8Bit().constData(), langCode.toLocal8Bit().constData(),
				langName.toLocal8Bit().constData());
	}*/

	QActionGroup* langGroup = new QActionGroup(this);

	for (QString langCode : sys->getInstalledLanguages()) {
		QString langName = sys->getLanguageName(langCode);

		QAction* langAction = new QAction(langName, this);
		connect(langAction, SIGNAL(triggered()), this, SLOT(langChangeRequested()));
		langAction->setData(langCode);
		langGroup->addAction(langAction);
		langAction->setCheckable(true);
		ui.menuLanguage->addAction(langAction);

		if (langCode == sys->getActiveLanguage()) {
			langAction->setChecked(true);
		}
	}


    // Must be executed before restoreGeometry(), because some stupid X11 window managers don't restore the maximized
	// state otherwise.
	show();

	s.beginGroup("gui_geometry");
    restoreGeometry(s.value("main_window_geometry").toByteArray());
    s.endGroup();


    QTimer::singleShot(0, this, SLOT(startup()));

    unsigned int loadStateDelay = s.value("advanced/load_window_state_delay", 500).toUInt();
    QTimer::singleShot(loadStateDelay, this, SLOT(loadWindowState()));
}


MainWindow::~MainWindow()
{
}


void MainWindow::startup()
{
	System* sys = System::getInstance();

	DatabaseConnection* conn = sys->getStartupDatabaseConnection();

	if (conn) {
		bool ok = true;
		QString pw;

		if (conn->getType() == DatabaseConnection::MySQL) {
			pw = QInputDialog::getText(this, tr("Enter Password"),
					QString(tr("Enter password for connection '%1':")).arg(conn->getName()),
					QLineEdit::Password, QString(), &ok);
		}

		if (ok) {
			sys->connectDatabase(conn, pw);
		}
	}
}


void MainWindow::loadWindowState()
{
	QSettings s;
	s.beginGroup("gui_geometry");
    restoreState(s.value("main_window_state").toByteArray());
    s.endGroup();
}


void MainWindow::addPartCategoryWidget(PartCategory* partCat)
{
	PartCategoryWidget* partWidget = new PartCategoryWidget(partCat, this);
	connect(partWidget->getPartDisplayWidget(), SIGNAL(hasLocalChangesStatusChanged(bool)),
			this, SLOT(localChangesStateChanged()));
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
	for (QLinkedList<ConnectMenuEntry>::iterator it = connectMenuEntries.begin() ; it != connectMenuEntries.end() ; it++) {
		QAction* action = it->action;
		ui.connectMenu->removeAction(action);
		delete action;
	}

	connectMenuEntries.clear();

	System* sys = System::getInstance();

	System::DatabaseConnectionIterator it;
	for (it = sys->getPermanentDatabaseConnectionsBegin() ; it != sys->getPermanentDatabaseConnectionsEnd() ; it++) {
		DatabaseConnection* conn = *it;

		QAction* action = new QAction(conn->getName(), ui.connectMenu);
		ConnectMenuEntry e;
		e.action = action;
		e.conn = conn;

		connect(action, SIGNAL(triggered()), this, SLOT(connectProfileRequested()));

		ui.connectMenu->addAction(action);

		connectMenuEntries.push_back(e);
	}
}


void MainWindow::connectRequested()
{
	ConnectDialog dlg(this);
	dlg.exec();
}


void MainWindow::connectProfileRequested()
{
	QAction* action = (QAction*) sender();

	DatabaseConnection* conn = NULL;

	for (QLinkedList<ConnectMenuEntry>::iterator it = connectMenuEntries.begin() ; it != connectMenuEntries.end() ; it++) {
		ConnectMenuEntry e = *it;

		if (action == e.action) {
			conn = e.conn;
			break;
		}
	}

	if (!conn)
		return;

	QString pw;

	if (conn->getType() == DatabaseConnection::MySQL) {
		bool ok;
		pw = QInputDialog::getText(this, tr("Enter Password"),
				QString(tr("Enter password for connection '%1':")).arg(conn->getName()),
				QLineEdit::Password, QString(), &ok);

		if (!ok)
			return;
	}

	System* sys = System::getInstance();

	sys->connectDatabase(conn, pw);
}


void MainWindow::disconnectRequested()
{
	System* sys = System::getInstance();
	DatabaseConnection* conn = sys->getCurrentDatabaseConnection();

	if (conn) {
		sys->disconnect(false);

		if (!sys->isPermanentDatabaseConnection(conn)) {
			delete conn;
		}
	}
}


void MainWindow::settingsRequested()
{
	SettingsDialog dlg;
	dlg.exec();
}


void MainWindow::databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
	System* sys = System::getInstance();

	if (newConn  &&  sys->hasValidDatabaseConnection()) {
		QString serverDesc;

		if (newConn->getType() == DatabaseConnection::MySQL) {
			serverDesc = QString(tr("MySQL - %1@%2:%3")).arg(newConn->getMySQLUser()).arg(newConn->getMySQLHost())
					.arg(newConn->getMySQLPort());

			if (!newConn->getName().isNull()) {
				serverDesc = QString(tr("%1 (%2)")).arg(newConn->getName()).arg(serverDesc);
			}
		} else {
			serverDesc = QString(tr("SQLite - %1")).arg(newConn->getSQLiteFilePath());
		}

		connectionStatusLabel->setText(QString(tr("Connected to %1")).arg(serverDesc));
		ui.disconnectAction->setEnabled(true);
	} else if (!newConn  &&  oldConn) {
		connectionStatusLabel->setText(tr("Not Connected"));
		ui.disconnectAction->setEnabled(false);
	}
}


void MainWindow::aboutToQuit()
{
	QSettings s;

	s.beginGroup("gui_geometry");

	s.setValue("main_window_geometry", saveGeometry());
	s.setValue("main_window_state", saveState());
	s.setValue("main_window_num_container_widgets", findChildren<QDockWidget*>().size());

	s.endGroup();

	s.sync();
}


void MainWindow::saveRequested()
{
	for (QMap<PartCategory*, PartCategoryWidget*>::iterator it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
		PartCategoryWidget* cw = it.value();
		cw->getPartDisplayWidget()->saveChanges();
	}
}


void MainWindow::localChangesStateChanged()
{
	bool hasChanges = false;

	for (QMap<PartCategory*, PartCategoryWidget*>::iterator it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
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


void MainWindow::jumpToPart(PartCategory* cat, unsigned int id)
{
	PartCategoryWidget* cw = catWidgets[cat];

	mainTabber->setCurrentWidget(cw);

	cw->jumpToPart(id);
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

	for (unsigned int i = 0 ; i < dws.size() ; i++) {
		QDockWidget* dw = dws[i];
		ContainerWidget* cw = (ContainerWidget*) dw->widget();
		cw->gotoNextContainer();
	}
}


void MainWindow::nextPartRequested()
{
	QWidget* currentWidget = mainTabber->currentWidget();

	PartCategory* cat = NULL;
	PartCategoryWidget* cw = NULL;

	for (QMap<PartCategory*, PartCategoryWidget*>::iterator it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
		if (currentWidget == it.value()) {
			cat = it.key();
			cw = it.value();
			break;
		}
	}

	if (!cat)
		return;

	cw->gotoNextPart();
}


void MainWindow::previousPartRequested()
{
	QWidget* currentWidget = mainTabber->currentWidget();

	PartCategory* cat = NULL;
	PartCategoryWidget* cw = NULL;

	for (QMap<PartCategory*, PartCategoryWidget*>::iterator it = catWidgets.begin() ; it != catWidgets.end() ; it++) {
		if (currentWidget == it.value()) {
			cat = it.key();
			cw = it.value();
			break;
		}
	}

	if (!cat)
		return;

	cw->gotoPreviousPart();
}


void MainWindow::jumpToContainer(unsigned int cid)
{
	QList<QDockWidget*> dws = findChildren<QDockWidget*>();

	for (unsigned int i = 0 ; i < dws.size() ; i++) {
		QDockWidget* dw = dws[i];
		ContainerWidget* cw = (ContainerWidget*) dw->widget();
		cw->jumpToContainer(cid);
	}
}


void MainWindow::aboutRequested()
{
	QString aboutText = QString(tr("AboutText")).arg(EDB_GIT_SHA1).arg(EDB_GIT_REFSPEC);
	QMessageBox::about(this, tr("About Electronics Database"), aboutText);
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

