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

#include "PartCategoryWidget.h"

#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>
#include "../../exception/SQLException.h"
#include "../../model/part/PartFactory.h"
#include "../../System.h"
#include "../GUIStatePersister.h"

namespace electronicsdb
{


PartCategoryWidget::PartCategoryWidget(PartCategory* partCat, QWidget* parent)
        : QWidget(parent), partCat(partCat), filter(new Filter), displayFlags(0), displayWidget(nullptr),
          recordAddButton(nullptr), recordRemoveButton(nullptr), recordDuplicateButton(nullptr)
{
    System* sys = System::getInstance();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    QMargins cm = topLayout->contentsMargins();
    topLayout->setContentsMargins(cm.left(), cm.top(), cm.right(), 0);
    setLayout(topLayout);

    mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setObjectName("mainSplitter");
    topLayout->addWidget(mainSplitter);


    QGroupBox* filterContWidget = new QGroupBox(tr("Filter"), mainSplitter);
    mainSplitter->addWidget(filterContWidget);

    QVBoxLayout* filterContLayout = new QVBoxLayout(filterContWidget);
    filterContWidget->setLayout(filterContLayout);

    filterWidget = new FilterWidget(mainSplitter, partCat);
    connect(filterWidget, &FilterWidget::applied, this, &PartCategoryWidget::filterApplied);
    filterContLayout->addWidget(filterWidget);


    //QWidget* displayContWidget = new QWidget(mainSplitter);
    QGroupBox* displayContWidget = new QGroupBox(tr("Parts"), mainSplitter);
    mainSplitter->addWidget(displayContWidget);

    QHBoxLayout* displayContLayout = new QHBoxLayout(displayContWidget);
    cm = displayContLayout->contentsMargins();
    displayContLayout->setContentsMargins(cm.left(), cm.top(), cm.right(), 0);
    displayContLayout->setContentsMargins(0, 0, 0, 0);
    displayContWidget->setLayout(displayContLayout);


    displaySplitter = new QSplitter(Qt::Horizontal, displayContWidget);
    displaySplitter->setObjectName("displaySplitter");
    displayContLayout->addWidget(displaySplitter);

    QWidget* listingWidget = new QWidget(displaySplitter);
    displaySplitter->addWidget(listingWidget);

    QVBoxLayout* listingLayout = new QVBoxLayout(listingWidget);
    listingWidget->setLayout(listingLayout);

    listingTable = new ListingTable(partCat, filter, listingWidget);
    connect(listingTable, &ListingTable::currentPartChanged, this, &PartCategoryWidget::listingCurrentPartChanged);
    connect(listingTable, &ListingTable::partActivated, this, &PartCategoryWidget::listingPartActivated);
    connect(listingTable, &ListingTable::deletePartsRequested, this, &PartCategoryWidget::listingDeletePartsRequested);
    connect(listingTable, &ListingTable::addPartRequested, this, &PartCategoryWidget::listingAddPartRequested);
    connect(listingTable, &ListingTable::duplicatePartsRequested,
            this, &PartCategoryWidget::listingDuplicatePartsRequested);
    connect(listingTable, &ListingTable::selectedPartsChanged, this, &PartCategoryWidget::listingTableSelectionChanged);

    listingLayout->addWidget(listingTable);
    listingTable->updateData();

    listingTableModel = listingTable->getModel();


    QWidget* listingButtonWidget = new QWidget(listingWidget);
    listingLayout->addWidget(listingButtonWidget);

    QHBoxLayout* listingButtonLayout = new QHBoxLayout(listingButtonWidget);
    listingButtonWidget->setLayout(listingButtonLayout);

    listingButtonLayout->addStretch(1);

    recordAddButton = new QPushButton(QIcon::fromTheme("document-new", QIcon(":/icons/document-new.png")),
            "", listingButtonWidget);
    connect(recordAddButton, &QPushButton::clicked, this, &PartCategoryWidget::recordAddRequested);
    listingButtonLayout->addWidget(recordAddButton);

    recordRemoveButton = new QPushButton(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.png")),
            "", listingButtonWidget);
    recordRemoveButton->setEnabled(false);
    connect(recordRemoveButton, &QPushButton::clicked, this, &PartCategoryWidget::recordRemoveRequested);
    listingButtonLayout->addWidget(recordRemoveButton);

    recordDuplicateButton = new QPushButton(QIcon::fromTheme("edit-copy", QIcon(":/icons/edit-copy.png")),
            "", listingButtonWidget);
    recordDuplicateButton->setEnabled(false);
    connect(recordDuplicateButton, &QPushButton::clicked, this, &PartCategoryWidget::recordDuplicateRequested);
    listingButtonLayout->addWidget(recordDuplicateButton);

    listingButtonLayout->addStretch(1);


    //QGroupBox* detailWidget = new QGroupBox(tr("Part Details"), displaySplitter);
    QWidget* detailWidget = new QWidget(displaySplitter);
    displaySplitter->addWidget(detailWidget);

    QVBoxLayout* detailLayout = new QVBoxLayout(detailWidget);
    cm = detailLayout->contentsMargins();
    detailLayout->setContentsMargins(cm.left(), cm.top(), cm.right(), 0);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailWidget->setLayout(detailLayout);

    displayWidget = new PartDisplayWidget(partCat, detailWidget);
    connect(displayWidget, &PartDisplayWidget::partChosen, this, &PartCategoryWidget::partChosenSlot);
    connect(displayWidget, &PartDisplayWidget::defocusRequested, this, &PartCategoryWidget::displayWidgetDefocusRequested);
    connect(displayWidget, &PartDisplayWidget::gotoPreviousPartRequested, this, &PartCategoryWidget::gotoPreviousPart);
    connect(displayWidget, &PartDisplayWidget::gotoNextPartRequested, this, &PartCategoryWidget::gotoNextPart);
    detailLayout->addWidget(displayWidget);


    displaySplitter->setStretchFactor(0, 3);
    displaySplitter->setStretchFactor(1, 1);

    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);


