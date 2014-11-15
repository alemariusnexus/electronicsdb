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

#ifndef PROPERTYLINEEDIT_H_
#define PROPERTYLINEEDIT_H_

#include "global.h"
#include "PartProperty.h"
#include "PartCategory.h"
#include "PlainLineEdit.h"


class PropertyLineEdit : public PlainLineEdit
{
	Q_OBJECT

public:
	PropertyLineEdit(PartProperty* prop, QWidget* parent = NULL);
	void setState(DisplayWidgetState state);
	void setFlags(flags_t flags);
	QList<QString> getValues() const;

private:
	void setTextValid(bool valid);
	void applyState();

private slots:
	void textEdited(const QString& text);

signals:
	void editedByUser(const QString& text);

private:
	PartCategory* cat;
	PartProperty* prop;

	DisplayWidgetState state;
	flags_t flags;
};

#endif /* PROPERTYLINEEDIT_H_ */
