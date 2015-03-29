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

#include "PartCategoryWidget.h"
#include "System.h"
#include <QtCore/QSettings>
#include <QtGui/QSplitter>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>



PartCategoryWidget::PartCategoryWidget(PartCategory* partCat, QWidget* parent)
		: QWidget(parent), partCat(partCat), filter(new Filter), displayFlags(0), displayWidget(NULL), recordAddButton(NULL),
		  recordRemoveButton(NULL), recordDuplicateButton(NULL)
{
	System* sys = System::getInstance();

	QVBoxLayout* topLayout = new QVBoxLayout(this);
	QMargins cm = topLayout->contentsMargins();
	topLayout->setContentsMargins(cm.left(), cm.top(), cm.right(), 0);
	setLayout(topLayout);

	mainSplitter = new QSplitter(Qt::Vertical, this);
    topLayout->addWidget(mainSplitter);


    QGroupBox* filterContWidget = new QGroupBox(tr("Filter"), mainSplitter);
    mainSplitter->addWidget(filterContWidget);

    QVBoxLayout* filterContLayout = new QVBoxLayout(filterContWidget);
    filterContWidget->setLayout(filterContLayout);

    filterWidget = new FilterWidget(mainSplitter, partCat);
    connect(filterWidget, SIGNAL(applied()), this, SLOT(filterApplied()));
    filterContLayout->addWidget(filterWidget);


    //QWidget* displayContWidget = new QWidget(mainSplitter);
    QGroupBox* displayContWidget = new QGroupBox(tr("Parts"), mainSplitter);
    mainSplitter->addWidget(displayContWidget);

    QHBoxLayout* displayContLayout = new QHBoxLayout(displayContWidget);
    cm = displayContLayout->contentsMargins();
    displayContLayout->setContentsMargins(cm.left(), cm.top(), cm.right(), 0);
    displayContWidget->setLayout(displayContLayout);


    displaySplitter = new QSplitter(Qt::Horizontal, displayContWidget);
    displayContLayout->addWidget(displaySplitter);

    QWidget* listingWidget = new QWidget(displaySplitter);
    displaySplitter->addWidget(listingWidget);

    QVBoxLayout* listingLayout = new QVBoxLayout(listingWidget);
    listingWidget->setLayout(listingLayout);

    listingTable = new ListingTable(partCat, filter, listingWidget);
    connect(listingTable, SIGNAL(currentPartChanged(unsigned int)), this, SLOT(listingCurrentPartChanged(unsigned int)));
    connect(listingTable, SIGNAL(partActivated(unsigned int)), this, SLOT(listingPartActivated(unsigned int)));
    connect(listingTable, SIGNAL(deletePartsRequested(const QList<unsigned int>&)), this,
    		SLOT(listingDeletePartsRequested(const QList<unsigned int>&)));
    connect(listingTable, SIGNAL(addPartRequested()), this, SLOT(listingAddPartRequested()));
    connect(listingTable, SIGNAL(duplicatePartsRequested(const QList<unsigned int>&)), this,
    		SLOT(listingDuplicatePartsRequested(const QList<unsigned int>&)));
    connect(listingTable, SIGNAL(selectedPartsChanged(const QList<unsigned int>&)),
    		this, SLOT(listingTableSelectionChanged(const QList<unsigned int>&)));

    listingLayout->addWidget(listingTable);
    listingTable->updateData();

    listingTableModel = listingTable->getModel();

	/*listingTableModel = new PartTableModel(partCat, filter, this);
	listingTable->setModel(listingTableModel);
	listingTable->updateData();*/


	QWidget* listingButtonWidget = new QWidget(listingWidget);
	listingLayout->addWidget(listingButtonWidget);

	QHBoxLayout* listingButtonLayout = new QHBoxLayout(listingButtonWidget);
	listingButtonWidget->setLayout(listingButtonLayout);

	listingButtonLayout->addStretch(1);

	recordAddButton = new QPushButton(QIcon::fromTheme("document-new", QIcon(":/icons/document-new.png")),
			"", listingButtonWidget);
	connect(recordAddButton, SIGNAL(clicked()), this, SLOT(recordAddRequested()));
	listingButtonLayout->addWidget(recordAddButton);

	recordRemoveButton = new QPushButton(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.png")),
			"", listingButtonWidget);
	recordRemoveButton->setEnabled(false);
	connect(recordRemoveButton, SIGNAL(clicked()), this, SLOT(recordRemoveRequested()));
	listingButtonLayout->addWidget(recordRemoveButton);

	recordDuplicateButton = new QPushButton(QIcon::fromTheme("edit-copy", QIcon(":/icons/edit-copy.png")),
			"", listingButtonWidget);
	recordDuplicateButton->setEnabled(false);
	connect(recordDuplicateButton, SIGNAL(clicked()), this, SLOT(recordDuplicateRequested()));
	listingButtonLayout->addWidget(recordDuplicateButton);

	listingButtonLayout->addStretch(1);


	QGroupBox* detailWidget = new QGroupBox(tr("Part Details"), displaySplitter);
	displaySplitter->addWidget(detailWidget);

	QVBoxLayout* detailLayout = new QVBoxLayout(detailWidget);
	cm = detailLayout->contentsMargins();
    detailLayout->setContentsMargins(cm.left(), cm.top(), cm.right(), 0);
	detailLayout->setContentsMargins(0, 0, 0, 0);
	detailWidget->setLayout(detailLayout);

	displayWidget = new PartDisplayWidget(partCat, detailWidget);
	connect(displayWidget, SIGNAL(recordChosen(unsigned int)), this, SLOT(recordChosenSlot(unsigned int)));
	connect(displayWidget, SIGNAL(defocusRequested()), this, SLOT(displayWidgetDefocusRequested()));
	connect(displayWidget, SIGNAL(gotoPreviousPartRequested()), this, SLOT(gotoPreviousPart()));
	connect(displayWidget, SIGNAL(gotoNextPartRequested()), this, SLOT(gotoNextPart()));
	detailLayout->addWidget(displayWidget);


	displaySplitter->setStretchFactor(0, 3);
	displaySplitter->setStretchFactor(1, 1);

    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);


    QSettings s;

    s.beginGroup(QString("gui_geometry_%1").arg(partCat->getTableName()));

    mainSplitter->restoreState(s.value("pcw_main_splitter_state").toByteArray());
    displaySplitter->restoreState(s.value("pcw_display_splitter_state").toByteArray());

    s.endGroup();


    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));
    connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)));

    populateDatabaseDependentUI();
}


