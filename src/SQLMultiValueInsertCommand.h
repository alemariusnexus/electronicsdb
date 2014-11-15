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

#ifndef SQLMULTIVALUEINSERTCOMMAND_H_
#define SQLMULTIVALUEINSERTCOMMAND_H_

#include "global.h"
#include "SQLCommand.h"
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>



/**
 * CAUTION: This class expects that there are no preexisting multi-value entries with the given ID. If this expectation
 * 		is not met, reverting this command will delete ALL of the multi-values for that given ID, even if they were not
 * 		inserted by this command!
 */
class SQLMultiValueInsertCommand : public SQLCommand
{
public:
	SQLMultiValueInsertCommand(const QString& tableName, const QString& idField, unsigned int id);
	void addValues(QList<QMap<QString, QString> > vals);
	void addValue(QMap<QString, QString> val) { addValues(QList<QMap<QString, QString> >() << val); }

	virtual void commit();
	virtual void revert();

private:
	QString tableName;
	QString idField;
	unsigned int id;
	QList<QMap<QString, QString> > values;
	//QList<QString> values;
};

#endif /* SQLMULTIVALUEINSERTCOMMAND_H_ */
