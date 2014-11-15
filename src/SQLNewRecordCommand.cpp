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

#include "SQLNewRecordCommand.h"




SQLNewRecordCommand::SQLNewRecordCommand(PartCategory* partCat, PartCategory::DataMap data)
		: partCat(partCat), data(data), insId(0)
{
}


SQLNewRecordCommand::~SQLNewRecordCommand()
{
	for (int i = committedCmds.size()-1 ; i >= 0 ; i--) {
		SQLCommand* cmd = committedCmds[i];
		delete cmd;
	}
}


void SQLNewRecordCommand::commit()
{
	SQLInsertCommand* baseCmd = new SQLInsertCommand(partCat->getTableName(), partCat->getIDField());

	QMap<QString, QString> record;

	for (PartCategory::DataMap::iterator it = data.begin() ; it != data.end() ; it++) {
		PartProperty* prop = it.key();
		QList<QString> vals = it.value();

		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::MultiValue)  ==  0) {
			record[prop->getFieldName()] = vals[0];
		}
	}

	baseCmd->addRecord(record);
	baseCmd->commit();

	committedCmds << baseCmd;

	unsigned int id = baseCmd->getFirstInsertID();

	for (PartCategory::DataMap::iterator it = data.begin() ; it != data.end() ; it++) {
		PartProperty* prop = it.key();
		QList<QString> vals = it.value();

		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::MultiValue)  !=  0) {
			SQLInsertCommand* insCmd = new SQLInsertCommand(prop->getMultiValueForeignTableName(),
					prop->getMultiValueIDFieldName());

			for (QList<QString>::iterator iit = vals.begin() ; iit != vals.end() ; iit++) {
				QString val = *iit;

				QMap<QString, QString> mvRecord;
				mvRecord[prop->getMultiValueIDFieldName()] = QString("%1").arg(id);
				mvRecord[prop->getFieldName()] = val;

				insCmd->addRecord(mvRecord);
			}

			insCmd->commit();

			committedCmds << insCmd;
		}
	}

	insId = id;
}


void SQLNewRecordCommand::revert()
{
	for (int i = committedCmds.size()-1 ; i >= 0 ; i--) {
		SQLCommand* cmd = committedCmds[i];
		cmd->revert();
		delete cmd;
	}

	committedCmds.clear();
	insId = 0;
}
