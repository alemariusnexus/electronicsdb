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

#include "ContainerRemovePartsCommand.h"
#include "System.h"
#include "SQLMultiValueInsertCommand.h"
#include "SQLDeleteCommand.h"
#include "SQLAdvancedDeleteCommand.h"
#include <QtCore/QMap>
#include <cstdlib>




ContainerRemovePartsCommand::ContainerRemovePartsCommand()
		: cid(0), allContainers(true)
{
}


void ContainerRemovePartsCommand::setContainerID(unsigned int cid)
{
	this->cid = cid;
}


void ContainerRemovePartsCommand::setAllContainers(bool allConts)
{
	allContainers = allConts;
}


void ContainerRemovePartsCommand::addPart(PartCategory* cat, unsigned int pid)
{
	ContainerPart part;
	part.cat = cat;
	part.pid = pid;
	parts << part;
}


void ContainerRemovePartsCommand::commit()
{
	committedCmds.clear();

	QString whereClause("");

	if (!allContainers) {
		whereClause += QString("cid=%1").arg(cid);
	} else {
		whereClause += "1=0";
	}

	for (ContainerPart part : parts) {
		whereClause += QString(" OR (ptype='%1' AND pid=%2)").arg(part.cat->getID()).arg(part.pid);
	}

	SQLAdvancedDeleteCommand* delCmd = new SQLAdvancedDeleteCommand("container_part", whereClause);
	committedCmds << delCmd;

	for (SQLCommand* cmd : committedCmds) {
		cmd->commit();
	}

	/*SQLDatabase sql = getSQLConnection();

	System* sys = System::getInstance();

	committedCmds.clear();

	QMap<QString, PartCategory*> catMap;

	for (System::PartCategoryIterator it = sys->getPartCategoryBegin() ; it != sys->getPartCategoryEnd() ; it++) {
		PartCategory* cat = *it;
		catMap[cat->getID()] = cat;
	}

	QList<unsigned int> cids;

	if (allContainers) {
		SQLResult res = sql.sendQuery(u"SELECT id FROM container");

		while (res.nextRecord()) {
			cids << res.getUInt32(0);
		}
	} else {
		cids << cid;
	}

	for (unsigned int cid : cids) {
		QString query = QString("SELECT ptype, pid FROM container_part WHERE cid='%1'").arg(cid);

		SQLResult res = sql.sendQuery(query);

		SQLMultiValueInsertCommand* insCmd = new SQLMultiValueInsertCommand("container_part", "cid", cid);

		while (res.nextRecord()) {
			QString ptype = res.getString(0);
			unsigned int id = res.getUInt32(1);

			PartCategory* cat = catMap[ptype];

			bool removed = false;
			for (ContainerPart part : parts) {
				if (part.cat == cat  &&  part.pid == id) {
					removed = true;
					break;
				}
			}

			if (!removed) {
				QMap<QString, QString> data;
				data["ptype"] = cat->getID();
				data["pid"] = QString("%1").arg(id);
				insCmd->addValue(data);
			}
		}

		SQLDeleteCommand* delCmd = new SQLDeleteCommand("container_part", "cid");
		delCmd->addRecord(cid);

		committedCmds << delCmd;
		committedCmds << insCmd;
	}

	uint64_t s = GetTickcount();

	for (SQLCommand* cmd : committedCmds) {
		cmd->commit();
	}

	uint64_t e = GetTickcount();

	printf("It took %ums\n", (unsigned int) (e-s));*/
}


void ContainerRemovePartsCommand::revert()
{
	for (int i = committedCmds.size()-1 ; i >= 0 ; i--) {
		SQLCommand* cmd = committedCmds[i];
		cmd->revert();
	}

	committedCmds.clear();
}
