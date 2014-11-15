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

#ifndef PROPERTYFILEWIDGET_H_
#define PROPERTYFILEWIDGET_H_

#include "global.h"
#include "PartProperty.h"
#include "PartCategory.h"
#include "PlainLineEdit.h"
#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>



class PropertyFileWidget : public QWidget
{
	Q_OBJECT

public:
	PropertyFileWidget(PartProperty* prop, QWidget* parent = NULL, QObject* keyEventFilter = NULL);
	void displayFile(unsigned int partID, const QString& fileName);
	void setState(DisplayWidgetState state);
	void setFlags(flags_t flags);
	void setValueFocus();
	QString getValue() const;



private:
	void applyState();
	QString getCurrentAbsolutePath(bool mustExist = false);

private slots:
	void fileChosen();
	void textEdited(const QString& text);
	void openRequested();
	void copyClipboardRequested();

signals:
	void changedByUser();

private:
	PartCategory* cat;
	PartProperty* prop;

	unsigned int currentPartID;

	DisplayWidgetState displayState;
	flags_t displayFlags;

	PlainLineEdit* fileEdit;
	QPushButton* chooseButton;
	QPushButton* openButton;
	QPushButton* clipboardButton;
};

#endif /* PROPERTYFILEWIDGET_H_ */
