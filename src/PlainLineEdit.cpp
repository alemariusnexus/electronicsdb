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

#include "PlainLineEdit.h"
#include "System.h"
#include "EditStack.h"



void PlainLineEdit::keyPressEvent(QKeyEvent* evt)
{
	EditStack* editStack = System::getInstance()->getEditStack();

	if (evt->matches(QKeySequence::Undo)) {
		if (editStack->canUndo()) {
			editStack->undo();
			evt->accept();
		} else {
			evt->ignore();
		}

		return;
	} else if (evt->matches(QKeySequence::Redo)) {
		if (editStack->canRedo()) {
			editStack->redo();
			evt->accept();
		} else {
			evt->ignore();
		}

		return;
	}

	QLineEdit::keyPressEvent(evt);
}
