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

#include "PartTableModel.h"
#include "System.h"
#include "NoDatabaseConnectionException.h"
#include "PartCategory.h"
#include "elutil.h"
#include <cstdlib>
#include <QtGui/QBrush>
#include <QtCore/QMultiMap>
#include <QtCore/QDataStream>
#include <nxcommon/sql/sql.h>


#define NUM_RETRIEVAL_CACHE_PADDING 250
#define NUM_IDONLY_RETRIEVAL_CACHE_PADDING 10000
#define MAX_DATA_CACHE_SIZE 2500
#define MAX_ID_TO_ROW_CACHE_SIZE 10000





PartTableModel::PartTableModel(PartCategory* partCat, Filter* filter, QObject* parent)
		: QAbstractTableModel(parent), partCat(partCat), filter(filter), cachedNumRows(0), dataCache(MAX_DATA_CACHE_SIZE),
		  idToRowCache(MAX_ID_TO_ROW_CACHE_SIZE), dragIdentifierCounter(0)
{
	System* sys = System::getInstance();

	EditStack* editStack = sys->getEditStack();


}


void PartTableModel::updateData()
{
	cachedNumRows = 0;

	dataCache.clear();
	idToRowCache.clear();

	System* sys = System::getInstance();

	if (sys->hasValidDatabaseConnection()) {
		try {
			cachedNumRows = partCat->countMatches(filter);
		} catch (SQLException& ex) {
			cachedNumRows = 0;
			reset();
			throw;
		}
	}

	reset();
}


void PartTableModel::cacheData(unsigned int rowBegin, unsigned int size, bool fullData)
{
	System* sys = System::getInstance();

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	if (!sql.isValid()) {
		throw NoDatabaseConnectionException("PartTableModel::getPartData() called without an open database!",
				__FILE__, __LINE__);
	}

	QList<unsigned int> ids = partCat->find(filter, rowBegin, size);

	if (fullData) {
		PartCategory::PropertyList multiValueProps;

		PartCategory::PropertyList props;

		for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
			PartProperty* prop = *it;

			if ((prop->getFlags() & PartProperty::DisplayHideFromListingTable)  ==  0) {
				props << prop;
			}
		}

		PartCategory::MultiDataMap data = partCat->getValues(ids, props);

		QMap<unsigned int, PartData*> idDataMap;

		QMap<PartProperty*, QSet<unsigned int> > linkIds;

		for (PartCategory::MultiDataMap::iterator it = data.begin() ; it != data.end() ; it++) {
			unsigned int id = it.key();
			QMap<PartProperty*, QList<QString> > idData = it.value();

			PartData* ddata = new PartData;
			ddata->id = id;

			for (PartCategory::PropertyIterator pit = props.begin() ; pit != props.end() ; pit++) {
				PartProperty* prop = *pit;

				QList<QString> rawData = idData[prop];

				if (prop->getType() == PartProperty::PartLink) {
					for (QList<QString>::iterator sit = rawData.begin() ; sit != rawData.end() ; sit++) {
						linkIds[prop] << (*sit).toUInt();
					}
					ddata->displayData << QString();
				} else {
					QString displayStr = prop->formatValueForDisplay(rawData);
					ddata->displayData << displayStr;
				}
			}

			idDataMap.insert(id, ddata);
		}

		//QMap<PartProperty*, PartCategory::MultiDataMap> linkValues;
		QMap<PartProperty*, QMap<unsigned int, QString> > linkDescs;

		for (QMap<PartProperty*, QSet<unsigned int> >::iterator it = linkIds.begin() ; it != linkIds.end() ; it++) {
			PartProperty* prop = it.key();
			QSet<unsigned int> idSet = it.value();
			QList<unsigned int> ids = idSet.toList();

			PartCategory* linkCat = prop->getPartLinkCategory();


			linkDescs[prop] = linkCat->getDescriptions(ids);
		}

		for (PartCategory::MultiDataMap::iterator it = data.begin() ; it != data.end() ; it++) {
			unsigned int id = it.key();
			QMap<PartProperty*, QList<QString> > idData = it.value();

			PartData* ddata = idDataMap[id];

			unsigned int i = 0;
			for (PartCategory::PropertyIterator pit = props.begin() ; pit != props.end() ; pit++, i++) {
				PartProperty* prop = *pit;

				if (prop->getType() != PartProperty::PartLink)
					continue;

				QList<QString> rawData = idData[prop];

				if (prop->getType() == PartProperty::PartLink) {
					QList<QString> rawIds = idData[prop];

					PartCategory* linkCat = prop->getPartLinkCategory();

					QMap<unsigned int, QString> propLinkDescs = linkDescs[prop];

					QString displayStr("");

					for (QList<QString>::iterator iit = rawIds.begin() ; iit != rawIds.end() ; iit++) {
						if (iit != rawIds.begin())
							displayStr += ", ";

						unsigned int linkId = (*iit).toUInt();
						displayStr += propLinkDescs[linkId];
					}

					ddata->displayData[i] = displayStr;
				}
			}
		}

		int crow = rowBegin;
		for (QList<unsigned int>::iterator it = ids.begin() ; it != ids.end() ; it++, crow++) {
			unsigned int id = *it;

			PartData* data = idDataMap[id];
			dataCache.insert(crow, data);

			idToRowCache.insert(id, new int(crow), 1);
		}
	} else {
		int crow = rowBegin;
		for (QList<unsigned int>::iterator it = ids.begin() ; it != ids.end() ; it++, crow++) {
			unsigned int id = *it;

			idToRowCache.insert(id, new int(crow), 1);
		}
	}
}


