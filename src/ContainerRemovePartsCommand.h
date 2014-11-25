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

#ifndef CONTAINERREMOVEPARTSCOMMAND_H_
#define CONTAINERREMOVEPARTSCOMMAND_H_

#include "global.h"
#include "ContainerEditCommand.h"
#include "PartCategory.h"
#include <QtCore/QList>


class ContainerRemovePartsCommand : public SQLCommand
{
private:
	struct ContainerPart
	{
		PartCategory* cat;
		unsigned int pid;
	};

public:
	ContainerRemovePartsCommand();
	virtual ~ContainerRemovePartsCommand() {}

	void setContainerID(unsigned int cid);
	void setAllContainers(bool allConts);
	void addPart(PartCategory* cat, unsigned int pid);

	virtual void commit();
	virtual void revert();

private:
	QString desc;
	unsigned int cid;
	bool allContainers;
	QList<ContainerPart> parts;

	QList<SQLCommand*> committedCmds;
};

#endif /* CONTAINERREMOVEPARTSCOMMAND_H_ */
