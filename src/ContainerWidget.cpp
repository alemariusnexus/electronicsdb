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

#include "ContainerWidget.h"
#include "System.h"
#include "NoDatabaseConnectionException.h"
#include <nxcommon/exception/InvalidValueException.h>
#include "ContainerEditCommand.h"
#include "SQLInsertCommand.h"
#include "SQLDeleteCommand.h"
#include "SQLMultiValueInsertCommand.h"
#include "CompoundEditCommand.h"
#include "ContainerTableItemDelegate.h"
#include <QtCore/QSettings>
#include <QtGui/QMenu>



ContainerEditCommand* ContainerWidget::lastDragCommand = NULL;




ContainerWidget::ContainerWidget(QWidget* parent)
		: QWidget(parent), currentID(INVALID_PART_ID)
{
	ui.setupUi(this);

	System* sys = System::getInstance();


	ui.contTableView->setItemDelegate(new ContainerTableItemDelegate(ui.contTableView, this));
	contTableModel = new ContainerTableModel(ui.contTableView);
	ui.contTableView->setModel(contTableModel);

	ui.contTableView->setAutoScroll(true);
	ui.contTableView->setSelectionMode(QTableView::ExtendedSelection);
	ui.contTableView->setSelectionBehavior(QTableView::SelectRows);

	ui.contTableView->horizontalHeader()->resizeSection(0, 100);


	partTableModel = new ContainerPartTableModel(ui.partTableView);

	ui.partTableView->setDragDropMode(QListView::DragDrop);
	ui.partTableView->setDragEnabled(true);
	ui.partTableView->setAcceptDrops(true);
	ui.partTableView->setDefaultDropAction(Qt::MoveAction);
	ui.partTableView->setDropIndicatorShown(true);

	ui.partTableView->setContextMenuPolicy(Qt::CustomContextMenu);

	ui.partTableView->verticalHeader()->hide();
	ui.partTableView->horizontalHeader()->hide();
	ui.partTableView->horizontalHeader()->setStretchLastSection(true);

	ui.partTableView->setModel(partTableModel);


	ui.contRemoveButton->setEnabled(false);

	ui.contAddButton->setEnabled(sys->hasValidDatabaseConnection());

	ui.splitter->setStretchFactor(0, 1);
	ui.splitter->setStretchFactor(1, 0);


	ui.contTableView->installEventFilter(this);
	ui.partTableView->installEventFilter(this);


	removePartsAction = new QAction(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.png")),
			tr("Remove"), this);
	connect(removePartsAction, SIGNAL(triggered()), this, SLOT(removeSelectedPartsRequested()));



	connect(ui.contSearchButton, SIGNAL(clicked()), this, SLOT(searchRequested()));
	connect(ui.contIdField, SIGNAL(returnPressed()), this, SLOT(searchRequested()));
	connect(ui.contTableView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
			this, SLOT(currentContainerChanged(const QModelIndex&, const QModelIndex&)));
	connect(ui.contTableView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
			this, SLOT(containerSelectionChanged()));
	connect(ui.contAddButton, SIGNAL(clicked()), this, SLOT(containerAddRequested()));
	connect(ui.contRemoveButton, SIGNAL(clicked()), this, SLOT(containerRemoveRequested()));
	connect(ui.partTableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(partActivated(const QModelIndex&)));
	connect(ui.partTableView, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(partTableViewContextMenuRequested(const QPoint&)));
	connect(partTableModel, SIGNAL(partsDropped(ContainerPartTableModel::PartList)),
			this, SLOT(partsDropped(ContainerPartTableModel::PartList)));
	connect(partTableModel, SIGNAL(partsDragged(ContainerPartTableModel::PartList)),
			this, SLOT(partsDragged(ContainerPartTableModel::PartList)));


	EditStack* editStack = sys->getEditStack();

	connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)));
	connect(sys, SIGNAL(containersChanged()), this, SLOT(reload()));
	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));
	connect(editStack, SIGNAL(undone(EditCommand*)), this, SLOT(reload()));
	connect(editStack, SIGNAL(redone(EditCommand*)), this, SLOT(reload()));

	for (System::PartCategoryIterator it = sys->getPartCategoryBegin() ; it != sys->getPartCategoryEnd() ; it++) {
		PartCategory* cat = *it;

		connect(cat, SIGNAL(recordEdited(unsigned int, PartCategory::DataMap)), this, SLOT(reload()));
		connect(cat, SIGNAL(recordsRemoved(QList<unsigned int>)), this, SLOT(reload()));
	}


	reload();
}


