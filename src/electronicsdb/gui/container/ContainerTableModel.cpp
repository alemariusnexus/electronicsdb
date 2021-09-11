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

#include "ContainerTableModel.h"

#include <cstdlib>
#include <memory>
#include <QMap>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../model/container/PartContainer.h"
#include "../../model/container/PartContainerFactory.h"
#include "../../model/part/Part.h"
#include "../../model/part/PartFactory.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../dragndrop/DragNDropManager.h"

namespace electronicsdb
{


ContainerTableModel::ContainerTableModel(QObject* parent)
        : QAbstractTableModel(parent), orderAscending(true)
{
}

ContainerTableModel::~ContainerTableModel()
{
    clear();
}

void ContainerTableModel::clear()
{
    containers.clear();
}

void ContainerTableModel::setFilter(const QString& sqlLikeFilter)
{
    currentFilter = sqlLikeFilter;

    reload();
}

void ContainerTableModel::reload()
{
    beginResetModel();

    clear();

    System* sys = System::getInstance();

    if (!sys->isAppModelLoaded()) {
        endResetModel();
        return;
    }

    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create());

    PartContainerList conts = PartContainerFactory::getInstance().loadItems(
            currentFilter.isEmpty() ? QString() : QString("WHERE name LIKE %1").arg(dbw->escapeString(currentFilter)),
            orderAscending ? "ORDER BY name ASC" : "ORDER BY name DESC",
            PartContainerFactory::LoadContainerParts
            );

    PartList allParts;
    for (const PartContainer& cont : conts) {
        allParts << cont.getParts();
    }

    PartFactory::getInstance().loadItems(allParts.begin(), allParts.end(), 0, [](PartCategory* pcat) {
        return pcat->getDescriptionProperties();
    });

    containers = conts;

    endResetModel();
}

int ContainerTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return containers.size();
}

int ContainerTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant ContainerTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int col = index.column();

    const PartContainer& cont = containers[row];

    if (col == 0) {
        if (role == Qt::DisplayRole) {
            return cont.getName();
        }
    } else if (col == 1) {
        if (role == Qt::DisplayRole) {
            QString desc("");

            for (const Part& part : cont.getParts()) {
                if (!desc.isEmpty())
                    desc += "\n";
                desc += part.getDescription();
            }

            return desc;
        }
    }

    return QVariant();
}

QModelIndex ContainerTableModel::index(int row, int col, const QModelIndex& parent) const
{
    if (parent.isValid())
        return QModelIndex();
    if (!hasIndex(row, col, parent))
        return QModelIndex();

    return createIndex(row, col);
}

QVariant ContainerTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (orient != Qt::Horizontal)
        return QVariant();
    if (role != Qt::DisplayRole)
        return QVariant();


    if (section == 0) {
        return tr("Name");
    } else if (section == 1) {
        return tr("Parts");
    } else {
        return QVariant();
    }
}

void ContainerTableModel::sort(int col, Qt::SortOrder order)
{
    if (order == Qt::AscendingOrder) {
        orderAscending = true;
    } else {
        orderAscending = false;
    }

    reload();
}

Qt::ItemFlags ContainerTableModel::flags(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if (idx.column() == 0) {
        return flags | Qt::ItemIsEditable;
    } else {
        return flags;
    }
}

bool ContainerTableModel::setData(const QModelIndex& idx, const QVariant& val, int role)
{
    if (role != Qt::EditRole)
        return false;
    if (idx.column() != 0)
        return false;

    int row = idx.row();

    QString name = val.toString();

    if (name.isEmpty()) {
        return false;
    }

    for (int i = 0 ; i < containers.size() ; i++) {
        const PartContainer& cont = containers[i];

        if (cont.getName() == name  &&  i != row) {
            return false;
        }
    }

    PartContainer& cont = containers[row];

    System* sys = System::getInstance();

    if (!sys->hasValidDatabaseConnection()) {
        reload();
        return false;
    }

    cont.setName(name);
    PartContainerFactory::getInstance().updateItem(cont);

    return true;
}

Qt::DropActions ContainerTableModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

QStringList ContainerTableModel::mimeTypes() const
{
    QStringList types;
    types << PartFactory::getInstance().getPartListMimeDataType();
    return types;
}

bool ContainerTableModel::dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int col,
        const QModelIndex& parent)
{
    if (action != Qt::MoveAction  &&  action != Qt::CopyAction) {
        return false;
    }

    PartContainer cont = getIndexContainer(parent);
    if (!cont) {
        return false;
    }

    PartList parts = DragNDropManager::getInstance().acceptContainerPartsDrag(mimeData, cont);

    if (!parts.empty()) {
        QMap<PartContainer, PartList> partsByCont;
        partsByCont[cont] = parts;

        emit partsDropped(partsByCont);

        return true;
    }

    return false;
}

QMimeData* ContainerTableModel::mimeData(const QModelIndexList& indices) const
{
    QMap<PartContainer, PartList> partsByCont;

    for (const QModelIndex& idx : indices) {
        if (idx.column() == 0) {
            PartContainer cont = getIndexContainer(idx);
            partsByCont[cont] = cont.getParts();
        }
    }

    return DragNDropManager::getInstance().startContainerPartsDrag(partsByCont);
}

PartContainer ContainerTableModel::getIndexContainer(const QModelIndex& idx) const
{
    return containers[idx.row()];
}

QModelIndex ContainerTableModel::getIndexFromContainer(const PartContainer& cont) const
{
    for (int i = 0 ; i < containers.size() ; i++) {
        if (containers[i] == cont) {
            return index(i, 0);
        }
    }
    return QModelIndex();
}


}
