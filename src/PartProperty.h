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

#ifndef PARTPROPERTY_H_
#define PARTPROPERTY_H_

#include "global.h"
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>


class PartCategory;


class PartProperty : public QObject
{
	Q_OBJECT

public:
	enum Type
	{
		String,
		Integer,
		Float,
		Decimal,
		PartLink,
		Boolean,
		File
	};

	enum Flags
	{
		Default = 0,
		FullTextIndex = 1 << 0,
		MultiValue = 1 << 1,
		NullAllowed = 1 << 2,
		DescriptionProperty = 1 << 3,
		SIPrefixesDefaultToBase2 = 1 << 4,

		DisplayWithUnits = 1 << 16,
		DisplayNoSelectionList = 1 << 17,
		DisplayHideFromListingTable = 1 << 18,
		DisplayTextArea = 1 << 19,
		DisplayMultiInSingleField = 1 << 20,
		DisplayDynamicEnum = 1 << 21,

		DisplayUnitPrefixSIBase2 = 1 << 22,
		DisplayUnitPrefixPercent = 1 << 23,
		DisplayUnitPrefixPPM = 1 << 24,
		DisplayUnitPrefixNone = 1 << 25
	};

	enum IntegerStorageType
	{
		Int8,
		Int16,
		Int24,
		Int32,
		Int64
	};

public:
	PartProperty(const QString& fieldName, const QString& userReadableName, Type type, flags_t flags = Default,
			PartCategory* cat = NULL);

	void setType(Type type);
	void setFieldName(const QString& fieldName);
	void setUserReadableName(const QString& name);
	void setFlags(flags_t flags);
	void setFullTextSearchUserPrefix(const QString& pfx);
	void setUnitSuffix(const QString& suffix);
	void setSQLNaturalOrderCode(const QString& oc);
	void setSQLAscendingOrderCode(const QString& oc);
	void setSQLDescendingOrderCode(const QString& oc);
	void setSQLDefaultOrderCode(const QString& baseOc);
	void setPartLinkCategory(PartCategory* cat);
	void setInitialValue(const QString& val);
	//void setDecimalConstraint(unsigned int precision, unsigned int scale);
	void setStringMaximumLength(unsigned int len);
	void setBooleanDisplayText(const QString& trueText, const QString& falseText);
	void setIntegerValidRange(int64_t min, int64_t max);
	void setFloatValidRange(double min, double max);
	void setDecimalValidRange(double min, double max) { setFloatValidRange(min, max); }
	void setTextAreaMinimumHeight(unsigned int h);

	PartCategory* getPartCategory() { return cat; }
	QString getFieldName() const { return fieldName; }
	QString getUserReadableName() const { return userReadableName; }
	QString getFullTextSearchUserPrefix() const { return ftUserPrefix; }
	QString getSQLNaturalOrderCode() const { return sqlNaturalOrderCode; }
	QString getSQLAscendingOrderCode() const { return sqlAscendingOrderCode; }
	QString getSQLDescendingOrderCode() const { return sqlDescendingOrderCode; }
	QString getInitialValue() const { return initialValue; }
	QString getMultiValueForeignTableName() const;
	QString getMultiValueIDFieldName() const { return "id"; }
	//unsigned int getDecimalPrecision() const { return decimalPrecision; }
	//unsigned int getDecimalScale() const { return decimalScale; }
	unsigned int getStringMaximumLength() const { return strMaxLen; }
	QString getBooleanTrueDisplayText() const { return boolTrueText; }
	QString getBooleanFalseDisplayText() const { return boolFalseText; }
	int64_t getIntegerMinimum() const;
	int64_t getIntegerMaximum() const;
	double getFloatMinimum() const;
	double getFloatMaximum() const;
	double getDecimalMinimum() const { return getFloatMinimum(); }
	double getDecimalMaximum() const { return getFloatMaximum(); }
	unsigned int getTextAreaMinimumHeight() const { return textAreaMinHeight; }
	PartCategory* getPartLinkCategory() { return linkCat; }

	Type getType() const { return type; }
	flags_t getFlags() const { return flags; }

	QList<QString> formatValuesForDisplay(QList<QList<QString> > rawData) const;
	QString formatValueForDisplay(QList<QString> rawData) const;

	QString formatValueForEditing(QList<QString> rawData) const;

	QList<QString> parseUserInputValue(const QString& userStr) const;

private:
	QString formatSingleValueForDisplay(const QString& rawStr) const;
	QString parseSingleUserInputValue(const QString& userStr) const;

private:
	PartCategory* cat;
	QString fieldName;
	QString userReadableName;
	QString ftUserPrefix;
	QString unitSuffix;
	QString sqlNaturalOrderCode;
	QString sqlAscendingOrderCode;
	QString sqlDescendingOrderCode;
	QString initialValue;
	//unsigned int decimalPrecision;
	//unsigned int decimalScale;
	unsigned int strMaxLen;
	QString boolTrueText;
	QString boolFalseText;
	int64_t intRangeMin;
	int64_t intRangeMax;
	double floatRangeMin;
	double floatRangeMax;
	unsigned int textAreaMinHeight;
	Type type;
	flags_t flags;

	PartCategory* linkCat;


	friend class PartCategory;
};

#endif /* PARTPROPERTY_H_ */
