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

#ifndef PROPERTYMULTIVALUEWIDGET_H_
#define PROPERTYMULTIVALUEWIDGET_H_

#include "global.h"
#include "PartProperty.h"
#include "PartCategory.h"
#include "PropertyLineEdit.h"
#include "PropertyTextEdit.h"
#include "PropertyLinkEditWidget.h"
#include "PropertyComboBox.h"
#include "PropertyFileWidget.h"
#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QCheckBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>



class PropertyMultiValueWidget : public QWidget
{
	Q_OBJECT

public:
	PropertyMultiValueWidget(PartProperty* prop, QWidget* parent = NULL, QObject* keyEventFilter = NULL);

	void display(unsigned int id, QList<QString> rawVals);
	void setState(DisplayWidgetState state);
	void setFlags(flags_t flags);
	QList<QString> getValues(bool initialValueOnError = false) const;
	void focusValueWidget();

private:
	void applyState();

private slots:
	void currentItemChanged(QListWidgetItem* newItem, QListWidgetItem* oldItem);
	void addRequested();
	void removeRequested();
	void boolFieldToggled(bool val);
	void valueFieldEdited(const QString&);
	void textEditEdited();
	void linkWidgetChangedByUser();
	void fileChanged();

signals:
	void changedByUser();

private:
	PartCategory* cat;
	PartProperty* prop;

	DisplayWidgetState state;
	flags_t flags;

	unsigned int currentID;

	QListWidget* listWidget;
	QPushButton* listAddButton;
	QPushButton* listRemoveButton;
	QCheckBox* boolBox;
	PropertyTextEdit* textEdit;
	PropertyLineEdit* lineEdit;
	PropertyLinkEditWidget* linkWidget;
	PropertyComboBox* enumBox;
	PropertyFileWidget* fileWidget;
};

#endif /* PROPERTYMULTIVALUEWIDGET_H_ */