void ContainerWidget::setConfigurationName(const QString& name)
{
	configName = name;

	QSettings s;

    s.beginGroup(QString("gui_geometry_cw_%1").arg(name));

    ui.splitter->restoreState(s.value("main_splitter_state").toByteArray());

    s.endGroup();
}


void ContainerWidget::aboutToQuit()
{
	QSettings s;

	s.beginGroup(QString("gui_geometry_cw_%1").arg(configName));

	s.setValue("main_splitter_state", ui.splitter->saveState());

	s.endGroup();
}


QString ContainerWidget::convertSearchPattern(const QString& str)
{
	QString strCpy(str);

	strCpy.replace('%', "\\%");
	strCpy.replace('_', "\\_");
	strCpy.replace('*', '%');
	strCpy.replace('?', '_');

	return strCpy;
}


void ContainerWidget::reload()
{
	bool focus = ui.contTableView->isAncestorOf(qApp->focusWidget());

	QModelIndexList sel = ui.contTableView->selectionModel()->selectedRows(0);

	QList<unsigned int> selIds;

	for (unsigned int i = 0 ; i < sel.size() ; i++) {
		QModelIndex idx = sel[i];

		unsigned int id = contTableModel->getIndexID(idx);
		selIds << id;
	}

	ui.contTableView->selectionModel()->blockSignals(true);
	contTableModel->reload();
	ui.contTableView->selectionModel()->blockSignals(false);
	displayContainer(currentID);

	QItemSelection selection;

	for (unsigned int i = 0 ; i < selIds.size() ; i++) {
		unsigned int id = selIds[i];

		QModelIndex idx = contTableModel->getIndexFromID(id);
		selection.select(idx, contTableModel->index(idx.row(), contTableModel->columnCount()-1));
	}

	ui.contTableView->setCurrentIndex(contTableModel->getIndexFromID(currentID));

	ui.contTableView->selectionModel()->select(selection, QItemSelectionModel::Select);

	if (focus) {
		ui.contTableView->setFocus();
	}

	containerSelectionChanged();
}


void ContainerWidget::displayContainer(unsigned int id)
{
	System* sys = System::getInstance();

	currentID = id;

	if (id == INVALID_CONTAINER_ID  ||  !sys->hasValidDatabaseConnection()) {
		partTableModel->display(INVALID_CONTAINER_ID, ContainerPartTableModel::PartList());
	} else {
		SQLDatabase sql = sys->getCurrentSQLDatabase();

		QString query = QString("SELECT ptype, pid FROM container_part WHERE cid=%1")
				.arg(id);

		SQLResult res = sql.sendQuery(query);

		QMap<PartCategory*, QList<unsigned int> > catIds;

		while (res.nextRecord()) {
			QString ptype = res.getString(0);
			unsigned int pid = res.getUInt32(1);

			PartCategory* cat = NULL;

			for (System::PartCategoryIterator it = sys->getPartCategoryBegin() ; it != sys->getPartCategoryEnd() ; it++) {
				PartCategory* c = *it;

				if (ptype == c->getTableName()) {
					cat = c;
					break;
				}
			}

			if (!cat) {
				throw InvalidValueException(QString("Invalid part category name: %1").arg(ptype).toUtf8().constData(),
					__FILE__, __LINE__);
			}

			catIds[cat] << pid;
		}

		ContainerPartTableModel::PartList parts;

		for (QMap<PartCategory*, QList<unsigned int> >::iterator it = catIds.begin() ; it != catIds.end() ; it++) {
			PartCategory* cat = it.key();
			QList<unsigned int> pids = it.value();

			QMap<unsigned int, QString> descs = cat->getDescriptions(pids);

			for (unsigned int i = 0 ; i < pids.size() ; i++) {
				unsigned int pid = pids[i];
				QString desc = descs[pid];

				ContainerPartTableModel::ContainerPart part;
				part.cat = cat;
				part.id = pid;
				part.desc = desc;

				parts << part;
			}
		}

		partTableModel->display(id, parts);
	}
}


