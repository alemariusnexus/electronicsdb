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

#ifndef SQLNEWRECORDCOMMAND_H_
#define SQLNEWRECORDCOMMAND_H_

#include "SQLCommand.h"
#include "PartCategory.h"
#include "SQLInsertCommand.h"
#include <QtCore/QList>


class SQLNewRecordCommand : public SQLCommand
{
public:
	SQLNewRecordCommand(PartCategory* partCat, PartCategory::DataMap data);
	virtual ~SQLNewRecordCommand();
	virtual void commit();
	virtual void revert();

	unsigned int getLastInsertID() const { return insId; }

private:
	PartCategory* partCat;
	PartCategory::DataMap data;
	QList<SQLCommand*> committedCmds;
	unsigned int insId;
};

#endif /* SQLNEWRECORDCOMMAND_H_ */
