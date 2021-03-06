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

#ifndef COMPOUNDEDITCOMMAND_H_
#define COMPOUNDEDITCOMMAND_H_

#include "global.h"
#include "EditCommand.h"
#include <QtCore/QList>



class CompoundEditCommand : public EditCommand
{
public:
	virtual QString getDescription() const;
	virtual void commit();
	virtual void revert();

	void setDescription(const QString& desc);
	void addCommand(EditCommand* cmd);

private:
	QString desc;
	QList<EditCommand*> cmds;
};

#endif /* COMPOUNDEDITCOMMAND_H_ */
