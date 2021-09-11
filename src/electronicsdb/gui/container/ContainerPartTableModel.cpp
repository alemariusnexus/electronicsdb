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

#include "ContainerPartTableModel.h"

#include <cstdio>
#include <cstdlib>
#include <QDataStream>
#include <QUuid>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../model/part/PartFactory.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../dragndrop/DragNDropManager.h"

namespace electronicsdb
{


ContainerPartTableModel::ContainerPartTableModel(QObject* parent)
        : QAbstractTableModel(parent), currentCont(PartContainer())
{
}

ContainerPartTableModel::~ContainerPartTableModel()
{
    doClear();
}

void ContainerPartTableModel::display(const PartContainer& cont, PartList parts)
{
    beginResetModel();

    doClear();
    this->parts = parts;
    currentCont = cont;

    PartFactory::getInstance().loadItems(parts.begin(), parts.end(), 0, [](PartCategory* pcat) {
        return pcat->getDescriptionProperties();
    });

    endResetModel();
}

PartContainer ContainerPartTableModel::getDisplayedContainer() const
{
    return currentCont;
}

void ContainerPartTableModel::doClear()
{
    currentCont = PartContainer();
    parts.clear();
}

int ContainerPartTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

int ContainerPartTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return parts.size();
}

QModelIndex ContainerPartTableModel::index(int row, int col, const QModelIndex& parent) const
{
    if (!hasIndex(row, col, parent))
        return QModelIndex();
    return createIndex(row, col);
}

QVariant ContainerPartTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();

    const Part& part = parts[row];

    if (role == Qt::DisplayRole) {
        return part.getDescription();
    }

    return QVariant();
}

QVariant ContainerPartTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (orient != Qt::Horizontal)
        return QVariant();
    if (role != Qt::DisplayRole)
        return QVariant();

    return tr("Part Description");
}

Qt::ItemFlags ContainerPartTableModel::flags(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

    return flags;
}

Qt::DropActions ContainerPartTableModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

QStringList ContainerPartTableModel::mimeTypes() const
{
    QStringList types;
    types << PartFactory::getInstance().getPartListMimeDataType();
    return types;
}

bool ContainerPartTableModel::dropMimeData (
        const QMimeData* mimeData, Qt::DropAction action, int row, int col, const QModelIndex& parent
) {
    if (!currentCont) {
        return false;
    }
    if (action != Qt::MoveAction  &&  action != Qt::CopyAction) {
        return false;
    }

    PartList parts = DragNDropManager::getInstance().acceptContainerPartsDrag(mimeData, currentCont);

    if (!parts.empty()) {
        QMap<PartContainer, PartList> partsByCont;
        partsByCont[currentCont] = parts;

        emit partsDropped(partsByCont);

        return true;
    }

    return false;
}

QMimeData* ContainerPartTableModel::mimeData(const QModelIndexList& indices) const
{
    PartList parts;
    for (const QModelIndex& idx : indices) {
        if (idx.column() == 0) {
            parts << getIndexPart(idx);
        }
    }

    return DragNDropManager::getInstance().startContainerPartsDrag(currentCont, parts);
}

Part ContainerPartTableModel::getIndexPart(const QModelIndex& idx) const
{
    if (!idx.isValid()) {
        return Part();
    }
    return parts[idx.row()];
}

QModelIndex ContainerPartTableModel::getIndexFromPart(const Part& part) const
{
    for (int i = 0 ; i < parts.size() ; i++) {
        if (parts[i] == part) {
            return index(i, 0);
        }
    }

    return QModelIndex();
}


}
