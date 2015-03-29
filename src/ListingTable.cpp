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

#include "ListingTable.h"
#include "PartTableModel.h"
#include "System.h"
#include <QtGui/QHeaderView>
#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QDataStream>



ListingTable::ListingTable(PartCategory* partCat, Filter* filter, QWidget* parent)
		: QTableView(parent), partCat(partCat), model(new PartTableModel(partCat, filter, this)), ignoreSectionResize(false)
{
	setModel(model);

	QHeaderView* hheader = horizontalHeader();

	verticalHeader()->hide();

	hheader->setStretchLastSection(true);
    hheader->setCascadingSectionResizes(false);
    hheader->setMovable(true);


    setSelectionBehavior(SelectRows);

    setDragEnabled(true);
    setDragDropMode(DragOnly);

    setAutoScroll(true);
    setSortingEnabled(true);

    sortByColumn(0, Qt::AscendingOrder);

    setContextMenuPolicy(Qt::CustomContextMenu);
    hheader->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(partActivatedSlot(const QModelIndex&)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(contextMenuRequested(const QPoint&)));
	connect(hheader, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(headerContextMenuRequested(const QPoint&)));


	System* sys = System::getInstance();
	EditStack* editStack = sys->getEditStack();

	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

	connect(partCat, SIGNAL(recordEdited(unsigned int, PartCategory::DataMap)), this, SLOT(updateData()));
	connect(partCat, SIGNAL(recordCreated(unsigned int, PartCategory::DataMap)), this, SLOT(updateData()));
	connect(partCat, SIGNAL(recordsRemoved(QList<unsigned int>)), this, SLOT(updateData()));

	connect(editStack, SIGNAL(undone(EditCommand*)), this, SLOT(updateData()));
	connect(editStack, SIGNAL(redone(EditCommand*)), this, SLOT(updateData()));

	connect(selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
			this, SLOT(currentChanged(const QModelIndex&, const QModelIndex&)));
	connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    		this, SLOT(selectionChangedSlot(const QItemSelection&, const QItemSelection&)));
	connect(model, SIGNAL(updateRequested()), this, SLOT(updateData()));


	QSettings s;

	s.beginGroup(QString("gui_geometry_%1").arg(partCat->getTableName()));

	if (!s.contains("lt_state")  ||  !restoreState(s.value("lt_state").toByteArray())) {
		unsigned initialTotalWidth = 1000;
		int nc = model->columnCount();

		for (unsigned int i = 0 ; i < nc-1 ; i++) {
			setColumnWidth(i, initialTotalWidth / nc);
		}
	}

	s.endGroup();
}


void ListingTable::aboutToQuit()
{
	QSettings s;

	s.beginGroup(QString("gui_geometry_%1").arg(partCat->getTableName()));

	s.setValue("lt_state", saveState());

	s.endGroup();
}


void ListingTable::keyPressEvent(QKeyEvent* evt)
{
	int k = evt->key();

	if (k == Qt::Key_Enter  ||  k == Qt::Key_Return) {
		emit partActivated(model->getIndexID(currentIndex()));
		evt->accept();
	} else {
		QTableView::keyPressEvent(evt);
	}
}


void ListingTable::currentChanged(const QModelIndex& newIdx, const QModelIndex& oldIdx)
{
	scrollTo(newIdx, EnsureVisible);
	emit currentPartChanged(model->getIndexID(newIdx));
}


void ListingTable::selectionChangedSlot(const QItemSelection&, const QItemSelection&)
{
	emit selectedPartsChanged(getSelectedPartIDs());
}


void ListingTable::partActivatedSlot(const QModelIndex& idx)
{
	emit partActivated(model->getIndexID(idx));
}


