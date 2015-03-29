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

#include "SQLDeleteCommand.h"
#include <nxcommon/exception/InvalidValueException.h>
#include "sqlutils.h"
#include <QtCore/QString>
#include <QtCore/QMap>
#include <nxcommon/sql/sql.h>



SQLDeleteCommand::SQLDeleteCommand(const QString& tableName, const QString& idField)
		: SQLAdvancedDeleteCommand(tableName, QString("1=0")), idField(idField)
{
}


void SQLDeleteCommand::addRecords(QList<QString> ids)
{
	this->ids.append(ids);
	rebuildWhereClause();
}


void SQLDeleteCommand::addRecords(QList<unsigned int> ids)
{
	for (unsigned int i = 0 ; i < ids.size() ; i++) {
		this->ids << QString("%1").arg(ids[i]);
	}

	rebuildWhereClause();
}



void SQLDeleteCommand::rebuildWhereClause()
{
	if (ids.empty()) {
		// Empty lists for an IN(...) expression are not allowed, so we just use an expression that evaluates to false.
		setWhereClause("1=0");
	} else {
		QString idListStr("");

		for (unsigned int i = 0 ; i < ids.size() ; i++) {
			if (i != 0)
				idListStr += ",";

			idListStr += QString("'%1'").arg(ids[i]);
		}

		setWhereClause(QString("%1 IN (%2)").arg(idField).arg(idListStr));
	}
}
