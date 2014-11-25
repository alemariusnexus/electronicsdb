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

#include "PropertyLineEdit.h"
#include <nxcommon/exception/InvalidValueException.h>



PropertyLineEdit::PropertyLineEdit(PartProperty* prop, QWidget* parent)
		: PlainLineEdit(parent), cat(prop->getPartCategory()), prop(prop), state(Enabled), flags(0)
{
	unsigned int maxLen = prop->getStringMaximumLength();

	if (maxLen != 0) {
		setMaxLength(maxLen);
	}

	connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(textEdited(const QString&)));

	setTextValid(true);
}


void PropertyLineEdit::setState(DisplayWidgetState state)
{
	this->state = state;

	applyState();
}


void PropertyLineEdit::setFlags(flags_t flags)
{
	this->flags = flags;

	applyState();
}


QList<QString> PropertyLineEdit::getValues() const
{
	flags_t flags = prop->getFlags();

	return prop->parseUserInputValue(text());
}


void PropertyLineEdit::applyState()
{
	if (state == Enabled) {
		if ((flags & ChoosePart)  !=  0) {
			setEnabled(true);
			setReadOnly(true);
		} else {
			setEnabled(true);
			setReadOnly(false);
		}
	} else if (state == Disabled) {
		setEnabled(false);
	} else if (state == ReadOnly) {
		setEnabled(true);
		setReadOnly(true);
	}
}


void PropertyLineEdit::setTextValid(bool valid)
{
	setStyleSheet(
			QString("\
			QLineEdit { \
			    border: 1px solid %1; \
			    border-radius: 2px; \
			} \
			QLineEdit:focus { \
			    border: 1px solid %2; \
			    border-radius: 2px; \
			} \
			").arg(valid ? "grey" : "red").arg(valid ? "blue" : "red"));
}


void PropertyLineEdit::textEdited(const QString& text)
{
	try {
		prop->parseUserInputValue(text);
		setTextValid(true);
	} catch (InvalidValueException& ex) {
		setTextValid(false);
	}

	emit editedByUser(text);
}
