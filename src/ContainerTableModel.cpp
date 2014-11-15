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

#include "ContainerTableModel.h"
#include "System.h"
#include "NoDatabaseConnectionException.h"
#include <nxcommon/exception/InvalidValueException.h>
#include "elutil.h"
#include "SQLUpdateCommand.h"
#include "ContainerEditCommand.h"
#include <cstdlib>
#include <QtCore/QMap>





ContainerTableModel::ContainerTableModel(QObject* parent)
		: QAbstractTableModel(parent), orderAscending(true)
{

}


void ContainerTableModel::setFilter(const QString& sqlLikeFilter)
{
	currentFilter = sqlLikeFilter;

	reload();
}


void ContainerTableModel::reload()
{
	containers.clear();

	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection()) {
		reset();
		return;
	}

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	QString query = QString (
			"SELECT id, name, ptype, pid "
			"FROM container "
			"LEFT JOIN container_part "
			"    ON container.id = container_part.cid "
			"WHERE name LIKE '%1' "
			"ORDER BY name %2")
			.arg(currentFilter.isEmpty() ? "%" : (QString) sql.escapeString(currentFilter))
			.arg(orderAscending ? "ASC" : "DESC");

	SQLResult res = sql.sendQuery(query);

	QMap<QString, PartCategory*> catMap;
	QMap<PartCategory*, QList<unsigned int> > catIdMap;

	for (System::PartCategoryIterator it = sys->getPartCategoryBegin() ; it != sys->getPartCategoryEnd() ; it++) {
		PartCategory* cat = *it;
		catMap[cat->getTableName()] = cat;
	}

	while (res.nextRecord()) {
		unsigned int id = res.getUInt32(0);

		if (containers.empty()  ||  containers.last().id != id) {
			ContainerEntry cont;
			cont.id = id;
			containers << cont;
		}

		ContainerEntry& cont = containers.last();
		cont.name = res.getString(1);
		cont.id = id;

		QString ptype = res.getString(2);

		if (ptype.isEmpty())
			continue;

		QMap<QString, PartCategory*>::iterator it = catMap.find(ptype);

		if (it == catMap.end()) {
			throw InvalidValueException(QString("Invalid part category name: %1").arg(ptype).toUtf8().constData(),
					__FILE__, __LINE__);
		}

		PartCategory* cat = it.value();

		PartEntry entry;
		entry.cat = cat;
		entry.id = res.getUInt32(3);

		catIdMap[cat] << entry.id;

		cont.parts << entry;
	}

	for (QMap<PartCategory*, QList<unsigned int> >::iterator it = catIdMap.begin() ; it != catIdMap.end() ; it++) {
		PartCategory* cat = it.key();
		QList<unsigned int> ids = it.value();

		QMap<unsigned int, QString> descs = cat->getDescriptions(ids);

		for (ContainerIterator cit = containers.begin() ; cit != containers.end() ; cit++) {
			ContainerEntry& cont = *cit;

			for (PartIterator pit = cont.parts.begin() ; pit != cont.parts.end() ; pit++) {
				PartEntry& part = *pit;

				if (part.cat == cat) {
					part.desc = descs[part.id];
				}
			}
		}
	}

	reset();
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

	const ContainerEntry& cont = containers[row];

	if (col == 0) {
		if (role == Qt::DisplayRole) {
			return cont.name;
		}
	} else if (col == 1) {
		if (role == Qt::DisplayRole) {
			QString desc("");

			for (ConstPartIterator it = cont.parts.begin() ; it != cont.parts.end() ; it++) {
				if (!desc.isEmpty())
					desc += ", ";

				const PartEntry& part = *it;

				if (part.desc.isNull())
					desc += QString("%1").arg(part.id);
				else
					desc += part.desc;

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

	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

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

	for (unsigned int i = 0 ; i < containers.size() ; i++) {
		ContainerEntry& cont = containers[i];

		if (cont.name == name  &&  i != row) {
			return false;
		}
	}

	ContainerEntry& cont = containers[row];

	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection()) {
		reset();
		return false;
	}

	ContainerEditCommand* cmd = new ContainerEditCommand;

	SQLUpdateCommand* baseCmd = new SQLUpdateCommand("container", "id", cont.id);
	baseCmd->addFieldValue("name", name);
	cmd->addSQLCommand(baseCmd);

	EditStack* editStack = sys->getEditStack();

	editStack->push(cmd);

	sys->signalContainersChanged();

	return true;
}


unsigned int ContainerTableModel::getIndexID(const QModelIndex& idx) const
{
	int row = idx.row();

	const ContainerEntry& cont = containers[row];
	return cont.id;
}


QModelIndex ContainerTableModel::getIndexFromID(unsigned int id) const
{
	for (unsigned int i = 0 ; i < containers.size() ; i++) {
		const ContainerEntry& cont = containers[i];

		if (cont.id == id) {
			return index(i, 0);
		}
	}

	return QModelIndex();
}



