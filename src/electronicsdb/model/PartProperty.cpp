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

#include "PartProperty.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <unordered_map>
#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QTime>
#include <QVariant>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/exception/InvalidValueException.h>
#include "../exception/NoDatabaseConnectionException.h"
#include "../exception/SQLException.h"
#include "../util/elutil.h"
#include "../System.h"
#include "part/Part.h"
#include "part/PartFactory.h"
#include "PartCategory.h"
#include "PartPropertyMetaType.h"

namespace electronicsdb
{


bool PartProperty::isValidFieldName(const QString& fname)
{
    static QRegularExpression regex("^[a-zA-Z0-9_]+$");
    static QSet<QString> reservedIDs({"id"});
    return regex.match(fname).hasMatch()  &&  !reservedIDs.contains(fname)  &&  !fname.startsWith("__");
}


PartProperty::PartProperty (
        const QString& fieldName,
        PartPropertyMetaType* parentMetaType,
        PartCategory* cat
)       : cat(nullptr), fieldName(fieldName),
          metaType(new PartPropertyMetaType(QString(), InvalidType, parentMetaType)),
          stringStorageMaxLen(std::numeric_limits<int>::max()),
          intStorageRangeMin(std::numeric_limits<int64_t>::min()),
          intStorageRangeMax(std::numeric_limits<int64_t>::max())
{
    init();

    if (cat) {
        cat->addProperty(this);
        assert(this->cat == cat);
    }
}

PartProperty::PartProperty (
        const QString& fieldName,
        PartPropertyMetaType* customMetaType,
        PartCategory* cat,
        CustomMetaType
)       : cat(nullptr), fieldName(fieldName),
          metaType(customMetaType),
          stringStorageMaxLen(std::numeric_limits<int>::max()),
          intStorageRangeMin(std::numeric_limits<int64_t>::min()),
          intStorageRangeMax(std::numeric_limits<int64_t>::max())
{
    init();

    if (cat) {
        cat->addProperty(this);
        assert(this->cat == cat);
    }
}

PartProperty::PartProperty (
        const QString& fieldName,
        Type coreType,
        PartCategory* cat
)       : cat(nullptr), fieldName(fieldName),
          metaType(new PartPropertyMetaType(QString(), coreType)),
          stringStorageMaxLen(std::numeric_limits<int>::max()),
          intStorageRangeMin(std::numeric_limits<int64_t>::min()),
          intStorageRangeMax(std::numeric_limits<int64_t>::max())
{
    init();

    if (cat) {
        cat->addProperty(this);
        assert(this->cat == cat);
    }
}

PartProperty::PartProperty(const PartProperty& other, PartCategory* newCat)
        : cat(newCat), fieldName(other.fieldName), metaType(new PartPropertyMetaType(*other.metaType)),
          sqlAscendingOrderCodeDefault(other.sqlAscendingOrderCodeDefault),
          sqlDescendingOrderCodeDefault(other.sqlDescendingOrderCodeDefault),
          stringStorageMaxLen(other.stringStorageMaxLen),
          intStorageRangeMin(other.intStorageRangeMin), intStorageRangeMax(other.intStorageRangeMax)
{
    assert(newCat->getID() == other.cat->getID());
}

PartProperty::~PartProperty()
{
    delete metaType;
}

void PartProperty::init()
{
    if (!isValidFieldName(fieldName)) {
        throw InvalidValueException(QString("Invalid part property field name: %1").arg(fieldName).toUtf8().constData(),
                                    __FILE__, __LINE__);
    }

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    setDefaultValueFromType();

    sqlAscendingOrderCodeDefault = QString("ORDER BY %1 ASC").arg(dbw->escapeIdentifier(fieldName));
    sqlDescendingOrderCodeDefault = QString("ORDER BY %1 DESC").arg(dbw->escapeIdentifier(fieldName));
}

void PartProperty::setFieldName(const QString& fieldName)
{
    this->fieldName = fieldName;
}

PartProperty::Type PartProperty::getType() const
{
    auto mt = metaType;
    while (mt->parent  &&  mt->coreType == InvalidType) {
        mt = mt->parent;
    }
    return mt->coreType;
}

void PartProperty::setUserReadableName(const QString& name)
{
    metaType->userReadableName = name;
}

QString PartProperty::getUserReadableName() const
{
    auto mt = metaType;
    while (mt  &&  mt->userReadableName.isNull()) {
        mt = mt->parent;
    }
    return mt ? mt->userReadableName : fieldName;
}

flags_t PartProperty::getFlags() const
{
    flags_t flags = 0;
    flags_t flagsWithNonNullValue = 0;
    auto mt = metaType;
    while (mt) {
        flags_t flagsStillOpen = mt->flagsWithNonNullValue & ~flagsWithNonNullValue;
        flags |= mt->flags & flagsStillOpen;
        flagsWithNonNullValue |= mt->flagsWithNonNullValue;
        mt = mt->parent;
    }

    if ((flagsWithNonNullValue & PartProperty::SIPrefixesDefaultToBase2) == 0) {
        QSettings s;
        bool siBinPrefixes = s.value("main/si_binary_prefixes_default", true).toBool();
        if (getUnitPrefixType() == UnitPrefixSIBase2) {
            if (siBinPrefixes) {
                flags |= SIPrefixesDefaultToBase2;
            }
        }
    }

    flags |= FlagsImplicitValue & ~flagsWithNonNullValue;

    return flags;
}

void PartProperty::setFullTextSearchUserPrefix(const QString& pfx)
{
    metaType->ftUserPrefix = pfx;
}

QString PartProperty::getFullTextSearchUserPrefix() const
{
    auto mt = metaType;
    while (mt  &&  mt->ftUserPrefix.isNull()) {
        mt = mt->parent;
    }
    return mt ? mt->ftUserPrefix : fieldName;
}

void PartProperty::setUnitSuffix(const QString& suffix)
{
    metaType->unitSuffix = suffix;
}

QString PartProperty::getUnitSuffix() const
{
    auto mt = metaType;
    while (mt->parent  &&  mt->unitSuffix.isNull()) {
        mt = mt->parent;
    }
    return mt->unitSuffix;
}

void PartProperty::setUnitPrefixType(UnitPrefixType type)
{
    metaType->unitPrefixType = type;
}

PartProperty::UnitPrefixType PartProperty::getUnitPrefixType() const
{
    auto mt = metaType;
    while (mt->parent  &&  mt->unitPrefixType == UnitPrefixInvalid) {
        mt = mt->parent;
    }
    return mt->unitPrefixType == UnitPrefixInvalid ? UnitPrefixSIBase10 : mt->unitPrefixType;
}

void PartProperty::setSQLNaturalOrderCode(const QString& oc)
{
    metaType->sqlNaturalOrderCode = oc;
}

QString PartProperty::getSQLNaturalOrderCode() const
{
    auto mt = metaType;
    while (mt  &&  mt->sqlNaturalOrderCode.isNull()) {
        mt = mt->parent;
    }
    return mt ? mt->sqlNaturalOrderCode : getSQLAscendingOrderCode();
}

void PartProperty::setSQLAscendingOrderCode(const QString& oc)
{
    metaType->sqlAscendingOrderCode = oc;
}

QString PartProperty::getSQLAscendingOrderCode() const
{
    auto mt = metaType;
    while (mt  &&  mt->sqlAscendingOrderCode.isNull()) {
        mt = mt->parent;
    }
    return mt ? mt->sqlAscendingOrderCode : sqlAscendingOrderCodeDefault;
}

void PartProperty::setSQLDescendingOrderCode(const QString& oc)
{
    metaType->sqlDescendingOrderCode = oc;
}

QString PartProperty::getSQLDescendingOrderCode() const
{
    auto mt = metaType;
    while (mt  &&  mt->sqlDescendingOrderCode.isNull()) {
        mt = mt->parent;
    }
    return mt ? mt->sqlDescendingOrderCode : sqlDescendingOrderCodeDefault;
}

void PartProperty::setStringMaximumLength(int len)
{
    metaType->strMaxLen = len;
}

int PartProperty::getStringMaximumLength() const
{
    auto mt = metaType;
    while (mt  &&  mt->strMaxLen < 0) {
        mt = mt->parent;
    }
    return std::min(mt ? mt->strMaxLen : std::numeric_limits<int>::max(), stringStorageMaxLen);
}

void PartProperty::setBooleanTrueDisplayText(const QString& text)
{
    metaType->boolTrueText = text;
}

QString PartProperty::getBooleanTrueDisplayText() const
{
    auto mt = metaType;
    while (mt  &&  mt->boolTrueText.isNull()) {
        mt = mt->parent;
    }
    return mt ? mt->boolTrueText : tr("TRUE");
}

void PartProperty::setBooleanFalseDisplayText(const QString& text)
{
    metaType->boolFalseText = text;
}

QString PartProperty::getBooleanFalseDisplayText() const
{
    auto mt = metaType;
    while (mt  &&  mt->boolFalseText.isNull()) {
        mt = mt->parent;
    }
    return mt ? mt->boolFalseText : tr("FALSE");
}

void PartProperty::setIntegerMinimum(int64_t val)
{
    metaType->intRangeMin = val;
}

int64_t PartProperty::getIntegerMinimum() const
{
    auto mt = metaType;
    while (mt  &&  mt->intRangeMin == std::numeric_limits<int64_t>::max()) {
        mt = mt->parent;
    }
    return std::max(mt ? mt->intRangeMin : std::numeric_limits<int64_t>::min(), intStorageRangeMin);

}

void PartProperty::setIntegerMaximum(int64_t val)
{
    metaType->intRangeMax = val;
}

int64_t PartProperty::getIntegerMaximum() const
{
    auto mt = metaType;
    while (mt  &&  mt->intRangeMax == std::numeric_limits<int64_t>::min()) {
        mt = mt->parent;
    }
    return std::min(mt ? mt->intRangeMax : std::numeric_limits<int64_t>::max(), intStorageRangeMax);
}

void PartProperty::setDecimalMinimum(double val)
{
    metaType->floatRangeMin = val;
}

double PartProperty::getDecimalMinimum() const
{
    auto mt = metaType;
    while (mt  &&  mt->floatRangeMin == std::numeric_limits<double>::max()) {
        mt = mt->parent;
    }
    return mt ? mt->floatRangeMin : std::numeric_limits<double>::lowest();
}

void PartProperty::setDecimalMaximum(double val)
{
    metaType->floatRangeMax = val;
}

double PartProperty::getDecimalMaximum() const
{
    auto mt = metaType;
    while (mt  &&  mt->floatRangeMax == std::numeric_limits<double>::lowest()) {
        mt = mt->parent;
    }
    return mt ? mt->floatRangeMax : std::numeric_limits<double>::max();
}

double PartProperty::getFloatMinimum() const
{
    return getDecimalMinimum();
}

double PartProperty::getFloatMaximum() const
{
    return getDecimalMaximum();
}

void PartProperty::setTextAreaMinimumHeight(int h)
{
    metaType->textAreaMinHeight = h;
}

int PartProperty::getTextAreaMinimumHeight() const
{
    auto mt = metaType;
    while (mt  &&  mt->textAreaMinHeight < 0) {
        mt = mt->parent;
    }
    return mt ? mt->textAreaMinHeight : 50;
}

void PartProperty::setSortIndex(int sortIdx)
{
    metaType->sortIndex = sortIdx;

    if (cat) {
        cat->notifyPropertyOrderChanged();
    }
}

int PartProperty::getSortIndex() const
{
    return metaType->sortIndex;
}

void PartProperty::setDefaultValue(const QVariant& val)
{
    metaType->defaultValue = val;
    setDefaultValueFunction(DefaultValueConst);
}

void PartProperty::setDefaultValueFunction(DefaultValueFunction func)
{
    metaType->defaultValueFunc = func;
}

void PartProperty::setDefaultValueFromType()
{
    switch (getType()) {
    case String:
        setDefaultValue(QString(""));
        break;
    case Integer:
        setDefaultValue(static_cast<qlonglong>(isValidIntegerValue(0) ? 0 : getIntegerMinimum()));
        break;
    case Float:
        setDefaultValue(isValidFloatValue(0.0) ? 0.0 : getFloatMinimum());
        break;
    case Decimal:
        setDefaultValue(isValidFloatValue(0.0) ? 0.0 : getDecimalMinimum());
        break;
    case Boolean:
        setDefaultValue(false);
        break;
    case File:
        setDefaultValue(QString(""));
        break;
    case Date:
        setDefaultValueFunction(DefaultValueNow);
        break;
    case Time:
        setDefaultValueFunction(DefaultValueNow);
        break;
    case DateTime:
        setDefaultValueFunction(DefaultValueNow);
        break;
    default:
        assert(false);
    }
}

QVariant PartProperty::getDefaultValue() const
{
    DefaultValueFunction func = getDefaultValueFunction();
    switch (func) {
    case DefaultValueConst:
        return metaType->defaultValue;
    case DefaultValueNow:
        switch (getType()) {
        case Date:
            return QDate::currentDate();
        case Time:
            return QTime::currentTime();
        case DateTime:
            return QDateTime::currentDateTime();
        default:
            assert(false);
            return QVariant();
        }
        break;
    default:
        assert(false);
        return QVariant();
    }
}

PartProperty::DefaultValueFunction PartProperty::getDefaultValueFunction() const
{
    return metaType->defaultValueFunc;
}

QString PartProperty::getDefaultValueSQLExpression() const
{
    if ((getFlags() & MultiValue) != 0) {
        throw InvalidStateException("Can't generate SQL initial value expression for multi-value property",
                                    __FILE__, __LINE__);
    }

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    switch (getType()) {
    case String:
        return dbw->escapeString(getDefaultValue().toString());
    case Integer:
        return QString::number(getDefaultValue().toLongLong());
    case Float:
        return QString::number(getDefaultValue().toDouble());
    case Decimal:
        return QString::number(getDefaultValue().toDouble());
    case Boolean:
        return getDefaultValue().toBool() ? "TRUE" : "FALSE";
    case File:
        return dbw->escapeString(getDefaultValue().toString());
    case Date:
        if (getDefaultValueFunction() == DefaultValueNow) {
            return "CURRENT_DATE";
        } else {
            return dbw->escapeString(getDefaultValue().toDate().toString(Qt::ISODate));
        }
    case Time:
        if (getDefaultValueFunction() == DefaultValueNow) {
            return "CURRENT_TIME";
        } else {
            return dbw->escapeString(getDefaultValue().toTime().toString(Qt::ISODate));
        }
    case DateTime:
        if (getDefaultValueFunction() == DefaultValueNow) {
            return "CURRENT_TIMESTAMP";
        } else {
            return dbw->escapeString(getDefaultValue().toDateTime().toString(Qt::ISODate));
        }
    default:
        assert(false);
        return QString();
    }
}

QString PartProperty::getMultiValueForeignTableName() const
{
    if (cat)
        return QString("pcatmv_%1_%2").arg(cat->getID(), getFieldName());
    else
        return QString();
}

QList<QString> PartProperty::formatValuesForDisplay(const QList<QVariant>& values) const
{
    QList<QString> displayStrs;

    for (const QVariant& value : values) {
        QString displayStr("");

        if ((getFlags() & MultiValue)  !=  0) {
            for (const QVariant& singleValue : value.toList()) {
                if (!displayStr.isEmpty()) {
                    displayStr += ", ";
                }
                displayStr += formatSingleValueForDisplay(singleValue);
            }
        } else {
            displayStr = formatSingleValueForDisplay(value);
        }

        displayStrs << displayStr;
    }

    return displayStrs;
}

QString PartProperty::formatValueForDisplay(const QVariant& value) const
{
    return formatValuesForDisplay(QList<QVariant>() << value)[0];
}

QString PartProperty::formatSingleValueForDisplay(const QVariant& value) const
{
    if (value.isNull()) {
        return tr("(Invalid)");
    }

    auto flags = getFlags();

    if ((flags & DisplayWithUnits)  !=  0) {
        Type type = getType();

        //SuffixFormat format = SuffixFormatSIBase10;
        SuffixFormat format = SuffixFormatNone;

        switch (getUnitPrefixType()) {
        case UnitPrefixSIBase10:
            format = SuffixFormatSIBase10;
            break;
        case UnitPrefixSIBase2:
            format = SuffixFormatSIBase2;
            break;
        case UnitPrefixPercent:
            format = SuffixFormatPercent;
            break;
        case UnitPrefixPPM:
            format = SuffixFormatPPM;
            break;
        default:
            format = SuffixFormatNone;
        }

        if (type == Integer) {
            return FormatIntSuffices(value.toLongLong(), format) + getUnitSuffix();
        } else if (type == Float  ||  type == Decimal) {
            return FormatFloatSuffices(value.toDouble(), format) + getUnitSuffix();
        } else {
            // Nothing to do. DisplayWithUnits is now set by default, even for non-numeric properties, so we shouldn't
            // throw an error in that case.
        }
    }

    Type type = getType();

    QLocale loc;

    if (type == Boolean) {
        if (value.toBool()) {
            return getBooleanTrueDisplayText();
        } else {
            return getBooleanFalseDisplayText();
        }
    } else if (type == PartProperty::Date) {
        return loc.toString(value.toDate(), QLocale::ShortFormat);
    } else if (type == PartProperty::Time) {
        return loc.toString(value.toTime(), QLocale::ShortFormat);
    } else if (type == PartProperty::DateTime) {
        return loc.toString(value.toDateTime(), QLocale::ShortFormat);
    }

    return value.toString();
}

QString PartProperty::formatValueForEditing(const QVariant& value) const
{
    return formatValueForDisplay(value);
}

QString PartProperty::formatSingleValueForEditing(const QVariant& value) const
{
    return formatSingleValueForDisplay(value);
}

QVariant PartProperty::parseSingleUserInputValue(const QString& userStr) const
{
    auto flags = getFlags();

    Type type = getType();

    if (type == String) {
        return userStr;
    } else if (type == Integer) {
        bool ok;
        int64_t val = ResolveIntSuffices(userStr.trimmed(), &ok, (flags & SIPrefixesDefaultToBase2)  !=  0);

        if (!ok) {
            throw InvalidValueException(tr("Input is not a valid integer value!").toUtf8().constData(), __FILE__, __LINE__);
        }

        int64_t min = getIntegerMinimum();
        int64_t max = getIntegerMaximum();

        if (val < min  ||  val > max) {
            throw InvalidValueException(tr("Input is out of range! Valid range: %1-%2").arg(min).arg(max).toUtf8().constData(),
                    __FILE__, __LINE__);
        }

        return static_cast<qlonglong>(val);
    } else if (type == Float  ||  type == Decimal) {
        bool ok;
        double val = ResolveFloatSuffices(userStr.trimmed(), &ok, (flags & SIPrefixesDefaultToBase2)  !=  0);

        if (!ok) {
            throw InvalidValueException(tr("Input is not a valid floating point value!").toUtf8().constData(), __FILE__, __LINE__);
        }

        double min = getFloatMinimum();
        double max = getFloatMaximum();

        if (val < min  ||  val > max) {
            throw InvalidValueException(tr("Input is out of range! Valid range: %1-%2").arg(min).arg(max).toUtf8().constData(),
                    __FILE__, __LINE__);
        }

        return val;
    } else {
        throw InvalidValueException("Invalid PartProperty type in PartProperty::parseSingleUserInputValue()!",
                __FILE__, __LINE__);
    }
}

QVariant PartProperty::parseUserInputValue(const QString& userStr) const
{
    auto flags = getFlags();

    if ((flags & MultiValue)  !=  0  &&  (flags & DisplayMultiInSingleField)  !=  0) {
        QStringList vals;

        if (userStr.contains('\n')) {
            vals = userStr.split('\n');
        } else {
            if (!userStr.trimmed().isEmpty()) {
                vals = userStr.split(',');
            }
        }

        QList<QVariant> pvals;

        for (QStringList::iterator it = vals.begin() ; it != vals.end() ; it++) {
            QString val = (*it).trimmed();
            pvals << parseSingleUserInputValue(val);
        }

        return pvals;
    } else {
        return parseSingleUserInputValue(userStr);
    }
}

void PartProperty::checkDatabaseConnection() const
{
    if (!System::getInstance()->hasValidDatabaseConnection()) {
        throw NoDatabaseConnectionException("Database connection required for PartProperty", __FILE__);
    }
}

QList<QVariant> PartProperty::getDistinctValues() const
{
    checkDatabaseConnection();

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    auto flags = getFlags();

    QString queryStr;
    if ((flags & MultiValue)  !=  0) {
        queryStr = QString("SELECT DISTINCT %1.%2 FROM %1 %3")
                .arg(dbw->escapeIdentifier(getMultiValueForeignTableName()),
                     dbw->escapeIdentifier(getFieldName()),
                     getSQLNaturalOrderCode());
    } else {
        queryStr = QString("SELECT DISTINCT %1.%2 FROM %1 %3")
                .arg(dbw->escapeIdentifier(cat->getTableName()),
                     dbw->escapeIdentifier(getFieldName()),
                     getSQLNaturalOrderCode());
    }

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, queryStr, "Error fetching property distinct values");

    QList<QVariant> values;
    while (query.next()) {
        values << query.value(0);
    }

    return values;
}

bool PartProperty::isValidIntegerValue(int64_t value) const
{
    return value >= getIntegerMinimum()  &&  value <= getIntegerMaximum();
}

double PartProperty::isValidFloatValue(double value) const
{
    return value >= getFloatMinimum()  &&  value <= getFloatMaximum();
}

int PartProperty::getSortIndexInCategory(PartCategory* pcat) const
{
    if (pcat != this->cat) {
        return 0;
    }
    return getSortIndex();
}

void PartProperty::setSortIndexInCategory(PartCategory* pcat, int sortIdx)
{
    if (pcat != this->cat) {
        return;
    }
    setSortIndex(sortIdx);
}


}


QDataStream& operator<<(QDataStream& s, const electronicsdb::PartProperty* prop)
{
    return s << reinterpret_cast<quint64>(prop);
}

QDataStream& operator>>(QDataStream& s, electronicsdb::PartProperty*& prop)
{
    quint64 ptr;
    s >> ptr;
    prop = reinterpret_cast<electronicsdb::PartProperty*>(ptr);
    return s;
}