void ContainerWidget::databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
	contTableModel->reload();

	if (oldConn  &&  !newConn) {
		ui.contAddButton->setEnabled(false);
	} else if (newConn  &&  !oldConn) {
		ui.contAddButton->setEnabled(true);
	}
}


void ContainerWidget::searchRequested()
{
	QString pattern = ui.contIdField->text();
	QString convPattern = convertSearchPattern(pattern);

	contTableModel->setFilter(convPattern);
}


void ContainerWidget::currentContainerChanged(const QModelIndex& newIdx, const QModelIndex& oldIdx)
{
	if (!newIdx.isValid()) {
		displayContainer(INVALID_CONTAINER_ID);
		return;
	}

	unsigned int contId = contTableModel->getIndexID(newIdx);
	displayContainer(contId);
}


void ContainerWidget::partActivated(const QModelIndex& idx)
{
	if (!idx.isValid())
		return;

	ContainerPartTableModel::ContainerPart part = partTableModel->getIndexPart(idx);

	PartCategory* cat = part.cat;
	unsigned int pid = part.id;

	System* sys = System::getInstance();

	sys->jumpToPart(cat, pid);
}


void ContainerWidget::containerAddRequested()
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection())
		return;

	ui.contTableView->setCurrentIndex(QModelIndex());

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	SQLResult res = sql.sendQuery (
			u"SELECT CAST(SUBSTR(name, 4) AS UNSIGNED INTEGER) AS num "
			u"FROM container "
			u"WHERE name LIKE 'NEW_%' "
			u"ORDER BY num DESC LIMIT 1"
			);

	unsigned int newNum = 1;

	if (res.nextRecord()) {
		newNum = res.getUInt32(0) + 1;
	}

	QString name = QString("NEW%1").arg(newNum);

	ContainerEditCommand* cmd = new ContainerEditCommand;

	SQLInsertCommand* insCmd = new SQLInsertCommand("container", "id");
	QMap<QString, QString> data;
	data["name"] = name;
	insCmd->addRecord(data);
	cmd->addSQLCommand(insCmd);

	EditStack* editStack = sys->getEditStack();

	editStack->push(cmd);

	unsigned int id = insCmd->getFirstInsertID();

	sys->signalContainersChanged();

	QModelIndex idx = contTableModel->getIndexFromID(id);

	ui.contTableView->selectionModel()->clearSelection();

	if (idx.isValid()) {
		ui.contTableView->setCurrentIndex(idx);
		ui.contTableView->scrollTo(idx);
		ui.contTableView->edit(idx);
	}
}


void ContainerWidget::containerRemoveRequested()
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection())
		return;

	QModelIndexList sel = ui.contTableView->selectionModel()->selectedIndexes();
	QList<unsigned int> ids;

	int lowestRow = -1;

	for (QModelIndexList::iterator it = sel.begin() ; it != sel.end() ; it++) {
		QModelIndex idx = *it;
		int row = idx.row();

		if (lowestRow == -1  ||  row < lowestRow)
			lowestRow = row;

		unsigned int id = contTableModel->getIndexID(idx);
		ids << id;
	}

	ContainerEditCommand* cmd = new ContainerEditCommand;

	SQLDeleteCommand* delCmd = new SQLDeleteCommand("container", "id");
	delCmd->addRecords(ids);
	cmd->addSQLCommand(delCmd);

	SQLDeleteCommand* partDelCmd = new SQLDeleteCommand("container_part", "cid");
	partDelCmd->addRecords(ids);
	cmd->addSQLCommand(partDelCmd);

	EditStack* editStack = sys->getEditStack();

	editStack->push(cmd);

	sys->signalContainersChanged();

	QModelIndex cidx = contTableModel->index(lowestRow, 0);

	if (!cidx.isValid()  &&  lowestRow != 0) {
		cidx = contTableModel->index(lowestRow-1, 0);
	}

	ui.contTableView->setCurrentIndex(cidx);
}


void ContainerWidget::containerSelectionChanged()
{
	bool hasSel = ui.contTableView->selectionModel()->hasSelection();

	ui.contRemoveButton->setEnabled(hasSel);
}