void PartCategoryWidget::setDisplayFlags(flags_t flags)
{
	displayFlags = flags;

	displayWidget->setFlags(flags);

	if ((flags & ChoosePart)  !=  0) {
		recordAddButton->hide();
		recordRemoveButton->hide();
		recordDuplicateButton->hide();
	} else {
		recordAddButton->show();
		recordRemoveButton->show();
		recordDuplicateButton->show();
	}
}


void PartCategoryWidget::aboutToQuit()
{
	QSettings s;

	s.beginGroup(QString("gui_geometry_%1").arg(partCat->getTableName()));

	s.setValue("pcw_main_splitter_state", mainSplitter->saveState());
	s.setValue("pcw_display_splitter_state", displaySplitter->saveState());

	s.endGroup();
}


void PartCategoryWidget::filterApplied()
{
	filterWidget->readFilter(filter);

	try {
		rebuildListTable();
	} catch (SQLException& ex) {
		QMessageBox::critical(this, tr("SQL Error"),
				tr("SQL Error caught while rebuilding part listing table:\n\n%1").arg(ex.what()));
	}
}


void PartCategoryWidget::listingCurrentPartChanged(unsigned int id)
{
	if (displayWidget) {
		displayWidget->saveChanges();
		displayWidget->setDisplayedPart(id);
	}
}


void PartCategoryWidget::listingPartActivated(unsigned int id)
{
	if ((displayFlags & ChoosePart)  !=  0) {
		emit recordChosen(id);
	} else {
		displayWidget->focusAuto();
	}
}


void PartCategoryWidget::listingAddPartRequested()
{
	recordAddRequested();
}


void PartCategoryWidget::listingDeletePartsRequested(const QList<unsigned int>& ids)
{
	partCat->removeRecords(ids);
}


void PartCategoryWidget::listingDuplicatePartsRequested(const QList<unsigned int>& ids)
{
	unsigned int oldCurIdx = listingTable->getCurrentPart();
	QList<unsigned int> newIDs = partCat->duplicateRecords(ids);

	listingTable->selectionModel()->clearSelection();

	int oldCurIdxNum = ids.indexOf(oldCurIdx);
	if (oldCurIdxNum != -1) {
		// The current part was duplicated. Make its duplicate the new current part.
		unsigned int newID = newIDs[oldCurIdxNum];
		listingTable->setCurrentPart(newID);
	}

	listingTable->selectParts(newIDs);
}


