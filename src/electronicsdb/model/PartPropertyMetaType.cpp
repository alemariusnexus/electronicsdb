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

#include "PartPropertyMetaType.h"

#include <limits>

namespace electronicsdb
{


PartPropertyMetaType::PartPropertyMetaType (
        const QString& metaTypeID,
        PartProperty::Type coreType,
        PartPropertyMetaType* parent
)       : parent(parent), metaTypeID(metaTypeID), coreType(coreType), flags(0), flagsWithNonNullValue(0),
          unitPrefixType(PartProperty::UnitPrefixInvalid), strMaxLen(-1),
          intRangeMin(std::numeric_limits<int64_t>::max()), intRangeMax(std::numeric_limits<int64_t>::min()),
          floatRangeMin(std::numeric_limits<double>::max()), floatRangeMax(std::numeric_limits<double>::lowest()),
          textAreaMinHeight(-1), sortIndex(10000), defaultValueFunc(PartProperty::DefaultValueConst)
{
}

PartPropertyMetaType::PartPropertyMetaType(const PartPropertyMetaType& other)
        : parent(other.parent), metaTypeID(other.metaTypeID), coreType(other.coreType),
          userReadableName(other.userReadableName), flags(other.flags),
          flagsWithNonNullValue(other.flagsWithNonNullValue),
          ftUserPrefix(other.ftUserPrefix),
          unitSuffix(other.unitSuffix), unitPrefixType(other.unitPrefixType),
          sqlNaturalOrderCode(other.sqlNaturalOrderCode), sqlAscendingOrderCode(other.sqlAscendingOrderCode),
          sqlDescendingOrderCode(other.sqlDescendingOrderCode),
          strMaxLen(other.strMaxLen),
          boolTrueText(other.boolTrueText), boolFalseText(other.boolFalseText),
          intRangeMin(other.intRangeMin), intRangeMax(other.intRangeMax),
          floatRangeMin(other.floatRangeMin), floatRangeMax(other.floatRangeMax),
          textAreaMinHeight(other.textAreaMinHeight),
          sortIndex(other.sortIndex),
          defaultValue(other.defaultValue), defaultValueFunc(other.defaultValueFunc)
{
}


}