void ListingTable::updateData()
{
	QModelIndexList sel = selectionModel()->selectedRows();
	QList<unsigned int> selIds;

	for (QModelIndexList::iterator it = sel.begin() ; it != sel.end() ; it++) {
		QModelIndex idx = *it;
		selIds << model->getIndexID(idx);
	}

	QModelIndex cidx = currentIndex();
	unsigned int cidxId = model->getIndexID(cidx);

	model->updateData();

	QItemSelection newSel;

	unsigned int ns = 0;

	for (QList<unsigned int>::iterator it = selIds.begin() ; it != selIds.end() ; it++) {
		unsigned int id = *it;
		QModelIndex idx = model->getIndexWithID(id);

		if (idx.isValid()) {
			newSel.select(idx, model->index(idx.row(), model->columnCount()-1));
			ns++;
		}
	}

	selectionModel()->select(newSel, QItemSelectionModel::Select);

	cidx = model->getIndexWithID(cidxId);
	setCurrentIndex(cidx);

	if (!cidx.isValid()) {
		// There is no current part. One way this could happen is if parts were previously selected, but they were all
		// removed. In this case, setCurrentIndex(cidx) above was a no-op because the model was previously reset which
		// clears any current part selection WITHOUT EMITTING THE currentRowChanged SIGNAL! In this case, we have to emit
		// currentPartChanged() manually.
		emit currentPartChanged(INVALID_PART_ID);
	}

	if (!selectionModel()->hasSelection()) {
		// Same as above, but for selection.
		emit selectedPartsChanged(QList<unsigned int>());
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
	connect(scaleToFitAction, SIGNAL(triggered()), this, SLOT(scaleSectionsToFit()));
	menu->addAction(scaleToFitAction);

	menu->addSeparator();

	int nc = model->columnCount();

	for (int i = 0 ; i < nc ; i++) {
		QString headerStr = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();

		QAction* action = new QAction(headerStr, menu);
		connect(action, SIGNAL(triggered()), this, SLOT(headerSectionVisibilityActionTriggered()));
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
	connect(addAction, SIGNAL(triggered()), this, SLOT(addPartRequestedSlot()));
	menu.addAction(addAction);

	QAction* removeAction = new QAction(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.png")),
			tr("Delete Parts"), &menu);
	connect(removeAction, SIGNAL(triggered()), this, SLOT(deletePartsRequestedSlot()));
	menu.addAction(removeAction);

	QAction* duplicateAction = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/edit-copy.png")),
			tr("Duplicate Parts"), &menu);
	connect(duplicateAction, SIGNAL(triggered()), this, SLOT(duplicatePartsRequestedSlot()));
	menu.addAction(duplicateAction);

	if (selIndices.empty()) {
		removeAction->setEnabled(false);
		duplicateAction->setEnabled(false);
	}

	menu.addSeparator();

	QMenu* headerMenu = new QMenu(tr("Sections"), &menu);
	buildHeaderSectionMenu(headerMenu);
	menu.addMenu(headerMenu);

	menu.exec(mapToGlobal(pos));
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
	emit deletePartsRequested(getSelectedPartIDs());
}


void ListingTable::duplicatePartsRequestedSlot()
{
	emit duplicatePartsRequested(getSelectedPartIDs());
}


QList<unsigned int> ListingTable::getSelectedPartIDs() const
{
	QList<unsigned int> partIds;

	QModelIndexList selIndices = selectionModel()->selectedRows();
	for (QModelIndex idx : selIndices) {
		partIds << model->getIndexID(idx);
	}

	return partIds;
}


void ListingTable::setCurrentPart(unsigned int id)
{
	setCurrentIndex(model->getIndexWithID(id));
}


void ListingTable::setSelectedParts(const QList<unsigned int>& ids)
{
	clearSelection();
	selectParts(ids);
}


void ListingTable::selectParts(const QList<unsigned int>& ids)
{
	QItemSelection sel;

	for (unsigned int id : ids) {
		QModelIndex idx = model->getIndexWithID(id);
		sel.select(idx, model->index(idx.row(), model->columnCount()-1));
	}

	selectionModel()->select(sel, QItemSelectionModel::Select);
}


unsigned int ListingTable::getCurrentPart() const
{
	return model->getIndexID(currentIndex());
}


