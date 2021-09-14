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

#include "PartTableModel.h"

#include <cstdlib>
#include <QBrush>
#include <QDataStream>
#include <QMimeData>
#include <QMultiMap>
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../model/part/PartFactory.h"
#include "../../model/PartCategory.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../dragndrop/DragNDropManager.h"


#define NUM_RETRIEVAL_CACHE_PADDING 250
#define NUM_IDONLY_RETRIEVAL_CACHE_PADDING 10000
#define MAX_DATA_CACHE_SIZE 2500
#define MAX_ID_TO_ROW_CACHE_SIZE 10000

namespace electronicsdb
{

// TODO: Implement PROPER lazy loading


PartTableModel::PartTableModel(PartCategory* partCat, Filter* filter, QObject* parent)
        : QAbstractTableModel(parent), partCat(partCat), filter(filter), cachedNumRows(0), dataCache(MAX_DATA_CACHE_SIZE),
          idToRowCache(MAX_ID_TO_ROW_CACHE_SIZE), dragIdentifierCounter(0)
{
}


void PartTableModel::updateData()
{
    beginResetModel();

    cachedNumRows = 0;

    dataCache.clear();
    idToRowCache.clear();

    System* sys = System::getInstance();

    if (sys->hasValidDatabaseConnection()) {
        try {
            cachedNumRows = static_cast<int>(partCat->countMatches(filter));
        } catch (SQLException&) {
            cachedNumRows = 0;
            endResetModel();
            throw;
        }
    }

    endResetModel();
}


void PartTableModel::cacheData(int rowBegin, int size, bool fullData)
{
    PartList parts = partCat->find(filter, rowBegin, size);

    if (fullData) {
        PartCategory::PropertyList props;

        // Collect table properties
        for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
            PartProperty* prop = *it;

            if ((prop->getFlags() & PartProperty::DisplayHideFromListingTable)  ==  0) {
                props << prop;
            }
        }

        // Load table data
        PartFactory::getInstance().loadItemsSingleCategory(parts.begin(), parts.end(), 0, props);

        // Load descriptions for all parts, grouped by property
        QMap<PartProperty*, QList<QString>> descsByProp;
        for (PartProperty* prop : props) {
            QList<QVariant> vals;
            for (const Part& part : parts) {
                vals << part.getData(prop);
            }
            descsByProp[prop] = prop->formatValuesForDisplay(vals);
        }

        // Build PartData and populate caches
        int partIdx = 0;
        for (const Part& part : parts) {
            PartData* pdata = new PartData;
            pdata->part = part;

            for (PartProperty* prop : props) {
                pdata->displayData << descsByProp[prop][partIdx];
            }

            int row = rowBegin+partIdx;

            dataCache.insert(row, pdata);
            idToRowCache.insert(part.getID(), new int(row));

            partIdx++;
        }
    } else {
        int crow = rowBegin;
        for (const Part& part : parts) {
            idToRowCache.insert(part.getID(), new int(crow));
            crow++;
        }
    }
}


PartTableModel::PartData* PartTableModel::getPartData(int row)
{
    PartData* data = dataCache.object(row);

    if (data) {
        return data;
    }

    unsigned int offset;

    if (row >= NUM_RETRIEVAL_CACHE_PADDING)
        offset = row - NUM_RETRIEVAL_CACHE_PADDING;
    else
        offset = 0;

    cacheData(offset, (row - offset) + NUM_RETRIEVAL_CACHE_PADDING + 1);

    return dataCache.object(row);
}


int PartTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return cachedNumRows;
}


int PartTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    int cc = 0;

    for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
        PartProperty* prop = *it;

        if ((prop->getFlags() & PartProperty::DisplayHideFromListingTable)  ==  0) {
            cc++;
        }
    }

    return cc + 1;
}


