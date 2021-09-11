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

#include <limits>
#include <QDataStream>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include "AbstractPartProperty.h"

namespace electronicsdb
{

class PartCategory;
struct PartPropertyMetaType;


class PartProperty : public AbstractPartProperty
{
    Q_OBJECT

    friend class PartCategory;

public:
    enum Type
    {
        InvalidType,

        String,
        Integer,
        Float,
        Decimal,
        Boolean,
        File,
        Date,
        Time,
        DateTime
    };

    enum Flags
    {
        FullTextIndex                   = 0x0001,
        MultiValue                      = 0x0002,
        SIPrefixesDefaultToBase2        = 0x0004,

        DisplayWithUnits                = 0x00010000,
        DisplaySelectionList            = 0x00020000,
        DisplayHideFromListingTable     = 0x00040000,
        DisplayTextArea                 = 0x00080000,
        DisplayMultiInSingleField       = 0x00100000,
        DisplayDynamicEnum              = 0x00200000,

        // These are the flags that will be used when neither the property's own meta type nor any of the parent meta
        // types explicitly sets a value. In that way, they are a kind of default setting.
        FlagsImplicitValue = FullTextIndex | DisplayWithUnits | DisplaySelectionList
    };

    enum UnitPrefixType
    {
        UnitPrefixInvalid,

        UnitPrefixNone,
        UnitPrefixSIBase10,
        UnitPrefixSIBase2,
        UnitPrefixPercent,
        UnitPrefixPPM
    };

    enum IntegerStorageType
    {
        Int8,
        Int16,
        Int24,
        Int32,
        Int64
    };

    enum DefaultValueFunction
    {
        DefaultValueConst,

        DefaultValueNow
    };

    struct CustomMetaType {};

public:
    static bool isValidFieldName(const QString& fname);

public:
    PartProperty (
            const QString& fieldName,
            PartPropertyMetaType* parentMetaType,
            PartCategory* cat = nullptr
            );
    PartProperty (
            const QString& fieldName,
            PartPropertyMetaType* customMetaType,
            PartCategory* cat,
            CustomMetaType
    );
    PartProperty (
            const QString& fieldName,
            Type coreType,
            PartCategory* cat = nullptr
    );
    ~PartProperty();

    PartCategory* getPartCategory() { return cat; }

    void setFieldName(const QString& fieldName);
    QString getFieldName() const { return fieldName; }

    PartPropertyMetaType* getMetaType() { return metaType; }
    const PartPropertyMetaType* getMetaType() const { return metaType; }

    Type getType() const;

    void setUserReadableName(const QString& name);
    QString getUserReadableName() const;
    flags_t getFlags() const;

    void setFullTextSearchUserPrefix(const QString& pfx);
    QString getFullTextSearchUserPrefix() const;

    void setUnitSuffix(const QString& suffix);
    QString getUnitSuffix() const;

    void setUnitPrefixType(UnitPrefixType type);
    UnitPrefixType getUnitPrefixType() const;

    void setSQLNaturalOrderCode(const QString& oc);
    QString getSQLNaturalOrderCode() const;
    void setSQLAscendingOrderCode(const QString& oc);
    QString getSQLAscendingOrderCode() const;
    void setSQLDescendingOrderCode(const QString& oc);
    QString getSQLDescendingOrderCode() const;

    void setStringMaximumLength(int len);
    int getStringMaximumLength() const;

    void setBooleanTrueDisplayText(const QString& text);
    void setBooleanFalseDisplayText(const QString& text);
    QString getBooleanTrueDisplayText() const;
    QString getBooleanFalseDisplayText() const;

    void setIntegerMinimum(int64_t val);
    int64_t getIntegerMinimum() const;
    void setIntegerMaximum(int64_t val);
    int64_t getIntegerMaximum() const;
    void setDecimalMinimum(double val);
    double getDecimalMinimum() const;
    void setDecimalMaximum(double val);
    double getDecimalMaximum() const;
    double getFloatMinimum() const;
    double getFloatMaximum() const;

    void setTextAreaMinimumHeight(int h);
    int getTextAreaMinimumHeight() const;

    void setSortIndex(int sortIdx);
    int getSortIndex() const;

    void setDefaultValue(const QVariant& val);
    void setDefaultValueFunction(DefaultValueFunction func);
    void setDefaultValueFromType();
    QVariant getDefaultValue() const;
    DefaultValueFunction getDefaultValueFunction() const;
    QString getDefaultValueSQLExpression() const;


    void setStringStorageMaximumLength(int len) { stringStorageMaxLen = len; }
    int getStringStorageMaximumLength() const { return stringStorageMaxLen; }

    void setIntegerStorageRangeMinimum(int64_t val) { intStorageRangeMin = val; }
    int64_t getIntegerStorageRangeMinimum() const { return intStorageRangeMin; }
    void setIntegerStorageRangeMaximum(int64_t val) { intStorageRangeMax = val; }
    int64_t getIntegerStorageRangeMaximum() const { return intStorageRangeMax; }

    QString getMultiValueForeignTableName() const;
    QString getMultiValueIDFieldName() const { return "pid"; }

    QList<QString> formatValuesForDisplay(const QList<QVariant>& values) const;
    QString formatValueForDisplay(const QVariant& value) const;
    QString formatSingleValueForDisplay(const QVariant& value) const;

    QString formatValueForEditing(const QVariant& value) const;
    QString formatSingleValueForEditing(const QVariant& value) const;

    QVariant parseUserInputValue(const QString& userStr) const;
    QVariant parseSingleUserInputValue(const QString& userStr) const;

    QList<QVariant> getDistinctValues() const;

    int getSortIndexInCategory(PartCategory* pcat) const override;
    void setSortIndexInCategory(PartCategory* pcat, int sortIdx) override;

private:
    PartProperty(const PartProperty& other, PartCategory* newCat);

    void init();

    void checkDatabaseConnection() const;

    bool isValidIntegerValue(int64_t value) const;
    double isValidFloatValue(double value) const;

private:
    PartCategory* cat;
    QString fieldName;

    PartPropertyMetaType* metaType;

    QString sqlAscendingOrderCodeDefault;
    QString sqlDescendingOrderCodeDefault;

    int stringStorageMaxLen;
    int64_t intStorageRangeMin;
    int64_t intStorageRangeMax;

    friend class PartCategory;
};


}

Q_DECLARE_METATYPE(electronicsdb::PartProperty*)
Q_DECLARE_OPAQUE_POINTER(electronicsdb::PartProperty*)

// Required for moving pointers around in QAbstractItemView using drag and drop
QDataStream& operator<<(QDataStream& s, const electronicsdb::PartProperty* prop);
QDataStream& operator>>(QDataStream& s, electronicsdb::PartProperty*& prop);
