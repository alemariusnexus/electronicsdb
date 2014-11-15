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

#include "ContainerPartTableModel.h"
#include <nxcommon/exception/InvalidValueException.h>
#include "elutil.h"
#include "System.h"
#include <cstdlib>
#include <cstdio>
#include <QtCore/QDataStream>




ContainerPartTableModel::ContainerPartTableModel(QObject* parent)
		: QAbstractTableModel(parent), currentCID(INVALID_CONTAINER_ID), dragAction(Qt::IgnoreAction),
		  dragIdentifierCounter(0)
{
	System* sys = System::getInstance();

	connect(sys, SIGNAL(dropAccepted(Qt::DropAction, const char*)), this, SLOT(dropAccepted(Qt::DropAction, const char*)));
}


void ContainerPartTableModel::display(unsigned int cid, PartList parts)
{
	this->parts = parts;
	currentCID = cid;

	reload();
}


void ContainerPartTableModel::reload()
{
	reset();
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

	const ContainerPart& part = parts[row];

	if (role == Qt::DisplayRole) {
		return part.desc;
	}

	return QVariant();
}


QVariant ContainerPartTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
	if (orient != Qt::Horizontal)
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();

	return "Part Description";
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

	types << "application/nx-electronics-partrecords";

	return types;
}


bool ContainerPartTableModel::dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int col,
		const QModelIndex& parent)
{
	if (currentCID == INVALID_CONTAINER_ID)
		return false;

	if (action != Qt::MoveAction  &&  action != Qt::CopyAction)
		return false;
	if (!mimeData->hasFormat("application/nx-electronics-partrecords"))
		return false;

	QByteArray data = mimeData->data("application/nx-electronics-partrecords");

	QDataStream stream(&data, QIODevice::ReadOnly);

	char dragIdentifier[16];
	stream.readRawData(dragIdentifier, sizeof(dragIdentifier));

	char lDragIdentifier[16];

	const ContainerPartTableModel* thisCpy = this;
	memset(lDragIdentifier, 0, sizeof(lDragIdentifier));
	memcpy(lDragIdentifier, (const char*) &thisCpy, sizeof(ContainerPartTableModel*));
	memcpy(lDragIdentifier + sizeof(ContainerPartTableModel*), &dragIdentifierCounter, 1);

	if (memcmp(dragIdentifier, lDragIdentifier, sizeof(dragIdentifier))  ==  0) {
		// Dropped onto itself
		return true;
	}

	quint32 size;
	stream >> size;

	QList<ContainerPart> parts;

	for (quint32 i = 0 ; i < size ; i++) {
		PartCategory* cat;
		stream.readRawData((char*) &cat, sizeof(PartCategory*));

		unsigned int id;
		stream >> id;

		ContainerPart part;
		part.cat = cat;
		part.id = id;
		parts << part;
	}

	System::getInstance()->signalDropAccepted(action, dragIdentifier);
	emit partsDropped(parts);

	return true;
}


QMimeData* ContainerPartTableModel::mimeData(const QModelIndexList& indices) const
{
	QMimeData* mimeData = new QMimeData;

	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	const_cast<ContainerPartTableModel*>(this)->dragIdentifierCounter++;

	char dragIdentifier[16];

	const ContainerPartTableModel* thisCpy = this;
	memset(dragIdentifier, 0, sizeof(dragIdentifier));
	memcpy(dragIdentifier, (const char*) &thisCpy, sizeof(ContainerPartTableModel*));
	memcpy(dragIdentifier + sizeof(ContainerPartTableModel*), &dragIdentifierCounter, 1);

	stream.writeRawData(dragIdentifier, sizeof(dragIdentifier));

	quint32 size = indices.size();
	stream << size;

	ContainerPartTableModel* ncThis = const_cast<ContainerPartTableModel*>(this);
	ncThis->draggedParts.clear();

	for (QModelIndexList::const_iterator it = indices.begin() ; it != indices.end() ; it++) {
		QModelIndex idx = *it;

		if (idx.column() != 0)
			continue;

		int row = idx.row();
		const ContainerPart& part = parts[row];

		ncThis->draggedParts << part;

		stream.writeRawData((const char*) &part.cat, sizeof(PartCategory*));

		stream << part.id;
	}

	mimeData->setData("application/nx-electronics-partrecords", data);
	return mimeData;
}


void ContainerPartTableModel::dropAccepted(Qt::DropAction action, const char* dragIdentifier)
{
	char lDragIdentifier[16];

	const ContainerPartTableModel* thisCpy = this;
	memset(lDragIdentifier, 0, sizeof(lDragIdentifier));
	memcpy(lDragIdentifier, (const char*) &thisCpy, sizeof(ContainerPartTableModel*));
	memcpy(lDragIdentifier + sizeof(ContainerPartTableModel*), &dragIdentifierCounter, 1);

	if (memcmp(dragIdentifier, lDragIdentifier, sizeof(lDragIdentifier))  !=  0)
		return;

	if (action == Qt::MoveAction)
		emit partsDragged(draggedParts);
}


ContainerPartTableModel::ContainerPart ContainerPartTableModel::getIndexPart(const QModelIndex& idx) const
{
	if (!idx.isValid()) {
		ContainerPart part;
		part.cat = NULL;
		part.id = INVALID_PART_ID;
		return part;
	}

	int row = idx.row();
	return parts[row];
}


QModelIndex ContainerPartTableModel::getIndexFromPart(PartCategory* cat, unsigned int id) const
{
	for (unsigned int i = 0 ; i < parts.size() ; i++) {
		const ContainerPart& part = parts[i];

		if (part.cat == cat  &&  part.id == id) {
			return index(i, 0);
		}
	}

	return QModelIndex();
}



