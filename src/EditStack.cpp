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

#include "EditStack.h"
#include "System.h"



EditStack::EditStack(unsigned int undoStackSize, unsigned int redoStackSize)
		: undoStackSize(undoStackSize), redoStackSize(redoStackSize)
{
	System* sys = System::getInstance();

	connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)));
}


EditStack::~EditStack()
{
	clear();
}


void EditStack::clear()
{
	bool undoWasEmpty = undoStack.empty();
	bool redoWasEmpty = redoStack.empty();

	while (!undoStack.empty()) {
		delete undoStack.first();
		undoStack.removeFirst();
	}
	while (!redoStack.empty()) {
		delete redoStack.first();
		redoStack.removeFirst();
	}

	if (!undoWasEmpty)
		emit canUndoStatusChanged(false);
	if (!redoWasEmpty)
		emit canRedoStatusChanged(false);
}


void EditStack::push(EditCommand* cmd)
{
	bool redoWasEmpty = redoStack.empty();

	for (EditList::iterator it = redoStack.begin() ; it != redoStack.end() ; it++) {
		delete *it;
	}
	redoStack.clear();

	cmd->commit();

	freeUndoStack();
	undoStack << cmd;

	if (!redoWasEmpty)
		emit canRedoStatusChanged(false);
	if (undoStack.size() == 1)
		emit canUndoStatusChanged(true);
}


void EditStack::undo()
{
	if (undoStack.empty())
		return;

	EditCommand* cmd = undoStack.last();
	undoStack.pop_back();

	cmd->revert();

	freeRedoStack();
	redoStack << cmd;

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

	EditCommand* cmd = redoStack.last();
	redoStack.pop_back();

	cmd->commit();

	freeUndoStack();
	undoStack << cmd;

	emit redone(cmd);

	if (redoStack.empty())
		emit canRedoStatusChanged(false);
	if (undoStack.size() == 1)
		emit canUndoStatusChanged(true);
}


void EditStack::freeUndoStack()
{
	while (undoStack.size() >= undoStackSize) {
		delete undoStack.first();
		undoStack.removeFirst();
	}
}


void EditStack::freeRedoStack()
{
	while (redoStack.size() >= redoStackSize) {
		delete redoStack.first();
		redoStack.removeFirst();
	}
}


void EditStack::databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
	clear();
}
