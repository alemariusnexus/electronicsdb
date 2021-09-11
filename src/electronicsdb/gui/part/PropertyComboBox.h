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

#include <QComboBox>
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../../model/PartProperty.h"

namespace electronicsdb
{


class PropertyComboBox : public QComboBox
{
    Q_OBJECT

public:
    PropertyComboBox(PartProperty* prop, QWidget* parent = nullptr);
    void displayRecord(const Part& part, const QVariant& rawVal);
    void setState(DisplayWidgetState state);
    void setFlags(flags_t flags);
    QString getUserValue() const;
    QVariant getRawValue() const;

private:
    void applyState();
    void updateEnum();
    void setTextValid(bool valid);

private slots:
    void partsChanged(const PartList& inserted, const PartList& updated, const PartList& deleted);
    void editTextChangedSlot(const QString& text);

signals:
    void changedByUser(const QString& text);

private:
    PartCategory* cat;
    PartProperty* prop;

    DisplayWidgetState state;
    flags_t flags;

    bool programmaticallyChanging;

    Part currentPart;
};


}