QVariant PartTableModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();

    PartData* data = const_cast<PartTableModel*>(this)->getPartData(row);

    if (!data)
        return QVariant();

    int col = index.column();

    if (role == Qt::DisplayRole) {
        if (col == 0) {
            return QString::number(data->part.getID());
        } else {
            return data->displayData[col-1];
        }
    } else if (role == Qt::BackgroundRole) {
        return QVariant();
    } else {
        return QVariant();
    }
}


QModelIndex PartTableModel::index(int row, int col, const QModelIndex& parent) const
{
    if (parent.isValid())
        return QModelIndex();
    if (!hasIndex(row, col, parent))
        return QModelIndex();

    PartData* data = const_cast<PartTableModel*>(this)->getPartData(row);

    if (!data) {
        return createIndex(row, col, (void*) nullptr);
    }

    return createIndex(row, col);
}


QVariant PartTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (orient != Qt::Horizontal)
        return QVariant();
    if (role != Qt::DisplayRole  &&  role != Qt::TextAlignmentRole)
        return QVariant();

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignLeft;
    }

    if (section == 0) {
        return tr("ID");
    }

    unsigned int i = 1;
    for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
        PartProperty* prop = *it;

        if ((prop->getFlags() & PartProperty::DisplayHideFromListingTable)  ==  0) {
            if (i == section) {
                return prop->getUserReadableName();
            }

            i++;
        }
    }

    return QVariant();
}


void PartTableModel::sort(int col, Qt::SortOrder order)
{
    if (col == 0) {
        QString orderCode = QString("ORDER BY %1.%2 ")
                .arg(partCat->getTableName())
                .arg(partCat->getIDField());

        if (order == Qt::AscendingOrder)
            orderCode += "ASC";
        else
            orderCode += "DESC";

        filter->setSQLOrderCode(orderCode);
    } else {
        PartProperty* sortProp = nullptr;

        unsigned int i = 1;
        for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
            PartProperty* prop = *it;

            if ((prop->getFlags() & PartProperty::DisplayHideFromListingTable)  ==  0) {
                if (i == col) {
                    sortProp = prop;
                    break;
                }

                i++;
            }
        }

        if (!sortProp)
            return;

        if (order == Qt::AscendingOrder)
            filter->setSQLOrderCode(sortProp->getSQLAscendingOrderCode());
        else
            filter->setSQLOrderCode(sortProp->getSQLDescendingOrderCode());
    }

    emit updateRequested();
}


Qt::ItemFlags PartTableModel::flags(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}


Qt::DropActions PartTableModel::supportedDropActions() const
{
    return Qt::CopyAction;
}


QStringList PartTableModel::mimeTypes() const
{
    QStringList types;
    types << PartFactory::getInstance().getPartListMimeDataType();
    return types;
}


QMimeData* PartTableModel::mimeData(const QModelIndexList& indices) const
{
    PartList parts;
    for (const QModelIndex& idx : indices) {
        if (idx.column() == 0) {
            parts << getIndexPart(idx);
        }
    }

    QMimeData* mimeData = PartFactory::getInstance().encodePartListToMimeData(parts);
    return mimeData;
}


Part PartTableModel::getIndexPart(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return Part();

    PartData* data = const_cast<PartTableModel*>(this)->getPartData(idx.row());
    return data ? data->part : Part();
}


int PartTableModel::partToRow(const Part& part, bool fullSearch)
{
    if (!part) {
        return -1;
    }

    int* rowPtr = idToRowCache.object(part.getID());

    if (fullSearch) {
        int offset = 0;
        while (!rowPtr  &&  offset < cachedNumRows) {
            cacheData(offset, 2*NUM_IDONLY_RETRIEVAL_CACHE_PADDING, false);
            rowPtr = idToRowCache.object(part.getID());
            offset += 2*NUM_IDONLY_RETRIEVAL_CACHE_PADDING;
        }
    }

    if (!rowPtr)
        return -1;

    return *rowPtr;
}


QModelIndex PartTableModel::getIndexWithPart(const Part& part)
{
    int row = partToRow(part, true);

    if (row == -1)
        return QModelIndex();

    return index(row, 0);
}


}