void PartCategoryWidget::rebuildListTable()
{
	try {
		listingTable->updateData();
	} catch (SQLException& ex) {
		fprintf(stderr, "Caught one\n");
		QMessageBox::critical(this, tr("Error During Filtering"),
				tr("SQL Error caught during filter execution. Check your user-defined SQL filtering "
						"code for errors.\n\n%1").arg(ex.what()));
		fprintf(stderr, "Caught two\n");
	}
}


void PartCategoryWidget::databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
	System* sys = System::getInstance();

	if (sys->isEmergencyDisconnecting()) {
		return;
	}

	try {
		populateDatabaseDependentUI();

		if (newConn) {
			rebuildListTable();
		} else if (!newConn  &&  oldConn) {
			rebuildListTable();
		}
	} catch (SQLException& ex) {
		QMessageBox::critical(this, tr("SQL Error"),
				tr("An SQL error was caught while rebuilding the part listing table:\n\n%1").arg(ex.what()));
		sys->emergencyDisconnect();
		return;
	}
}


void PartCategoryWidget::recordAddRequested()
{
	unsigned int id = partCat->createRecord();
	QModelIndex idx = listingTableModel->getIndexWithID(id);
	listingTable->setCurrentIndex(idx);
}


void PartCategoryWidget::recordRemoveRequested()
{
	QModelIndexList sel = listingTable->selectionModel()->selectedRows();

	int lowestRow = -1;
	QList<unsigned int> ids;

	for (QModelIndexList::iterator it = sel.begin() ; it != sel.end() ; it++) {
		QModelIndex idx = *it;

		int row = idx.row();

		if (lowestRow == -1  ||  row < lowestRow)
			lowestRow = row;

		unsigned int id = listingTableModel->getIndexID(idx);
		ids << id;
	}

	partCat->removeRecords(ids);

	QModelIndex cidx = listingTableModel->index(lowestRow, 0);

	if (!cidx.isValid()  &&  lowestRow != 0) {
		cidx = listingTableModel->index(lowestRow-1, 0);
	}

	listingTable->setCurrentIndex(cidx);
}


void PartCategoryWidget::recordDuplicateRequested()
{
	listingDuplicatePartsRequested(listingTable->getSelectedPartIDs());
}


void PartCategoryWidget::jumpToPart(unsigned int id)
{
	QItemSelectionModel* smodel = listingTable->selectionModel();

	QModelIndex idx = listingTableModel->getIndexWithID(id);

	listingTable->setCurrentIndex(idx);

	displayWidget->saveChanges();
	displayWidget->setDisplayedPart(id);

	listingTable->scrollTo(idx, ListingTable::PositionAtCenter);
}


void PartCategoryWidget::recordChosenSlot(unsigned int id)
{
	emit recordChosen(id);
}


void PartCategoryWidget::gotoPreviousPart()
{
	QModelIndex cidx = listingTable->currentIndex();

	int rc = listingTableModel->rowCount();

	if (rc != 0) {
		if (!cidx.isValid()) {
			cidx = listingTableModel->index(listingTableModel->rowCount()-1, 0);
		} else {
			if (cidx.row() != 0) {
				cidx = listingTableModel->index(cidx.row()-1, 0);
			}
		}
	}

	listingTable->setCurrentIndex(cidx);

	displayWidget->focusAuto();
}


void PartCategoryWidget::gotoNextPart()
{
	QModelIndex cidx = listingTable->currentIndex();

	if (!cidx.isValid()) {
		cidx = listingTableModel->index(0, 0);
	} else {
		QModelIndex nextIdx = listingTableModel->index(cidx.row()+1, 0);

		if (nextIdx.isValid())
			cidx = nextIdx;
	}

	listingTable->setCurrentIndex(cidx);

	displayWidget->focusAuto();
}


void PartCategoryWidget::displayWidgetDefocusRequested()
{
	listingTable->setFocus();
}


void PartCategoryWidget::updateButtonStates()
{
	System* sys = System::getInstance();

	if (recordAddButton) {
		bool hasSel = listingTable->selectionModel()->hasSelection();

		recordAddButton->setEnabled(sys->hasValidDatabaseConnection());
		recordRemoveButton->setEnabled(sys->hasValidDatabaseConnection()  &&  hasSel);
		recordDuplicateButton->setEnabled(sys->hasValidDatabaseConnection()  &&  hasSel);
	}
}


void PartCategoryWidget::populateDatabaseDependentUI()
{
	updateButtonStates();
}


void PartCategoryWidget::listingTableSelectionChanged(const QList<unsigned int>& ids)
{
	updateButtonStates();
}
