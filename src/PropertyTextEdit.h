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

#ifndef PROPERTYTEXTEDIT_H_
#define PROPERTYTEXTEDIT_H_

#include "global.h"
#include "PartProperty.h"
#include "PartCategory.h"
#include <QtGui/QPlainTextEdit>
#include <QtGui/QKeyEvent>



class PropertyTextEdit : public QPlainTextEdit
{
	Q_OBJECT

public:
	PropertyTextEdit(PartProperty* prop, QWidget* parent = NULL);
	void setText(const QString& text);
	void setState(DisplayWidgetState state);
	void setFlags(flags_t flags);
	QList<QString> getValues() const;

	virtual QSize sizeHint() const;

protected:
	virtual void keyPressEvent(QKeyEvent* evt);

private:
	void setTextValid(bool valid);
	void applyState();

private slots:
	void textChanged();

signals:
	void editedByUser(const QString& text);

private:
	PartCategory* cat;
	PartProperty* prop;

	DisplayWidgetState state;
	flags_t flags;

	bool programmaticallyChanging;
};

#endif /* PROPERTYTEXTEDIT_H_ */
