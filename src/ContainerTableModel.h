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

#ifndef CONTAINERTABLEMODEL_H_
#define CONTAINERTABLEMODEL_H_

#include "global.h"
#include "PartCategory.h"
#include <QtCore/QAbstractTableModel>
#include <QtCore/QList>


class ContainerTableModel : public QAbstractTableModel
{
	Q_OBJECT

private:
	struct PartEntry
	{
		PartCategory* cat;
		unsigned int id;
		QString desc;
	};

	typedef QList<PartEntry> PartList;
	typedef PartList::iterator PartIterator;
	typedef PartList::const_iterator ConstPartIterator;

	struct ContainerEntry
	{
		unsigned int id;
		QString name;
		PartList parts;
	};

	typedef QList<ContainerEntry> ContainerList;
	typedef ContainerList::iterator ContainerIterator;

public:
	ContainerTableModel(QObject* parent = NULL);

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orient, int role = Qt::DisplayRole) const;
	virtual void sort(int col, Qt::SortOrder order = Qt::AscendingOrder);
	virtual Qt::ItemFlags flags(const QModelIndex& idx) const;
	virtual bool setData(const QModelIndex& idx, const QVariant& val, int role = Qt::EditRole);

	void setFilter(const QString& sqlLikeFilter);
	void reload();
	unsigned int getIndexID(const QModelIndex& idx) const;
	QModelIndex getIndexFromID(unsigned int id) const;

private:


private:
	QString currentFilter;
	ContainerList containers;
	bool orderAscending;
};

#endif /* CONTAINERTABLEMODEL_H_ */