ContainerEditCommand* ContainerWidget::buildAddPartsCommand(unsigned int cid, ContainerPartTableModel::PartList parts)
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection())
		return NULL;

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	QString query = QString("SELECT ptype, pid FROM container_part WHERE cid='%1'")
			.arg(cid);

	QMap<QString, PartCategory*> catMap;

	for (System::PartCategoryIterator it = sys->getPartCategoryBegin() ; it != sys->getPartCategoryEnd() ; it++) {
		PartCategory* cat = *it;
		catMap[cat->getTableName()] = cat;
	}

	QList<ContainerPartTableModel::ContainerPart> insParts;

	SQLResult res = sql.sendQuery(query);

	while (res.nextRecord()) {
		QString ptype = res.getString(0);
		unsigned int id = res.getUInt32(1);

		PartCategory* cat = catMap[ptype];

		ContainerPartTableModel::ContainerPart part;
		part.cat = cat;
		part.id = id;
		insParts << part;
	}

	bool hasNewParts = false;

	for (unsigned int i = 0 ; i < parts.size() ; i++) {
		ContainerPartTableModel::ContainerPart& part = parts[i];

		bool isNew = true;
		for (unsigned int j = 0 ; j < insParts.size() ; j++) {
			ContainerPartTableModel::ContainerPart& insPart = insParts[j];

			if (insPart.cat == part.cat  &&  insPart.id == part.id) {
				isNew = false;
				break;
			}
		}

		if (!isNew)
			continue;

		insParts << part;
		hasNewParts = true;
	}

	if (!hasNewParts)
		return NULL;

	ContainerEditCommand* cmd = new ContainerEditCommand;

	SQLDeleteCommand* delCmd = new SQLDeleteCommand("container_part", "cid");
	delCmd->addRecord(cid);
	cmd->addSQLCommand(delCmd);

	SQLMultiValueInsertCommand* insCmd = new SQLMultiValueInsertCommand("container_part", "cid", cid);

	for (unsigned int i = 0 ; i < insParts.size() ; i++) {
		ContainerPartTableModel::ContainerPart& part = insParts[i];

		QMap<QString, QString> entry;
		entry["ptype"] = part.cat->getTableName();
		entry["pid"] = QString("%1").arg(part.id);
		insCmd->addValue(entry);
	}

	cmd->addSQLCommand(insCmd);

	return cmd;
}


ContainerEditCommand* ContainerWidget::buildRemovePartsCommand(unsigned int cid, ContainerPartTableModel::PartList parts)
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection())
		return NULL;

	QMap<QString, PartCategory*> catMap;

	for (System::PartCategoryIterator it = sys->getPartCategoryBegin() ; it != sys->getPartCategoryEnd() ; it++) {
		PartCategory* cat = *it;
		catMap[cat->getTableName()] = cat;
	}

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	QString query = QString("SELECT ptype, pid FROM container_part WHERE cid='%1'").arg(cid);

	SQLResult res = sql.sendQuery(query);

	SQLMultiValueInsertCommand* insCmd = new SQLMultiValueInsertCommand("container_part", "cid", cid);

	while (res.nextRecord()) {
		QString ptype = res.getString(0);
		unsigned int id = res.getUInt32(1);

		PartCategory* cat = catMap[ptype];

		bool removed = false;
		for (ContainerPartTableModel::PartIterator it = parts.begin() ; it != parts.end() ; it++) {
			ContainerPartTableModel::ContainerPart& part = *it;

			if (part.cat == cat  &&  part.id == id) {
				removed = true;
				break;
			}
		}

		if (!removed) {
			QMap<QString, QString> data;
			data["ptype"] = cat->getTableName();
			data["pid"] = QString("%1").arg(id);
			insCmd->addValue(data);
		}
	}

	SQLDeleteCommand* delCmd = new SQLDeleteCommand("container_part", "cid");
	delCmd->addRecord(cid);

	ContainerEditCommand* cmd = new ContainerEditCommand;
	cmd->addSQLCommand(delCmd);
	cmd->addSQLCommand(insCmd);

	return cmd;
}


void ContainerWidget::removeParts(unsigned int cid, ContainerPartTableModel::PartList parts)
{
	ContainerEditCommand* cmd = buildRemovePartsCommand(cid, parts);

	System* sys = System::getInstance();

	if (cmd) {
		sys->getEditStack()->push(cmd);
		sys->signalContainersChanged();
	}
}


