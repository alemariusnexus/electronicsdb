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

#include "SQLMultiValueInsertCommand.h"
#include "System.h"
#include <nxcommon/exception/InvalidStateException.h>



SQLMultiValueInsertCommand::SQLMultiValueInsertCommand(const QString& tableName, const QString& idField,
		unsigned int id)
		: tableName(tableName), idField(idField), id(id)
{
}


void SQLMultiValueInsertCommand::addValues(QList<QMap<QString, QString> > vals)
{
	values.append(vals);
}


void SQLMultiValueInsertCommand::commit()
{
	if (values.empty())
		return;

	SQLDatabase sql = getSQLConnection();

	QString valueCode("");

	QMap<QString, QString> firstVal = values[0];

	QString nameStr = QString("%1").arg(idField);

	for (QMap<QString, QString>::iterator it = firstVal.begin() ; it != firstVal.end() ; it++) {
		nameStr += ',';
		nameStr += it.key();
	}

	for (QList<QMap<QString, QString> >::iterator it = values.begin() ; it != values.end() ; it++) {
		QMap<QString, QString> vals = *it;

		if (it != values.begin())
			valueCode += ",";

		valueCode += "(";

		valueCode += QString("%1").arg(id);

		for (QMap<QString, QString>::iterator iit = vals.begin() ; iit != vals.end() ; iit++) {
			valueCode += ',';
			valueCode += QString("'%1'").arg(iit.value());
		}

		valueCode += ")";
	}

	QString query = QString("INSERT INTO %1 (%2) VALUES %3")
			.arg(tableName)
			.arg(nameStr)
			.arg(valueCode);

	sql.sendQuery(query);
}


void SQLMultiValueInsertCommand::revert()
{
	if (values.empty())
		return;

	SQLDatabase sql = getSQLConnection();

	QString query = QString("DELETE FROM %1 WHERE %2=%3")
			.arg(tableName)
			.arg(idField)
			.arg(id);

	sql.sendQuery(query);
}
