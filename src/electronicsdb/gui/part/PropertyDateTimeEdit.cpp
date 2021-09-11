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

#include "PropertyDateTimeEdit.h"

#include <QLocale>

namespace electronicsdb
{


PropertyDateTimeEdit::PropertyDateTimeEdit(PartProperty* prop, QWidget* parent, QObject* keyEventFilter)
        : QWidget(parent), prop(prop), state(Enabled), flags(0)
{
    ui.setupUi(this);

    ui.dateTimeEdit->installEventFilter(keyEventFilter);

    QLocale loc;

    PartProperty::Type type = prop->getType();

    if (type == PartProperty::Date) {
        ui.dateTimeEdit->setDisplayFormat(loc.dateFormat(QLocale::ShortFormat));
    } else if (type == PartProperty::Time) {
        ui.dateTimeEdit->setDisplayFormat(loc.timeFormat(QLocale::ShortFormat));
    } else if (type == PartProperty::DateTime) {
        ui.dateTimeEdit->setDisplayFormat(loc.dateTimeFormat(QLocale::ShortFormat));
    } else {
        assert(false);
    }

    connect(ui.dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &PropertyDateTimeEdit::dateTimeChangedSlot);
    connect(ui.nowButton, &QToolButton::clicked, this, &PropertyDateTimeEdit::nowRequested);

    updateState();
}

void PropertyDateTimeEdit::display(const Part& part, const QVariant& rawVal)
{
    PartProperty::Type type = prop->getType();

    ui.dateTimeEdit->blockSignals(true);

    if (type == PartProperty::Date) {
        ui.dateTimeEdit->setDate(rawVal.toDate());
    } else if (type == PartProperty::Time) {
        ui.dateTimeEdit->setTime(rawVal.toTime());
    } else if (type == PartProperty::DateTime) {
        ui.dateTimeEdit->setDateTime(rawVal.toDateTime());
    } else {
        assert(false);
    }

    ui.dateTimeEdit->blockSignals(false);
}

QVariant PropertyDateTimeEdit::getValue() const
{
    PartProperty::Type type = prop->getType();

    if (type == PartProperty::Date) {
        return ui.dateTimeEdit->date();
    } else if (type == PartProperty::Time) {
        return ui.dateTimeEdit->time();
    } else if (type == PartProperty::DateTime) {
        return ui.dateTimeEdit->dateTime();
    } else {
        assert(false);
        return QVariant();
    }
}

void PropertyDateTimeEdit::setState(DisplayWidgetState state)
{
    this->state = state;
    updateState();
}

void PropertyDateTimeEdit::setFlags(flags_t flags)
{
    this->flags = flags;
    updateState();
}

void PropertyDateTimeEdit::updateState()
{
    if (state == Enabled) {
        if ((flags & ChoosePart) != 0) {
            ui.dateTimeEdit->setEnabled(true);
            ui.dateTimeEdit->setReadOnly(true);
            ui.nowButton->setEnabled(false);
        } else {
            ui.dateTimeEdit->setEnabled(true);
            ui.dateTimeEdit->setReadOnly(false);
            ui.nowButton->setEnabled(true);
        }
    } else if (state == ReadOnly) {
        ui.dateTimeEdit->setEnabled(true);
        ui.dateTimeEdit->setReadOnly(true);
        ui.nowButton->setEnabled(false);
    } else if (state == Disabled) {
        ui.dateTimeEdit->setEnabled(false);
        ui.dateTimeEdit->setReadOnly(false);
        ui.nowButton->setEnabled(false);
    }
}

void PropertyDateTimeEdit::dateTimeChangedSlot(const QDateTime& dateTime)
{
    emit editedByUser();
}

void PropertyDateTimeEdit::nowRequested()
{
    ui.dateTimeEdit->setDateTime(QDateTime::currentDateTime());
}

void PropertyDateTimeEdit::setValueFocus()
{
    ui.dateTimeEdit->setFocus();
}


}