PartTableModel::PartData* PartTableModel::getPartData(unsigned int row)
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
			return QString("%1").arg(data->id);
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
		return createIndex(row, col, (void*) NULL);
	}

	return createIndex(row, col, data->id);
}


QVariant PartTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
	if (orient != Qt::Horizontal)
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();

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
		PartProperty* sortProp = NULL;

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

	types << "application/nx-electronics-partrecords";

	return types;
}


QMimeData* PartTableModel::mimeData(const QModelIndexList& indices) const
{
	QMimeData* mimeData = new QMimeData;

	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	char dragIdentifier[16];

	const PartTableModel* thisCpy = this;
	memset(dragIdentifier, 0, sizeof(dragIdentifier));
	memcpy(dragIdentifier, (const char*) &thisCpy, sizeof(PartTableModel*));
	memcpy(dragIdentifier + sizeof(PartTableModel*), &dragIdentifierCounter, 1);
	const_cast<PartTableModel*>(this)->dragIdentifierCounter++;

	stream.writeRawData(dragIdentifier, sizeof(dragIdentifier));

	quint32 size = 0;
	for (QModelIndexList::const_iterator it = indices.begin() ; it != indices.end() ; it++) {
		QModelIndex idx = *it;

		if (idx.column() != 0)
			continue;

		size++;
	}

	stream << size;

	for (QModelIndexList::const_iterator it = indices.begin() ; it != indices.end() ; it++) {
		QModelIndex idx = *it;

		if (idx.column() != 0)
			continue;

		stream.writeRawData((const char*) &partCat, sizeof(PartCategory*));

		unsigned int id = getIndexID(idx);
		stream << id;
	}

	mimeData->setData("application/nx-electronics-partrecords", data);
	return mimeData;
}


unsigned int PartTableModel::getIndexID(const QModelIndex& idx) const
{
	if (!idx.isValid())
		return INVALID_PART_ID;

	return idx.internalId();
}


unsigned int PartTableModel::rowToId(int row)
{
	PartData* data = getPartData(row);

	return data->id;
}


int PartTableModel::idToRow(unsigned int id, bool fullSearch)
{
	if (id == INVALID_PART_ID)
		return -1;

	int* rowPtr = idToRowCache.object(id);

	if (fullSearch) {
		unsigned int offset = 0;
		while (!rowPtr  &&  offset < cachedNumRows) {
			cacheData(offset, 2*NUM_IDONLY_RETRIEVAL_CACHE_PADDING, false);
			rowPtr = idToRowCache.object(id);
			offset += 2*NUM_IDONLY_RETRIEVAL_CACHE_PADDING;
		}
	}

	if (!rowPtr)
		return -1;

	return *rowPtr;
}


QModelIndex PartTableModel::getIndexWithID(unsigned int id)
{
	int row = idToRow(id, true);

	if (row == -1)
		return QModelIndex();

	return index(row, 0);
}


