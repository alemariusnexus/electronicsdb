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

#ifndef SQLUPDATECOMMAND_H_
#define SQLUPDATECOMMAND_H_

#include "global.h"
#include "SQLCommand.h"
#include <QtCore/QString>
#include <QtCore/QMap>



class SQLUpdateCommand : public SQLCommand
{
public:
	SQLUpdateCommand(const QString& tableName, const QString& idField, unsigned int id);
	void addFieldValue(const QString& fieldName, const QString& newValue);

	virtual void commit();
	virtual void revert();

private:
	QString tableName;
	QString idField;
	unsigned int id;
	QMap<QString, QString> values;

	QString revertQuery;
};

#endif /* SQLUPDATECOMMAND_H_ */
