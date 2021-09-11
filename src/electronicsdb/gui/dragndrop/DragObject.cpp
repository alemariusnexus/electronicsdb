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

#include "DragObject.h"

#include "../../command/CompoundEditCommand.h"
#include "../../command/EditStack.h"
#include "../../System.h"

namespace electronicsdb
{


DragObject::DragObject(const QString& context, const QVariant& dragPayload)
        : uuid(QUuid::createUuid()), context(context), dragPayload(dragPayload)
{
}

void DragObject::setDragPayload(const QVariant& payload)
{
    dragPayload = payload;
}

void DragObject::setDropPayload(const QVariant& payload)
{
    dropPayload = payload;
}

DragObject::~DragObject()
{
    for (EditCommand* cmd : acceptCmds) {
        delete cmd;
    }
}

void DragObject::accept()
{
    emit accepted();

    EditStack* editStack = System::getInstance()->getEditStack();

    if (acceptCmds.size() == 1) {
        editStack->push(acceptCmds[0]);
    } else if (acceptCmds.size() > 1) {
        CompoundEditCommand* compCmd = new CompoundEditCommand;
        for (EditCommand* cmd : acceptCmds) {
            compCmd->addCommand(cmd);
        }
        editStack->push(compCmd);
    }

    acceptCmds.clear();
}

void DragObject::enqueueAcceptCommand(EditCommand* cmd)
{
    if (!cmd) {
        return;
    }

    acceptCmds << cmd;
}


}
