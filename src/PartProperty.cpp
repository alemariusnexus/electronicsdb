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

#include "PartProperty.h"
#include "PartCategory.h"
#include "elutil.h"
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/exception/InvalidValueException.h>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <cstdio>
#include <cfloat>



PartProperty::PartProperty(const QString& fieldName, const QString& userReadableName, Type type, flags_t flags,
		PartCategory* cat)
		: cat(NULL), fieldName(fieldName), userReadableName(userReadableName), ftUserPrefix(fieldName),
		  sqlNaturalOrderCode(QString("ORDER BY %1 ASC").arg(fieldName)),
		  sqlAscendingOrderCode(sqlNaturalOrderCode),
		  sqlDescendingOrderCode(QString("ORDER BY %1 DESC").arg(fieldName)),
		  //decimalPrecision(32), decimalScale(16),
		  strMaxLen(0),
		  boolTrueText(tr("TRUE")), boolFalseText(tr("FALSE")),
		  intRangeMin(INT64_MIN), intRangeMax(INT64_MAX),
		  floatRangeMin(-DBL_MAX), floatRangeMax(DBL_MAX),
		  textAreaMinHeight(50),
		  type(type), flags(flags)
{
	if (type == Integer) {
		setInitialValue("0");
	} else if (type == Float  ||  type == Decimal) {
		setInitialValue("0.0");
	} else if (type == String) {
		setInitialValue("");
	} else if (type == PartLink) {
		setInitialValue("0");
	} else if (type == Boolean) {
		setInitialValue("0");
	} else if (type == File) {
		setInitialValue(QString());
	} else {
		throw InvalidValueException("Invalid PartProperty type in PartProperty constructor!", __FILE__, __LINE__);
	}

	if (cat)
		cat->addProperty(this);
}


void PartProperty::setType(Type type)
{
	this->type = type;
}


void PartProperty::setFieldName(const QString& fieldName)
{
	this->fieldName = fieldName;
}


void PartProperty::setUserReadableName(const QString& name)
{
	userReadableName = name;
}


void PartProperty::setFlags(flags_t flags)
{
	this->flags = flags;
}


void PartProperty::setFullTextSearchUserPrefix(const QString& pfx)
{
	ftUserPrefix = pfx;
}


void PartProperty::setUnitSuffix(const QString& suffix)
{
	unitSuffix = suffix;
}


void PartProperty::setSQLNaturalOrderCode(const QString& oc)
{
	sqlNaturalOrderCode = oc;
}


void PartProperty::setSQLAscendingOrderCode(const QString& oc)
{
	sqlAscendingOrderCode = oc;
}


void PartProperty::setSQLDescendingOrderCode(const QString& oc)
{
	sqlDescendingOrderCode = oc;
}


void PartProperty::setSQLDefaultOrderCode(const QString& baseOc)
{
	sqlAscendingOrderCode = QString("%1 ASC").arg(baseOc);
	sqlDescendingOrderCode = QString("%1 DESC").arg(baseOc);

	sqlNaturalOrderCode = sqlAscendingOrderCode;
}


void PartProperty::setInitialValue(const QString& val)
{
	initialValue = val;
}


void PartProperty::setPartLinkCategory(PartCategory* cat)
{
	linkCat = cat;
}


QString PartProperty::getMultiValueForeignTableName() const
{
	if (cat)
		return QString("pcatmv_%1_%2").arg(cat->getID()).arg(getFieldName());
	else
		return QString();
}


/*void PartProperty::setDecimalConstraint(unsigned int precision, unsigned int scale)
{
	decimalPrecision = precision;
	decimalScale = scale;
}*/


void PartProperty::setStringMaximumLength(unsigned int len)
{
	strMaxLen = len;
}


void PartProperty::setBooleanDisplayText(const QString& trueText, const QString& falseText)
{
	boolTrueText = trueText;
	boolFalseText = falseText;
}


void PartProperty::setIntegerValidRange(int64_t min, int64_t max)
{
	intRangeMin = min;
	intRangeMax = max;
}


int64_t PartProperty::getIntegerMinimum() const
{
	return intRangeMin;
}


int64_t PartProperty::getIntegerMaximum() const
{
	return intRangeMax;
}


void PartProperty::setFloatValidRange(double min, double max)
{
	floatRangeMin = min;
	floatRangeMax = max;
}


double PartProperty::getFloatMinimum() const
{
	return floatRangeMin;
}


double PartProperty::getFloatMaximum() const
{
	return floatRangeMax;
}


void PartProperty::setTextAreaMinimumHeight(unsigned int h)
{
	textAreaMinHeight = h;
}


