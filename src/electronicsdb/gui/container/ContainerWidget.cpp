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

#include "ContainerWidget.h"

#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../model/container/PartContainerFactory.h"
#include "../../model/part/PartFactory.h"
#include "../../System.h"
#include "../dragndrop/DragNDropManager.h"
#include "ContainerTableItemDelegate.h"

namespace electronicsdb
{


ContainerWidget::ContainerWidget(QWidget* parent)
        : QWidget(parent), currentCont(PartContainer())
{
    ui.setupUi(this);

    System* sys = System::getInstance();


    ui.contTableView->setItemDelegate(new ContainerTableItemDelegate(ui.contTableView, this));
    contTableModel = new ContainerTableModel(ui.contTableView);
    ui.contTableView->setModel(contTableModel);

    ui.contTableView->setDragDropMode(QListView::DragDrop);
    ui.contTableView->setDragEnabled(true);
    ui.contTableView->setAcceptDrops(true);
    ui.contTableView->setDefaultDropAction(Qt::MoveAction);
    ui.contTableView->setDropIndicatorShown(true);

    ui.contTableView->setAutoScroll(true);
    ui.contTableView->setSelectionMode(QTableView::ExtendedSelection);
    ui.contTableView->setSelectionBehavior(QTableView::SelectRows);

    ui.contTableView->horizontalHeader()->resizeSection(0, 100);
    ui.contTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);


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


    ui.contAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    ui.contRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));


    removePartsAction = new QAction(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.png")),
            tr("Remove"), this);
    connect(removePartsAction, &QAction::triggered, this, &ContainerWidget::removeSelectedPartsRequested);



    connect(ui.contSearchButton, &QPushButton::clicked, this, &ContainerWidget::searchRequested);
    connect(ui.contIdField, &QLineEdit::returnPressed, this, &ContainerWidget::searchRequested);
    connect(ui.contTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &ContainerWidget::currentContainerChanged);
    connect(ui.contTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ContainerWidget::containerSelectionChanged);
    connect(ui.contAddButton, &QPushButton::clicked, this, &ContainerWidget::containerAddRequested);
    connect(ui.contRemoveButton, &QPushButton::clicked, this, &ContainerWidget::containerRemoveRequested);
    connect(ui.partTableView, &QTableView::doubleClicked, this, &ContainerWidget::partActivated);
    connect(ui.partTableView, &QTableView::customContextMenuRequested,
            this, &ContainerWidget::partTableViewContextMenuRequested);
    connect(contTableModel, &ContainerTableModel::partsDropped, this, &ContainerWidget::partsDropped);
    connect(partTableModel, &ContainerPartTableModel::partsDropped, this, &ContainerWidget::partsDropped);


    connect(sys, &System::appModelReset, this, &ContainerWidget::appModelReset);
    connect(&PartContainerFactory::getInstance(), &PartContainerFactory::containersChanged, this, &ContainerWidget::reload);
    connect(&PartFactory::getInstance(), &PartFactory::partsChanged, this, &ContainerWidget::reload);
    connect(qApp, &QApplication::aboutToQuit, this, &ContainerWidget::aboutToQuit);


    reload();
}

void ContainerWidget::setConfigurationName(const QString& name)
{
    configName = name;

    QSettings s;

    s.beginGroup(QString("gui/container_widget/%1").arg(name));

    ui.contTableView->horizontalHeader()->restoreState(s.value("cont_table_hhdr_state").toByteArray());
    ui.splitter->restoreState(s.value("main_splitter_state").toByteArray());

    s.endGroup();
}

void ContainerWidget::aboutToQuit()
{
    QSettings s;

    s.beginGroup(QString("gui/container_widget/%1").arg(configName));

    s.setValue("cont_table_hhdr_state", ui.contTableView->horizontalHeader()->saveState());
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

    QList<PartContainer> selConts;
    for (QModelIndex idx : sel) {
        selConts << contTableModel->getIndexContainer(idx);
    }

    ui.contTableView->selectionModel()->blockSignals(true);
    contTableModel->reload();
    ui.contTableView->selectionModel()->blockSignals(false);
    displayContainer(currentCont);

    QItemSelection selection;

    for (const PartContainer& cont : selConts) {
        QModelIndex idx = contTableModel->getIndexFromContainer(cont);
        selection.select(idx, contTableModel->index(idx.row(), contTableModel->columnCount()-1));
    }

    ui.contTableView->setCurrentIndex(contTableModel->getIndexFromContainer(currentCont));

    ui.contTableView->selectionModel()->select(selection, QItemSelectionModel::Select);

    if (focus) {
        ui.contTableView->setFocus();
    }

    containerSelectionChanged();
}

