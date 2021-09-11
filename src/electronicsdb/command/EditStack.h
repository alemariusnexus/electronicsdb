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

#pragma once

#include "../global.h"

#include <list>
#include <QObject>
#include <QString>
#include "../db/DatabaseConnection.h"
#include "../db/SQLTransaction.h"
#include "EditCommand.h"

namespace electronicsdb
{


class EditStack : public QObject
{
    Q_OBJECT

public:
    typedef std::list<EditCommand*> EditList;

public:
    EditStack(int undoStackSize = 100, int redoStackSize = 100);
    ~EditStack();

    void setUndoStackSize(int undoStackSize);
    void setRedoStackSize(int redoStackSize);

    int getUndoStackSize() const { return undoStackSize; }
    int getRedoStackSize() const { return redoStackSize; }

    int getUndoStackUsedSize() const { return static_cast<int>(undoStack.size()); }
    int getRedoStackUsedSize() const { return static_cast<int>(redoStack.size()); }

    void push(EditCommand* cmd);

    bool canUndo() { return !undoStack.empty(); }
    bool canRedo() { return !redoStack.empty(); }

    EditCommand* getUndoTop() { return canUndo() ? undoStack.back() : nullptr; }
    EditCommand* getRedoTop() { return canRedo() ? redoStack.back() : nullptr; }

public slots:
    void undo();
    void redo();

    bool undoUntil(EditCommand* cmd);
    bool redoUntil(EditCommand* cmd);

    void clear();

signals:
    void undone(EditCommand* cmd);
    void redone(EditCommand* cmd);
    void canUndoStatusChanged(bool canUndo);
    void canRedoStatusChanged(bool canRedo);

private:
    void freeUndoStack(int minFreeSlots = 1);
    void freeRedoStack(int minFreeSlots = 1);

    void notifyChanges();

    void commitCommand(EditCommand* cmd);
    void revertCommand(EditCommand* cmd);

private:
    EditList undoStack;
    EditList redoStack;
    int undoStackSize;
    int redoStackSize;
};


}
