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

#include "../global.h"

#include <QList>
#include <QString>
#include <QVariant>
#include "PartProperty.h"

namespace electronicsdb
{


struct PartPropertyMetaType
{
    static QList<PartPropertyMetaType*> cloneDeep(const QList<PartPropertyMetaType*>& mtypes);

    PartPropertyMetaType (
            const QString& metaTypeID,
            PartProperty::Type coreType = PartProperty::InvalidType,
            PartPropertyMetaType* parent = nullptr
            );
    PartPropertyMetaType(const PartPropertyMetaType& other);

    PartPropertyMetaType* parent;
    QString metaTypeID;
    PartProperty::Type coreType;

    QString userReadableName;
    flags_t flags;
    flags_t flagsWithNonNullValue;

    QString ftUserPrefix;

    QString unitSuffix;
    PartProperty::UnitPrefixType unitPrefixType;

    QString sqlNaturalOrderCode;
    QString sqlAscendingOrderCode;
    QString sqlDescendingOrderCode;

    int strMaxLen;

    QString boolTrueText;
    QString boolFalseText;

    int64_t intRangeMin;
    int64_t intRangeMax;
    double floatRangeMin;
    double floatRangeMax;

    int textAreaMinHeight;

    int sortIndex;

    QVariant defaultValue;
    PartProperty::DefaultValueFunction defaultValueFunc;
};


}