void ContainerWidget::displayContainer(const PartContainer& cont)
{
    System* sys = System::getInstance();

    currentCont = cont;

    if (!currentCont  ||  !sys->hasValidDatabaseConnection()) {
        partTableModel->display(PartContainer(), PartList());
    } else {
        PartContainerFactory::getInstance().loadItems(currentCont, PartContainerFactory::LoadContainerParts);
        partTableModel->display(currentCont, currentCont.getParts());
    }
}

void ContainerWidget::appModelReset()
{
    System* sys = System::getInstance();
    bool modelLoaded = sys->isAppModelLoaded();

    contTableModel->reload();

    ui.contAddButton->setEnabled(modelLoaded);
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
        displayContainer(PartContainer());
        return;
    }

    PartContainer cont = contTableModel->getIndexContainer(newIdx);
    displayContainer(cont);
}

void ContainerWidget::partActivated(const QModelIndex& idx)
{
    if (!idx.isValid())
        return;

    Part part = partTableModel->getIndexPart(idx);
    System::getInstance()->jumpToPart(part);
}

void ContainerWidget::containerAddRequested()
{
    System* sys = System::getInstance();

    if (!sys->hasValidDatabaseConnection())
        return;

    ui.contTableView->setCurrentIndex(QModelIndex());

    PartContainerList newConts = PartContainerFactory::getInstance().loadItems (
            QString("WHERE name LIKE 'NEW!_%' ESCAPE '!'"),
            QString()
            );

    unsigned int newNum = 1;
    for (const PartContainer& cont : newConts) {
        bool ok;
        unsigned int num = cont.getName().mid(4).toUInt(&ok) + 1;
        if (ok) {
            newNum = std::max(newNum, num);
        }
    }

    QString name = QString("NEW%1").arg(newNum);

    PartContainer newCont{PartContainer::CreateBlankTag()};
    newCont.setName(name);

    PartContainerFactory::getInstance().insertItem(newCont);
    assert(newCont.hasID());

    QModelIndex idx = contTableModel->getIndexFromContainer(newCont);

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
    QList<PartContainer> conts;

    int lowestRow = -1;

    for (QModelIndexList::iterator it = sel.begin() ; it != sel.end() ; it++) {
        QModelIndex idx = *it;
        int row = idx.row();

        if (lowestRow == -1  ||  row < lowestRow)
            lowestRow = row;

        conts << contTableModel->getIndexContainer(idx);
    }

    PartContainerFactory::getInstance().deleteItems(conts.begin(), conts.end());

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

void ContainerWidget::removeParts(const PartContainer& cont, const PartList& parts)
{
    PartContainer ncCont(cont);

    PartContainerFactory::getInstance().loadItems(ncCont, PartContainerFactory::LoadContainerParts);
    assert(ncCont.isPartAssocsLoaded());

    PartList newParts;
    for (const Part& part : ncCont.getParts()) {
        if (!parts.contains(part)) {
            newParts << part;
        }
    }

    ncCont.setParts(newParts);

    PartContainerFactory::getInstance().updateItem(ncCont);
}

void ContainerWidget::partsDropped(const QMap<PartContainer, PartList>& parts)
{
    PartList curContParts = parts[currentCont];

    if (!curContParts.empty()) {
        QItemSelection sel;

        for (Part part : curContParts) {
            QModelIndex idx = partTableModel->getIndexFromPart(part);
            sel.select(idx, partTableModel->index(idx.row(), partTableModel->columnCount()-1));
        }

        ui.partTableView->selectionModel()->select(sel, QItemSelectionModel::Select);
    }
}

void ContainerWidget::partTableViewContextMenuRequested(const QPoint& pos)
{
    QModelIndex idx = ui.partTableView->indexAt(pos);

    if (!idx.isValid()) {
        return;
    }

    Part part = partTableModel->getIndexPart(idx);

    QMenu menu(ui.partTableView);

    if (!ui.partTableView->selectionModel()->selectedRows().empty()) {
        menu.addAction(removePartsAction);
    }

    menu.exec(ui.partTableView->mapToGlobal(pos));
}

void ContainerWidget::removeSelectedPartsRequested()
{
    QModelIndexList sel = ui.partTableView->selectionModel()->selectedRows();

    PartList parts;

    int lowestRow = -1;

    for (QModelIndex idx : sel) {
        int row = idx.row();

        if (lowestRow == -1  ||  row < lowestRow)
            lowestRow = row;

        parts << partTableModel->getIndexPart(idx);
    }

    removeParts(currentCont, parts);


    QModelIndex cidx = partTableModel->index(lowestRow, 0);

    if (!cidx.isValid()  &&  lowestRow != 0) {
        cidx = partTableModel->index(lowestRow-1, 0);
    }

    ui.partTableView->setCurrentIndex(cidx);
}

void ContainerWidget::jumpToContainer(const PartContainer& cont)
{
    QModelIndex idx = contTableModel->getIndexFromContainer(cont);
    ui.contTableView->setCurrentIndex(idx);
    displayContainer(cont);
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


}
