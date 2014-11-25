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

#include "PropertyComboBox.h"
#include "DatabaseConnection.h"
#include "System.h"
#include <nxcommon/exception/InvalidValueException.h>



PropertyComboBox::PropertyComboBox(PartProperty* prop, QWidget* parent)
		: QComboBox(parent), cat(prop->getPartCategory()), prop(prop), state(Enabled), flags(0),
		  programmaticallyChanging(false), currentID(INVALID_PART_ID)
{
	setEditable(true);

	unsigned int maxLen = prop->getStringMaximumLength();

	if (maxLen != 0) {
		lineEdit()->setMaxLength(maxLen);
	}

	connect(this, SIGNAL(editTextChanged(const QString&)), this, SLOT(editTextChangedSlot(const QString&)));


	System* sys = System::getInstance();

	connect(cat, SIGNAL(recordEdited(unsigned int, PartCategory::DataMap)),
			this, SLOT(recordEdited(unsigned int, PartCategory::DataMap)));
	connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)));
	connect(sys, SIGNAL(partCategoriesChanged()), this, SLOT(partCategoriesChanged()));

	setTextValid(true);

	updateEnum();
}


void PropertyComboBox::displayRecord(unsigned int id, const QString& rawVal)
{
	programmaticallyChanging = true;

	if (id == INVALID_PART_ID) {
		setEditText(prop->formatValueForEditing(QList<QString>()));
	} else {
		setEditText(prop->formatValueForEditing(QList<QString>() << rawVal));
	}

	programmaticallyChanging = false;

	currentID = id;

	setTextValid(true);
}


void PropertyComboBox::setState(DisplayWidgetState state)
{
	this->state = state;

	applyState();
}


void PropertyComboBox::setFlags(flags_t flags)
{
	this->flags = flags;

	applyState();
}


void PropertyComboBox::applyState()
{
	DisplayWidgetState state = this->state;

	if ((flags & ChoosePart)  !=  0) {
		state = ReadOnly;
	}

	if (state == Enabled) {
		setEnabled(true);
	} else if (state == Disabled) {
		setEnabled(false);
	} else if (state == ReadOnly) {
		setEnabled(false);
	}
}


void PropertyComboBox::updateEnum()
{
	programmaticallyChanging = true;

	QString oldText = currentText();
	clear();

	System* sys = System::getInstance();

	if (sys->hasValidDatabaseConnection()) {
		SQLDatabase sql = sys->getCurrentSQLDatabase();

		QString query;

		PartProperty::Type type = prop->getType();

		if ((prop->getFlags() & PartProperty::MultiValue)  !=  0) {
			query = QString("SELECT DISTINCT %1.%2 FROM %1 %3")
					.arg(prop->getMultiValueForeignTableName())
					.arg(prop->getFieldName())
					.arg(prop->getSQLNaturalOrderCode());
		} else {
			query = QString("SELECT DISTINCT %1.%2 FROM %1 %3")
					.arg(cat->getTableName())
					.arg(prop->getFieldName())
					.arg(prop->getSQLNaturalOrderCode());
		}

		SQLResult res = sql.sendQuery(query);

		while (res.nextRecord()) {
			QString val = res.getString(0);
			QString displayStr = prop->formatValueForEditing(QList<QString>() << val);

			addItem(displayStr, val);
		}
	}

	setEditText(oldText);

	programmaticallyChanging = false;
}


void PropertyComboBox::setTextValid(bool valid)
{
	if (currentID == INVALID_PART_ID)
		valid = true;

	setStyleSheet(
			QString("\
			QComboBox { \
			    border: 1px solid %1; \
			    border-radius: 2px; \
			} \
			QComboBox:focus { \
			    border: 1px solid %2; \
			    border-radius: 2px; \
			} \
			").arg(valid ? "grey" : "red").arg(valid ? "blue" : "red"));
}


QString PropertyComboBox::getUserValue() const
{
	return currentText();
}


QString PropertyComboBox::getRawValue() const
{
	QString text = currentText();

	return prop->parseUserInputValue(text)[0];
}


void PropertyComboBox::recordEdited(unsigned int id, PartCategory::DataMap newData)
{
	updateEnum();
}


void PropertyComboBox::editTextChangedSlot(const QString& text)
{
	try {
		getRawValue();
		setTextValid(true);
	} catch (InvalidValueException& ex) {
		setTextValid(false);
	}

	if (!programmaticallyChanging)
		emit changedByUser(text);
}


void PropertyComboBox::databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
}


void PropertyComboBox::partCategoriesChanged()
{
	updateEnum();
}