    QString cfgGroup = QString("gui/part_category_widget/%1").arg(partCat->getID());

    GUIStatePersister::getInstance().registerSplitter(cfgGroup, mainSplitter);
    GUIStatePersister::getInstance().registerSplitter(cfgGroup, displaySplitter);

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


PartList PartCategoryWidget::getSelectedParts() const
{
    return listingTable->getSelectedParts();
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


void PartCategoryWidget::listingCurrentPartChanged(const Part& part)
{
    if (displayWidget) {
        displayWidget->saveChanges();
        displayWidget->setDisplayedPart(part);
    }
}


void PartCategoryWidget::listingPartActivated(const Part& part)
{
    if ((displayFlags & ChoosePart)  !=  0) {
        emit partChosen(part);
    } else {
        displayWidget->focusAuto();
    }
}


void PartCategoryWidget::listingAddPartRequested()
{
    recordAddRequested();
}


void PartCategoryWidget::listingDeletePartsRequested(const PartList& parts)
{
    PartFactory::getInstance().deleteItems(parts.begin(), parts.end());
}


void PartCategoryWidget::listingDuplicatePartsRequested(const PartList& parts)
{
    Part oldCurPart = listingTable->getCurrentPart();

    // Load the items to be duplicated fully, otherwise non-loaded data won't be cloned.
    PartList protoParts = parts;
    PartFactory::getInstance().loadItems(protoParts.begin(), protoParts.end(), PartFactory::LoadAll);

    PartList newParts;
    for (const Part& part : protoParts) {
        Part newPart = part.clone();
        QList<PartContainer> dummyList;
        newPart.loadContainers(dummyList.begin(), dummyList.end()); // Don't put the duplicates into containers initially
        newParts << newPart;
    }

    PartFactory::getInstance().insertItems(newParts.begin(), newParts.end());
    for (const Part& part : newParts) {
        assert(part.hasID());
    }

    listingTable->selectionModel()->clearSelection();

    int oldCurIdxNum = parts.indexOf(oldCurPart);
    if (oldCurIdxNum != -1) {
        // The current part was duplicated. Make its duplicate the new current part.
        Part newPart = newParts[oldCurIdxNum];
        listingTable->setCurrentPart(newPart);
    }

    listingTable->selectParts(newParts);
}


void PartCategoryWidget::rebuildListTable()
{
    try {
        listingTable->updateData();
    } catch (SQLException& ex) {
        QMessageBox::critical(this, tr("Error During Filtering"),
                tr("SQL Error caught during filter execution. Check your user-defined SQL filtering "
                        "code for errors.\n\n%1").arg(ex.what()));
    }
}


void PartCategoryWidget::recordAddRequested()
{
    Part newPart(partCat, Part::CreateBlankTag());
    PartFactory::getInstance().insertItem(newPart);
    assert(newPart.hasID());
    QModelIndex idx = listingTableModel->getIndexWithPart(newPart);
    listingTable->setCurrentIndex(idx);
}


void PartCategoryWidget::recordRemoveRequested()
{
    QModelIndexList sel = listingTable->selectionModel()->selectedRows();

    int lowestRow = -1;
    PartList parts;

    for (QModelIndex idx : sel) {
        int row = idx.row();

        if (lowestRow == -1  ||  row < lowestRow)
            lowestRow = row;

        parts << listingTableModel->getIndexPart(idx);
    }

    PartFactory::getInstance().deleteItems(parts.begin(), parts.end());

    QModelIndex cidx = listingTableModel->index(lowestRow, 0);

    if (!cidx.isValid()  &&  lowestRow != 0) {
        cidx = listingTableModel->index(lowestRow-1, 0);
    }

    listingTable->setCurrentIndex(cidx);
}


void PartCategoryWidget::recordDuplicateRequested()
{
    listingDuplicatePartsRequested(listingTable->getSelectedParts());
}


void PartCategoryWidget::jumpToPart(const Part& part)
{
    QModelIndex idx = listingTableModel->getIndexWithPart(part);

    listingTable->setCurrentIndex(idx);

    displayWidget->saveChanges();
    displayWidget->setDisplayedPart(part);

    listingTable->scrollTo(idx, ListingTable::PositionAtCenter);
}


void PartCategoryWidget::partChosenSlot(const Part& part)
{
    emit partChosen(part);
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


void PartCategoryWidget::listingTableSelectionChanged(const PartList& parts)
{
    updateButtonStates();
}


}
