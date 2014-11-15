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
		: tableName(tableName), idField(idField)
{
}


void SQLDeleteCommand::addRecords(QList<QString> ids)
{
	this->ids.append(ids);
}


void SQLDeleteCommand::addRecords(QList<unsigned int> ids)
{
	for (unsigned int i = 0 ; i < ids.size() ; i++) {
		this->ids << QString("%1").arg(ids[i]);
	}
}


void SQLDeleteCommand::commit()
{
	SQLDatabase sql = getSQLConnection();

	QList<QMap<QString, QString> > colInfo = SQLListColumns(sql, tableName);

	QString nameCode("");
	QString valueCode("");

	unsigned int nc = 0;

	for (QList<QMap<QString, QString> >::iterator it = colInfo.begin() ; it != colInfo.end() ; it++) {
		QMap<QString, QString> col = *it;
		if (nc != 0)
			nameCode += ",";

		nameCode += col["name"];
		nc++;
	}

	QString idListStr("");

	for (unsigned int i = 0 ; i < ids.size() ; i++) {
		if (i != 0)
			idListStr += ",";

		idListStr += QString("'%1'").arg(ids[i]);
	}


	QString query = QString("SELECT %1 FROM %2 WHERE %3 IN(%4)")
			.arg(nameCode)
			.arg(tableName)
			.arg(idField)
			.arg(idListStr);

	SQLResult res = sql.sendQuery(query);

	if (!res.isValid()) {
		throw SQLException("Error storing SQL result", __FILE__, __LINE__);
	}

	unsigned int numRemoved = 0;

	bool first = true;
	while (res.nextRecord()) {
		if (!first)
			valueCode +=",";

		valueCode += "(";

		for (unsigned int i = 0 ; i < nc ; i++) {
			if (i != 0) {
				valueCode += ",";
			}

			valueCode += QString("'%1'").arg(sql.escapeString(res.getString(i)));
		}

		valueCode += ")";

		first = false;
		numRemoved++;
	}

	if (numRemoved != 0) {
		revertQuery = QString("INSERT INTO %1 (%2) VALUES %3")
				.arg(tableName)
				.arg(nameCode)
				.arg(valueCode);

		query = QString("DELETE FROM %1 WHERE %2 IN(%3)")
				.arg(tableName)
				.arg(idField)
				.arg(idListStr);

		sql.sendQuery(query);
	} else {
		revertQuery = QString();
	}
}


void SQLDeleteCommand::revert()
{
	if (revertQuery.isEmpty())
		return;

	SQLDatabase sql = getSQLConnection();

	sql.sendQuery(revertQuery);
}
