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

#ifndef EDITSTACK_H_
#define EDITSTACK_H_

#include "EditCommand.h"
#include "DatabaseConnection.h"
#include <QtCore/QObject>
#include <QtCore/QLinkedList>
#include <QtCore/QString>



class EditStack : public QObject
{
	Q_OBJECT

public:
	typedef QLinkedList<EditCommand*> EditList;

public:
	EditStack(unsigned int undoStackSize = 100, unsigned int redoStackSize = 100);
	~EditStack();
	void clear();
	void push(EditCommand* cmd);
	bool canUndo() { return !undoStack.empty(); }
	bool canRedo() { return !redoStack.empty(); }

public slots:
	void undo();
	void redo();

signals:
	void undone(EditCommand* cmd);
	void redone(EditCommand* cmd);
	void canUndoStatusChanged(bool canUndo);
	void canRedoStatusChanged(bool canRedo);

private slots:
	void databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);

private:
	void freeUndoStack();
	void freeRedoStack();

private:
	EditList undoStack;
	EditList redoStack;
	unsigned int undoStackSize;
	unsigned int redoStackSize;
};

#endif /* EDITSTACK_H_ */