QList<QString> PartProperty::formatValuesForDisplay(QList<QList<QString> > rawData) const
{
	QList<QString> displayStrs;

	if (type == PartLink) {
		QSet<unsigned int> idSet;

		for (QList<QList<QString> >::iterator it = rawData.begin() ; it != rawData.end() ; it++) {
			QList<QString> data = *it;

			for (QList<QString>::iterator iit = data.begin() ; iit != data.end() ; iit++) {
				QString rawVal = *iit;

				unsigned int id = rawVal.toUInt();

				idSet.insert(id);
			}
		}

		QList<unsigned int> ids = idSet.toList();

		QMap<unsigned int, QString> descs = linkCat->getDescriptions(ids);

		for (QList<QList<QString> >::iterator it = rawData.begin() ; it != rawData.end() ; it++) {
			QList<QString> rawRecData = *it;

			QString displayStr("");

			for (QList<QString>::iterator iit = rawRecData.begin() ; iit != rawRecData.end() ; iit++) {
				if (!displayStr.isEmpty())
					displayStr += ", ";

				unsigned int id = iit->toUInt();

				displayStr += descs[id];
			}

			displayStrs << displayStr;
		}
	} else {
		for (QList<QList<QString> >::iterator it = rawData.begin() ; it != rawData.end() ; it++) {
			QList<QString> data = *it;

			QString displayStr("");


			if ((flags & MultiValue)  !=  0) {
				for (QList<QString>::iterator it = data.begin() ; it != data.end() ; it++) {
					if (it != data.begin())
						displayStr += ", ";

					displayStr += formatSingleValueForDisplay(*it);
				}
			} else {
				if (data.isEmpty()) {
					displayStr = tr("(Invalid)");
				} else {
					displayStr = formatSingleValueForDisplay(data[0]);
				}
			}

			displayStrs << displayStr;
		}
	}

	return displayStrs;
}


QString PartProperty::formatValueForDisplay(QList<QString> rawData) const
{
	return formatValuesForDisplay(QList<QList<QString> >() << rawData)[0];
}


QString PartProperty::formatValueForEditing(QList<QString> rawData) const
{
	return formatValueForDisplay(rawData);
}


QString PartProperty::formatSingleValueForDisplay(const QString& rawStr) const
{
	if (rawStr.isNull()) {
		return tr("(Invalid)");
	}

	if ((flags & DisplayWithUnits)  !=  0) {
		Type type = getType();

		//bool base2 = (flags & DisplayBinaryPrefixes) != 0;
		SuffixFormat format = SuffixFormatSIBase10;

		if ((flags & DisplayUnitPrefixSIBase2)  !=  0) {
			format = SuffixFormatSIBase2;
		} else if ((flags & DisplayUnitPrefixPercent)  !=  0) {
			format = SuffixFormatPercent;
		} else if ((flags & DisplayUnitPrefixPPM)  !=  0) {
			format = SuffixFormatPPM;
		} else if ((flags & DisplayUnitPrefixNone)  !=  0) {
			format = SuffixFormatNone;
		}

		if (type == Integer) {
			int64_t val = rawStr.toLongLong();
			return FormatIntSuffices(val, format) + unitSuffix;
		} else if (type == Float  ||  type == Decimal) {
			double val = rawStr.toDouble();
			return FormatFloatSuffices(val, format) + unitSuffix;
		} else {
			printf("FICK DICH: %s\n", getFieldName().toLocal8Bit().constData());
			throw InvalidStateException("DisplayWithUnits flag may only be set for Integer or Float PartProperties!",
					__FILE__, __LINE__);
		}
	}

	if (type == Boolean) {
		qlonglong val = rawStr.toLongLong();

		if (val == 0) {
			return boolFalseText;
		} else {
			return boolTrueText;
		}
	} else {
		return rawStr;
	}
}


QString PartProperty::parseSingleUserInputValue(const QString& userStr) const
{
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

		/*if (!intIsSigned  &&  val < 0) {
			throw InvalidValueException(tr("Input must be unsigned!").toUtf8().constData(), __FILE__, __LINE__);
		}*/

		return QString("%1").arg(val);
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

		return QString("%1").arg(val);
	} else if (type == PartLink) {
		bool ok;
		userStr.toUInt(&ok);

		if (!ok) {
			throw InvalidValueException(tr("Input is not a valid part ID!").toUtf8().constData(), __FILE__, __LINE__);
		}

		return userStr;
	} else {
		throw InvalidValueException("Invalid PartProperty type in PartProperty::parseSingleUserInputValue()!",
				__FILE__, __LINE__);
	}
}


QList<QString> PartProperty::parseUserInputValue(const QString& userStr) const
{
	if ((flags & MultiValue)  !=  0  &&  (flags & DisplayMultiInSingleField)  !=  0) {
		QStringList vals;

		if (userStr.contains('\n')) {
			vals = userStr.split('\n');
		} else {
			if (!userStr.trimmed().isEmpty()) {
				vals = userStr.split(',');
			}
		}

		QList<QString> pvals;

		for (QStringList::iterator it = vals.begin() ; it != vals.end() ; it++) {
			QString val = (*it).trimmed();
			pvals << parseSingleUserInputValue(val);
		}

		return pvals;
	} else {
		return QList<QString>() << parseSingleUserInputValue(userStr);
	}
}


