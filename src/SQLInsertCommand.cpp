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

#include "SQLInsertCommand.h"
#include "System.h"
#include <nxcommon/exception/InvalidStateException.h>



SQLInsertCommand::SQLInsertCommand(const QString& tableName, const QString& idField)
		: tableName(tableName), idField(idField), insId(0)
{
}


void SQLInsertCommand::addRecord(QMap<QString, QString> data)
{
	records << data;
}


void SQLInsertCommand::commit()
{
	if (records.empty())
		return;

	SQLDatabase sql = getSQLConnection();

	QString nameCode("");
	QString valueCode("");

	QMap<QString, QString> firstRec = records[0];

	for (QMap<QString, QString>::iterator it = firstRec.begin() ; it != firstRec.end() ; it++) {
		QString n = it.key();

		if (it != firstRec.begin())
			nameCode += ",";

		nameCode += n;
	}

	for (QList<QMap<QString, QString> >::iterator it = records.begin() ; it != records.end() ; it++) {
		QMap<QString, QString> fieldValues = *it;

		if (it != records.begin())
			valueCode += ",";

		valueCode += "(";

		for (QMap<QString, QString>::iterator it = fieldValues.begin() ; it != fieldValues.end() ; it++) {
			QString n = it.key();
			QString v = it.value();

			if (it != fieldValues.begin()) {
				valueCode += ",";
			}

			valueCode += QString("'%1'").arg(sql.escapeString(v));
		}

		valueCode += ")";
	}

	QString query = QString("INSERT INTO %1 (%2) VALUES %3")
			.arg(tableName)
			.arg(nameCode)
			.arg(valueCode);

	sql.sendQuery(query);

	insId = sql.getLastInsertID();
}


void SQLInsertCommand::revert()
{
	if (records.empty())
		return;

	SQLDatabase sql = getSQLConnection();

	if (insId == 0) {
		throw InvalidStateException("Attempting to revert an SQLInsertCommand that wasn't committed!",
				__FILE__, __LINE__);
	}

	unsigned int maxInsId = insId + records.size() - 1;

	QString query = QString("DELETE FROM %1 WHERE %2>=%3 AND %2<=%4")
			.arg(tableName)
			.arg(idField)
			.arg(insId)
			.arg(maxInsId);

	sql.sendQuery(query);

	insId = 0;
}
