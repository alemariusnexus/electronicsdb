/*
    Copyright 2010-2021 David "Alemarius Nexus" Lerch

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

#include <nxcommon/exception/InvalidValueException.h>
#include "../../db/DatabaseConnection.h"
#include "../../model/part/PartFactory.h"
#include "../../System.h"

namespace electronicsdb
{


PropertyComboBox::PropertyComboBox(PartProperty* prop, QWidget* parent)
        : QComboBox(parent), cat(prop->getPartCategory()), prop(prop), state(Enabled), flags(0),
          programmaticallyChanging(false)
{
    setEditable(true);

    int maxLen = prop->getStringMaximumLength();

    if (maxLen >= 0) {
        lineEdit()->setMaxLength(maxLen);
    }

    connect(this, &PropertyComboBox::editTextChanged, this, &PropertyComboBox::editTextChangedSlot);


    System* sys = System::getInstance();

    connect(&PartFactory::getInstance(), &PartFactory::partsChanged, this, &PropertyComboBox::partsChanged);

    setTextValid(true);

    updateEnum();
}

void PropertyComboBox::displayRecord(const Part& part, const QVariant& rawVal)
{
    programmaticallyChanging = true;

    if (!part) {
        setEditText(prop->formatSingleValueForEditing(QVariant()));
    } else {
        setEditText(prop->formatSingleValueForEditing(rawVal));
    }

    programmaticallyChanging = false;

    currentPart = part;

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
        for (QVariant value : prop->getDistinctValues()) {
            QString displayStr = prop->formatSingleValueForEditing(value);
            addItem(displayStr, value);
        }
    }

    setEditText(oldText);

    programmaticallyChanging = false;
}

void PropertyComboBox::setTextValid(bool valid)
{
    if (!currentPart) {
        valid = true;
    }

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
            ").arg(valid ? "grey" : "red",
                   valid ? "blue" : "red"));
}

QString PropertyComboBox::getUserValue() const
{
    return currentText();
}

QVariant PropertyComboBox::getRawValue() const
{
    QString text = currentText();
    return prop->parseSingleUserInputValue(text);
}

void PropertyComboBox::partsChanged(const PartList& inserted, const PartList& updated, const PartList& deleted)
{
    updateEnum();
}

void PropertyComboBox::editTextChangedSlot(const QString& text)
{
    try {
        getRawValue();
        setTextValid(true);
    } catch (InvalidValueException&) {
        setTextValid(false);
    }

    if (!programmaticallyChanging)
        emit changedByUser(text);
}


}
