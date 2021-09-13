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

#include "SQLEditCommand.h"

#include <nxcommon/exception/InvalidValueException.h>

namespace electronicsdb
{


SQLEditCommand::SQLEditCommand()
{
}

SQLEditCommand::~SQLEditCommand()
{
    for (SQLCommand* cmd : cmds) {
        delete cmd;
    }
}

void SQLEditCommand::addSQLCommand(SQLCommand* cmd)
{
    if (!cmds.empty()  &&  cmd->getSQLConnection().connectionName() != cmds[0]->getSQLConnection().connectionName()) {
        throw InvalidValueException("All components of SQLEditCommand must share the same database connection",
                                    __FILE__, __LINE__);
    }
    cmds << cmd;
}

QSqlDatabase SQLEditCommand::getSQLDatabase() const
{
    return cmds.empty() ? QSqlDatabase() : cmds[0]->getSQLConnection();
}

void SQLEditCommand::commit()
{
    // NOTE: Do NOT turn this into a range- or iterator-based loop, because commands may be added during execution
    // (e.g. by commit listeners).
    for (int i = 0 ; i < cmds.size() ; i++) {
        cmds[i]->commit();
    }

    EditCommand::commit();
}

void SQLEditCommand::revert()
{
    for (int i = cmds.size()-1 ; i >= 0 ; i--) {
        SQLCommand* cmd = cmds[i];
        cmd->revert();
    }

    EditCommand::revert();
}


}
