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

#pragma once

#include "../../global.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QWidget>
#include <ui_PropertyDateTimeEdit.h>
#include "../../model/part/Part.h"
#include "../../model/PartProperty.h"

namespace electronicsdb
{


class PropertyDateTimeEdit : public QWidget
{
    Q_OBJECT

public:
    PropertyDateTimeEdit(PartProperty* prop, QWidget* parent = nullptr, QObject* keyEventFilter = nullptr);

    void display(const Part& part, const QVariant& rawVal);
    QVariant getValue() const;

    void setState(DisplayWidgetState state);
    void setFlags(flags_t flags);
    void setValueFocus();

private slots:
    void updateState();
    void dateTimeChangedSlot(const QDateTime& dateTime);
    void nowRequested();

signals:
    void editedByUser();

private:
    Ui_PropertyDateTimeEdit ui;

    PartProperty* prop;

    DisplayWidgetState state;
    flags_t flags;
};


}
