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

#include "ListingTable.h"

#include <QAction>
#include <QApplication>
#include <QDataStream>
#include <QHeaderView>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include "../../model/part/PartFactory.h"
#include "../../System.h"
#include "PartTableModel.h"

namespace electronicsdb
{


ListingTable::ListingTable(PartCategory* partCat, Filter* filter, QWidget* parent)
        : QTableView(parent), partCat(partCat), model(new PartTableModel(partCat, filter, this)), ignoreSectionResize(false)
{
    setModel(model);

    QHeaderView* hheader = horizontalHeader();

    verticalHeader()->hide();

    hheader->setStretchLastSection(true);
    hheader->setCascadingSectionResizes(false);
    hheader->setSectionsMovable(true);


    setSelectionBehavior(SelectRows);

    setDragEnabled(true);
    setDragDropMode(DragOnly);

    setAutoScroll(true);
    setSortingEnabled(true);

    sortByColumn(0, Qt::AscendingOrder);

    setContextMenuPolicy(Qt::CustomContextMenu);
    hheader->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &ListingTable::doubleClicked, this, &ListingTable::partActivatedSlot);
    connect(this, &ListingTable::customContextMenuRequested, this, &ListingTable::contextMenuRequested);
    connect(hheader, &QHeaderView::customContextMenuRequested, this, &ListingTable::headerContextMenuRequested);


    System* sys = System::getInstance();
    EditStack* editStack = sys->getEditStack();

    connect(qApp, &QApplication::aboutToQuit, this, &ListingTable::aboutToQuit);

    connect(&PartFactory::getInstance(), &PartFactory::partsChanged, this, &ListingTable::updateData);

    connect(selectionModel(), &QItemSelectionModel::currentRowChanged, this, &ListingTable::currentChanged);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ListingTable::selectionChangedSlot);
    connect(model, &PartTableModel::updateRequested, this, &ListingTable::updateData);


    QSettings s;

    s.beginGroup(QString("gui/listing_table/%1").arg(partCat->getTableName()));

    if (!s.contains("state")  ||  !restoreState(s.value("state").toByteArray())) {
        unsigned initialTotalWidth = 1000;
        int nc = model->columnCount();

        for (int i = 0 ; i < nc-1 ; i++) {
            setColumnWidth(i, initialTotalWidth / nc);
        }
    }

    s.endGroup();
}


void ListingTable::aboutToQuit()
{
    QSettings s;

    s.beginGroup(QString("gui/listing_table/%1").arg(partCat->getTableName()));

    s.setValue("state", saveState());

    s.endGroup();
}


void ListingTable::keyPressEvent(QKeyEvent* evt)
{
    int k = evt->key();

    if (k == Qt::Key_Enter  ||  k == Qt::Key_Return) {
        emit partActivated(model->getIndexPart(currentIndex()));
        evt->accept();
    } else if (k == Qt::Key_Delete) {
        emit deletePartsRequested(getSelectedParts());
    } else {
        QTableView::keyPressEvent(evt);
    }
}


void ListingTable::currentChanged(const QModelIndex& newIdx, const QModelIndex& oldIdx)
{
    scrollTo(newIdx, EnsureVisible);
    emit currentPartChanged(model->getIndexPart(newIdx));
}


void ListingTable::selectionChangedSlot(const QItemSelection&, const QItemSelection&)
{
    emit selectedPartsChanged(getSelectedParts());
}


void ListingTable::partActivatedSlot(const QModelIndex& idx)
{
    emit partActivated(model->getIndexPart(idx));
}


void ListingTable::updateData()
{
    QModelIndexList sel = selectionModel()->selectedRows();
    PartList selParts;
    for (QModelIndex idx : sel) {
        selParts << model->getIndexPart(idx);
    }

    QModelIndex cidx = currentIndex();
    Part cidxPart = model->getIndexPart(cidx);

    model->updateData();

    QItemSelection newSel;
    unsigned int ns = 0;
    for (const Part& part : selParts) {
        QModelIndex idx = model->getIndexWithPart(part);
        if (idx.isValid()) {
            newSel.select(idx, model->index(idx.row(), model->columnCount()-1));
            ns++;
        }
    }

    selectionModel()->select(newSel, QItemSelectionModel::Select);

    cidx = model->getIndexWithPart(cidxPart);
    setCurrentIndex(cidx);

    if (!cidx.isValid()) {
        // There is no current part. One way this could happen is if parts were previously selected, but they were all
        // removed. In this case, setCurrentIndex(cidx) above was a no-op because the model was previously reset which
        // clears any current part selection WITHOUT EMITTING THE currentRowChanged SIGNAL! In this case, we have to emit
        // currentPartChanged() manually.
        emit currentPartChanged(Part());
    }

    if (!selectionModel()->hasSelection()) {
        // Same as above, but for selection.
        emit selectedPartsChanged(PartList());
    }
}


