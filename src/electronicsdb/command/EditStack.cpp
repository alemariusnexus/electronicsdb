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

#include "EditStack.h"

#include <algorithm>
#include <memory>
#include <QSqlDatabase>
#include <nxcommon/log.h>
#include <nxcommon/exception/InvalidValueException.h>
#include "../db/SQLDatabaseWrapperFactory.h"
#include "../System.h"

namespace electronicsdb
{


EditStack::EditStack(int undoStackSize, int redoStackSize)
        : undoStackSize(undoStackSize), redoStackSize(redoStackSize)
{
    if (undoStackSize <= 0) {
        // We can't allow zero, because that would delete commands immediately before push() returns, which
        // means that the caller doesn't have a chance to read back their results.
        throw InvalidValueException("Undo stack size must be greater than 0!", __FILE__, __LINE__);
    }

    System* sys = System::getInstance();

    connect(sys, &System::databaseConnectionAboutToChange, this, &EditStack::clear);
    connect(sys, &System::appModelAboutToBeReset, this, &EditStack::clear);
}

EditStack::~EditStack()
{
    clear();
}

void EditStack::setUndoStackSize(int undoStackSize)
{
    this->undoStackSize = undoStackSize;
    freeUndoStack(0);
}

void EditStack::setRedoStackSize(int redoStackSize)
{
    this->redoStackSize = redoStackSize;
    freeRedoStack(0);
}

void EditStack::clear()
{
    bool undoWasEmpty = undoStack.empty();
    bool redoWasEmpty = redoStack.empty();

    while (!undoStack.empty()) {
        delete undoStack.front();
        undoStack.pop_front();
    }
    while (!redoStack.empty()) {
        delete redoStack.front();
        redoStack.pop_front();
    }

    if (!undoWasEmpty)
        emit canUndoStatusChanged(false);
    if (!redoWasEmpty)
        emit canRedoStatusChanged(false);
}

void EditStack::push(EditCommand* cmd)
{
    if (!cmd) {
        return;
    }

    bool redoWasEmpty = redoStack.empty();

    for (EditCommand* rcmd : redoStack) {
        delete rcmd;
    }
    redoStack.clear();

    commitCommand(cmd);

    freeUndoStack();
    undoStack.push_back(cmd);

    if (!redoWasEmpty)
        emit canRedoStatusChanged(false);
    if (undoStack.size() == 1)
        emit canUndoStatusChanged(true);
}

void EditStack::undo()
{
    if (undoStack.empty())
        return;

    EditCommand* cmd = undoStack.back();
    undoStack.pop_back();

    revertCommand(cmd);

    freeRedoStack();
    redoStack.push_back(cmd);

    emit undone(cmd);

    if (undoStack.empty())
        emit canUndoStatusChanged(false);
    if (redoStack.size() == 1)
        emit canRedoStatusChanged(true);
}

void EditStack::redo()
{
    if (redoStack.empty())
        return;

    EditCommand* cmd = redoStack.back();
    redoStack.pop_back();

    commitCommand(cmd);

    freeUndoStack();
    undoStack.push_back(cmd);

    emit redone(cmd);

    if (redoStack.empty())
        emit canRedoStatusChanged(false);
    if (undoStack.size() == 1)
        emit canUndoStatusChanged(true);
}

bool EditStack::undoUntil(EditCommand* cmd)
{
    while (canUndo()  &&  getUndoTop() != cmd) {
        undo();
    }
    return getUndoTop() == cmd;
}

bool EditStack::redoUntil(EditCommand* cmd)
{
    while (canRedo()  &&  getRedoTop() != cmd) {
        redo();
    }
    return getRedoTop() == cmd;
}

void EditStack::freeUndoStack(int minFreeSlots)
{
    minFreeSlots = std::min(minFreeSlots, undoStackSize);
    while (undoStackSize - getUndoStackUsedSize() < minFreeSlots) {
        delete undoStack.front();
        undoStack.pop_front();
    }
}

void EditStack::freeRedoStack(int minFreeSlots)
{
    minFreeSlots = std::min(minFreeSlots, redoStackSize);
    while (redoStackSize - getRedoStackUsedSize() < minFreeSlots) {
        delete redoStack.front();
        redoStack.pop_front();
    }
}

void EditStack::notifyChanges()
{
    System::getInstance()->processChangelogs();
}

void EditStack::commitCommand(EditCommand* cmd)
{
    QSqlDatabase db = cmd->getSQLDatabase();

    if (db.isValid()  &&  cmd->wantsSQLTransaction()) {
        std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));
        auto trans = dbw->beginTransaction();

        cmd->commit();

        trans.commit();
    } else {
        cmd->commit();
    }

    notifyChanges();
}

void EditStack::revertCommand(EditCommand* cmd)
{
    QSqlDatabase db = cmd->getSQLDatabase();

    if (db.isValid()  &&  cmd->wantsSQLTransaction()) {
        std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));
        auto trans = dbw->beginTransaction();

        cmd->revert();

        trans.commit();
    } else {
        cmd->revert();
    }

    notifyChanges();
}


}
