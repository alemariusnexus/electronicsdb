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

#ifndef CONTAINERPARTTABLEMODEL_H_
#define CONTAINERPARTTABLEMODEL_H_

#include "global.h"
#include "PartCategory.h"
#include <QtCore/QAbstractTableModel>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QMimeData>


class ContainerPartTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	struct ContainerPart
	{
		PartCategory* cat;
		unsigned int id;
		QString desc;
	};

	typedef QList<ContainerPart> PartList;
	typedef PartList::iterator PartIterator;
	typedef PartList::const_iterator ConstPartIterator;

public:
	ContainerPartTableModel(QObject* parent = NULL);

	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orient, int role = Qt::DisplayRole) const;
	virtual Qt::ItemFlags flags(const QModelIndex& idx) const;
	virtual Qt::DropActions supportedDropActions() const;
	virtual QStringList mimeTypes() const;
	virtual bool dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int col, const QModelIndex& parent);
	virtual QMimeData* mimeData(const QModelIndexList& indices) const;

	void display(unsigned int cid, PartList parts);
	void reload();
	ContainerPart getIndexPart(const QModelIndex& idx) const;
	QModelIndex getIndexFromPart(PartCategory* cat, unsigned int id) const;

private slots:
	void dropAccepted(Qt::DropAction action, const char* dragIdentifier);

signals:
	void partsDropped(ContainerPartTableModel::PartList parts);
	void partsDragged(ContainerPartTableModel::PartList parts);

private:
	unsigned int currentCID;
	PartList parts;

	PartList draggedParts;
	Qt::DropAction dragAction;
	uint8_t dragIdentifierCounter;
};

#endif /* CONTAINERPARTTABLEMODEL_H_ */
