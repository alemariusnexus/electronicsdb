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

#ifndef PARTEDITCOMMAND_H_
#define PARTEDITCOMMAND_H_

#include "global.h"
#include "EditCommand.h"
#include "SQLCommand.h"
#include "PartCategory.h"
#include <QtCore/QString>
#include <QtCore/QList>


class PartEditCommand : public EditCommand
{
public:
	PartEditCommand(PartCategory* partCat);
	~PartEditCommand();
	void addSQLCommand(SQLCommand* cmd);
	void setDescription(const QString& desc);

	PartCategory* getPartCategory() { return partCat; }
	virtual QString getDescription() const { return desc; }

	virtual void commit();
	virtual void revert();

private:
	PartCategory* partCat;
	QString desc;
	QList<SQLCommand*> cmds;
};

#endif /* PARTEDITCOMMAND_H_ */
