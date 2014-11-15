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

#ifndef PARTTABLEMODEL_H_
#define PARTTABLEMODEL_H_

#include "global.h"
#include "Filter.h"
#include "PartCategory.h"
#include <QtCore/QAbstractTableModel>
#include <QtCore/QCache>
#include <QtCore/QSet>
#include <QtCore/QMap>


class PartTableModel : public QAbstractTableModel
{
	Q_OBJECT

private:
	struct PartData
	{
		unsigned int id;
		QList<QString> displayData;
	};

public:
	PartTableModel(PartCategory* partCat, Filter* filter, QObject* parent = NULL);

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orient, int role = Qt::DisplayRole) const;
	virtual void sort(int col, Qt::SortOrder order = Qt::AscendingOrder);
	virtual Qt::ItemFlags flags(const QModelIndex& idx) const;
	virtual Qt::DropActions supportedDropActions() const;
	virtual QStringList mimeTypes() const;
	virtual QMimeData* mimeData(const QModelIndexList& indices) const;

	unsigned int getIndexID(const QModelIndex& idx) const;
	QModelIndex getIndexWithID(unsigned int id);

public slots:
	void updateData();

private:
	void cacheData(unsigned int rowBegin, unsigned int size = 1, bool fullData = true);
	PartData* getPartData(unsigned int row);
	unsigned int rowToId(int row);
	int idToRow(unsigned int id, bool fullSearch = false);

signals:
	void updateRequested();

private:
	PartCategory* partCat;
	Filter* filter;

	unsigned int cachedNumRows;

	QCache<int, PartData> dataCache;
	QCache<unsigned int, int> idToRowCache;

	uint8_t dragIdentifierCounter;
};

#endif /* PARTTABLEMODEL_H_ */
