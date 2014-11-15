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

#ifndef PROPERTYLINKEDITWIDGET_H_
#define PROPERTYLINKEDITWIDGET_H_

#include "global.h"
#include "PartProperty.h"
#include "PartCategory.h"
#include "PlainLineEdit.h"
#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>



class PropertyLinkEditWidget : public QWidget
{
	Q_OBJECT

public:
	PropertyLinkEditWidget(PartProperty* prop, bool compactMode = false, QWidget* parent = NULL, QObject* keyEventFilter = NULL);
	void displayLink(unsigned int linkId);
	void setState(DisplayWidgetState state);
	void setFlags(flags_t flags);
	unsigned int getLink() const;
	QString getValue() const;
	void setValueFocus();

private:
	void applyState();

private slots:
	void idFieldEdited(const QString& text);
	void chooseButtonClicked();
	void gotoButtonClicked();

signals:
	void changedByUser();

private:
	PartCategory* cat;
	PartProperty* prop;

	DisplayWidgetState state;
	flags_t flags;

	PlainLineEdit* linkIdField;
	QPushButton* linkChooseButton;
	QLabel* linkDescLabel;
	QPushButton* linkGotoButton;
};

#endif /* PROPERTYLINKEDITWIDGET_H_ */
