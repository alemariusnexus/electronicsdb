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

#include "ContainerEditCommand.h"



ContainerEditCommand::ContainerEditCommand()
{
}


ContainerEditCommand::~ContainerEditCommand()
{
	for (unsigned int i = 0 ; i < cmds.size() ; i++) {
		delete cmds[i];
	}
}


void ContainerEditCommand::addSQLCommand(SQLCommand* cmd)
{
	cmds << cmd;
}


void ContainerEditCommand::setDescription(const QString& desc)
{
	this->desc = desc;
}


void ContainerEditCommand::commit()
{
	for (QList<SQLCommand*>::iterator it = cmds.begin() ; it != cmds.end() ; it++) {
		SQLCommand* cmd = *it;
		cmd->commit();
	}
}


void ContainerEditCommand::revert()
{
	for (int i = cmds.size()-1 ; i >= 0 ; i--) {
		SQLCommand* cmd = cmds[i];
		cmd->revert();
	}
}
