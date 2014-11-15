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

#ifndef SQLDELETECOMMAND_H_
#define SQLDELETECOMMAND_H_

#include "global.h"
#include "SQLCommand.h"
#include <QtCore/QString>
#include <QtCore/QList>



class SQLDeleteCommand : public SQLCommand
{
public:
	SQLDeleteCommand(const QString& tableName, const QString& idField);

	void addRecords(QList<QString> ids);
	void addRecords(QList<unsigned int> ids);
	void addRecord(unsigned int id) { addRecords(QList<unsigned int>() << id); }
	void addRecord(const QString& id) { addRecords(QList<QString>() << id); }

	virtual void commit();
	virtual void revert();

private:
	QString tableName;
	QString idField;
	QList<QString> ids;

	QString revertQuery;
};

#endif /* SQLDELETECOMMAND_H_ */