void ListingTable::scaleSectionsToFit()
{
    int w = width();

    int totalWidth = 0;
    QList<int> oldWidths;

    int nc = model->columnCount();

    for (int i = 0 ; i < nc ; i++) {
        int cw = columnWidth(i);
        oldWidths << cw;
        totalWidth += cw;
    }

    for (int i = 0 ; i < nc-1 ; i++) {
        float rw = (float) oldWidths[i] / (float) totalWidth;
        setColumnWidth(i, rw*w);
    }
}


void ListingTable::buildHeaderSectionMenu(QMenu* menu)
{
    QHeaderView* header = horizontalHeader();

    QAction* scaleToFitAction = new QAction(tr("Scale To Fit Viewport"), menu);
    connect(scaleToFitAction, &QAction::triggered, this, &ListingTable::scaleSectionsToFit);
    menu->addAction(scaleToFitAction);

    menu->addSeparator();

    int nc = model->columnCount();

    for (int i = 0 ; i < nc ; i++) {
        QString headerStr = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();

        QAction* action = new QAction(headerStr, menu);
        connect(action, &QAction::triggered, this, &ListingTable::headerSectionVisibilityActionTriggered);
        action->setData(i);
        action->setCheckable(true);
        action->setChecked(!header->isSectionHidden(i));
        menu->addAction(action);
    }
}


void ListingTable::contextMenuRequested(const QPoint& pos)
{
    QMenu menu;

    QModelIndexList selIndices = selectionModel()->selectedRows();

    QAction* addAction = new QAction(QIcon::fromTheme("document-new", QIcon(":/icons/document-new.png")),
            tr("Add Part"), &menu);
    connect(addAction, &QAction::triggered, this, &ListingTable::addPartRequestedSlot);
    menu.addAction(addAction);

    QAction* removeAction = new QAction(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.png")),
            tr("Delete Parts"), &menu);
    connect(removeAction, &QAction::triggered, this, &ListingTable::deletePartsRequestedSlot);
    menu.addAction(removeAction);

    QAction* duplicateAction = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/edit-copy.png")),
            tr("Duplicate Parts"), &menu);
    connect(duplicateAction, &QAction::triggered, this, &ListingTable::duplicatePartsRequestedSlot);
    menu.addAction(duplicateAction);

    if (selIndices.empty()) {
        removeAction->setEnabled(false);
        duplicateAction->setEnabled(false);
    }

    menu.addSeparator();

    QMenu* headerMenu = new QMenu(tr("Sections"), &menu);
    buildHeaderSectionMenu(headerMenu);
    menu.addMenu(headerMenu);

    menu.exec(viewport()->mapToGlobal(pos));
}


void ListingTable::headerContextMenuRequested(const QPoint& pos)
{
    QHeaderView* header = horizontalHeader();

    QMenu menu;

    buildHeaderSectionMenu(&menu);

    menu.exec(header->mapToGlobal(pos));
}


void ListingTable::headerSectionVisibilityActionTriggered()
{
    QAction* action = (QAction*) sender();
    int sect = action->data().toInt();

    QHeaderView* header = horizontalHeader();
    header->setSectionHidden(sect, !action->isChecked());
}


QByteArray ListingTable::saveState()
{
    QHeaderView* header = horizontalHeader();

    return header->saveState();
}


bool ListingTable::restoreState(const QByteArray& state)
{
    QHeaderView* header = horizontalHeader();

    return header->restoreState(state);
}


void ListingTable::addPartRequestedSlot()
{
    emit addPartRequested();
}


void ListingTable::deletePartsRequestedSlot()
{
    emit deletePartsRequested(getSelectedParts());
}


void ListingTable::duplicatePartsRequestedSlot()
{
    emit duplicatePartsRequested(getSelectedParts());
}


PartList ListingTable::getSelectedParts() const
{
    PartList parts;

    QModelIndexList selIndices = selectionModel()->selectedRows();
    for (QModelIndex idx : selIndices) {
        parts << model->getIndexPart(idx);
    }

    return parts;
}


void ListingTable::setCurrentPart(const Part& part)
{
    setCurrentIndex(model->getIndexWithPart(part));
}


void ListingTable::setSelectedParts(const PartList& parts)
{
    clearSelection();
    selectParts(parts);
}


void ListingTable::selectParts(const PartList& parts)
{
    QItemSelection sel;

    for (const Part& part : parts) {
        QModelIndex idx = model->getIndexWithPart(part);
        sel.select(idx, model->index(idx.row(), model->columnCount()-1));
    }

    selectionModel()->select(sel, QItemSelectionModel::Select);
}


Part ListingTable::getCurrentPart() const
{
    return model->getIndexPart(currentIndex());
}


}
