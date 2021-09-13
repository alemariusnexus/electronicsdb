/*
    Copyright 2010-2021 David "Alemarius Nexus" Lerch

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

#include "CompoundEditCommand.h"

#include <nxcommon/exception/InvalidValueException.h>

namespace electronicsdb
{


CompoundEditCommand::~CompoundEditCommand()
{
    for (EditCommand* cmd : cmds) {
        delete cmd;
    }
}

QString CompoundEditCommand::getDescription() const
{
    return desc;
}

bool CompoundEditCommand::wantsSQLTransaction()
{
    bool transaction = false;
    for (EditCommand* cmd : cmds) {
        if (cmd->wantsSQLTransaction()) {
            transaction = true;
            break;
        }
    }
    return transaction;
}

QSqlDatabase CompoundEditCommand::getSQLDatabase() const
{
    return cmds.empty() ? QSqlDatabase() : cmds[0]->getSQLDatabase();
}

void CompoundEditCommand::commit()
{
    for (int i = 0 ; i < cmds.size() ; i++) {
        cmds[i]->commit();
    }

    EditCommand::commit();
}

void CompoundEditCommand::revert()
{
    for (int i = cmds.size()-1 ; i >= 0 ; i--) {
        cmds[i]->revert();
    }

    EditCommand::revert();
}

void CompoundEditCommand::setDescription(const QString& desc)
{
    this->desc = desc;
}

void CompoundEditCommand::addCommand(EditCommand* cmd)
{
    if (!cmds.empty()  &&  cmd->getSQLDatabase().connectionName() != cmds[0]->getSQLDatabase().connectionName()) {
        throw InvalidValueException("All components of CompoundEditCommand must share the same database connection",
                                    __FILE__, __LINE__);
    }
    cmds << cmd;
}


}