void ContainerWidget::partsDropped(QList<ContainerPartTableModel::ContainerPart> parts)
{
	ContainerEditCommand* cmd = buildAddPartsCommand(currentID, parts);

	System* sys = System::getInstance();

	EditStack* editStack = sys->getEditStack();

	if (lastDragCommand) {
		CompoundEditCommand* compCmd = new CompoundEditCommand;
		compCmd->addCommand(lastDragCommand);
		if (cmd)
			compCmd->addCommand(cmd);
		editStack->push(compCmd);
	} else {
		if (cmd)
			editStack->push(cmd);
	}

	lastDragCommand = NULL;

	sys->signalContainersChanged();

	QItemSelection sel;

	for (unsigned int i = 0 ; i < parts.size() ; i++) {
		ContainerPartTableModel::ContainerPart& part = parts[i];

		QModelIndex idx = partTableModel->getIndexFromPart(part.cat, part.id);
		sel.select(idx, partTableModel->index(idx.row(), partTableModel->columnCount()-1));
	}

	ui.partTableView->selectionModel()->select(sel, QItemSelectionModel::Select);
}


void ContainerWidget::partsDragged(ContainerPartTableModel::PartList parts)
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection())
		return;

	ContainerEditCommand* cmd = buildRemovePartsCommand(currentID, parts);
	lastDragCommand = cmd;
}


void ContainerWidget::partTableViewContextMenuRequested(const QPoint& pos)
{
	QModelIndex idx = ui.partTableView->indexAt(pos);

	if (!idx.isValid()) {
		return;
	}

	ContainerPartTableModel::ContainerPart part = partTableModel->getIndexPart(idx);

	QMenu menu(ui.partTableView);

	if (!ui.partTableView->selectionModel()->selectedRows().empty()) {
		menu.addAction(removePartsAction);
	}

	menu.exec(ui.partTableView->mapToGlobal(pos));
}


void ContainerWidget::removeSelectedPartsRequested()
{
	QModelIndexList sel = ui.partTableView->selectionModel()->selectedRows();

	ContainerPartTableModel::PartList parts;

	int lowestRow = -1;

	for (unsigned int i = 0 ; i < sel.size() ; i++) {
		QModelIndex idx = sel[i];
		int row = idx.row();

		if (lowestRow == -1  ||  row < lowestRow)
			lowestRow = row;

		parts << partTableModel->getIndexPart(idx);
	}

	removeParts(currentID, parts);


	QModelIndex cidx = partTableModel->index(lowestRow, 0);

	if (!cidx.isValid()  &&  lowestRow != 0) {
		cidx = partTableModel->index(lowestRow-1, 0);
	}

	ui.partTableView->setCurrentIndex(cidx);
}


void ContainerWidget::jumpToContainer(unsigned int cid)
{
	QModelIndex idx = contTableModel->getIndexFromID(cid);
	ui.contTableView->setCurrentIndex(idx);
	displayContainer(cid);
}


void ContainerWidget::gotoNextContainer()
{
	QModelIndex cidx = ui.contTableView->currentIndex();

	if (!cidx.isValid()) {
		cidx = contTableModel->index(0, 0);
	} else {
		QModelIndex nextIdx = contTableModel->index(cidx.row()+1, 0);

		if (nextIdx.isValid())
			cidx = nextIdx;
	}

	ui.contTableView->setCurrentIndex(cidx);
}


bool ContainerWidget::eventFilter(QObject* obj, QEvent* evt)
{
	if (obj == ui.partTableView) {
		if (evt->type() == QEvent::KeyPress) {
			QKeyEvent* kevt = (QKeyEvent*) evt;

			if (kevt->key() == Qt::Key_Delete) {
				removeSelectedPartsRequested();
				kevt->accept();
				return true;
			}
		}
	} else {
		if (evt->type() == QEvent::KeyPress) {
			QKeyEvent* kevt = (QKeyEvent*) evt;

			if (obj == ui.contTableView) {
				if (kevt->key() == Qt::Key_Delete) {
					containerRemoveRequested();
					kevt->accept();
					return true;
				}
			}

			if (	(kevt->modifiers() & Qt::ControlModifier)  !=  0
					&&  kevt->key() == Qt::Key_Plus
			) {
				containerAddRequested();
				kevt->accept();
				return true;
			}
		}
	}

	return false;
}


