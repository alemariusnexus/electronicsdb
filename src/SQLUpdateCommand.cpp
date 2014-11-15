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

#include "SQLUpdateCommand.h"
#include <nxcommon/exception/InvalidValueException.h>



SQLUpdateCommand::SQLUpdateCommand(const QString& tableName, const QString& idField, unsigned int id)
		: tableName(tableName), idField(idField), id(id)
{
}


void SQLUpdateCommand::addFieldValue(const QString& fieldName, const QString& newValue)
{
	values.insert(fieldName, newValue);
}


void SQLUpdateCommand::commit()
{
	SQLDatabase sql = getSQLConnection();

	QString query("SELECT ");

	for (QMap<QString, QString>::iterator it = values.begin() ; it != values.end() ; it++) {
		QString fieldName = it.key();

		if (it != values.begin()) {
			query += ",";
		}

		query += fieldName;
	}

	query += QString(" FROM %1 WHERE %2=%3")
			.arg(tableName)
			.arg(idField)
			.arg(id);

	SQLResult res = sql.sendQuery(query);

	if (!res.isValid()) {
		throw SQLException("Error storing SQL result", __FILE__, __LINE__);
	}

	if (!res.nextRecord()) {
		throw InvalidValueException (
				QString("Invalid number of rows returned for SQLUpdateCommand::commit() SELECT query").toAscii().constData(),
				__FILE__, __LINE__);
	}

	revertQuery = QString("UPDATE %1 SET ")
			.arg(tableName);

	unsigned int i = 0;
	for (QMap<QString, QString>::iterator it = values.begin() ; it != values.end() ; it++, i++) {
		QString fieldName = it.key();

		if (it != values.begin()) {
			revertQuery += ",";
		}

		revertQuery += QString("%1='%2'").arg(fieldName).arg(sql.escapeString(res.getString(i)));
	}

	revertQuery += QString(" WHERE %1=%2")
			.arg(idField)
			.arg(id);


	query = QString("UPDATE %1 SET ")
			.arg(tableName);

	i = 0;
	for (QMap<QString, QString>::iterator it = values.begin() ; it != values.end() ; it++, i++) {
		QString fieldName = it.key();
		QString val = it.value();

		if (it != values.begin()) {
			query += ",";
		}

		query += QString("%1='%2'").arg(fieldName).arg(sql.escapeString(val));
	}

	query += QString(" WHERE %1=%2")
			.arg(idField)
			.arg(id);

	sql.sendQuery(query);
}


void SQLUpdateCommand::revert()
{
	SQLDatabase sql = getSQLConnection();

	sql.sendQuery(revertQuery);
}
