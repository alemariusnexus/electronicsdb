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

#include "PartCategoryProvider.h"
#include "System.h"
#include "sqlutils.h"
#include <nxcommon/exception/InvalidValueException.h>
#include <cfloat>


PartCategoryProvider* PartCategoryProvider::instance = NULL;






PartCategoryProvider* PartCategoryProvider::getInstance()
{
	if (!instance) {
		instance = new PartCategoryProvider;
	}

	return instance;
}


void PartCategoryProvider::destroy()
{
	if (instance) {
		PartCategoryProvider* inst = instance;
		instance = NULL;
		delete inst;
	}
}




/*void PartCategoryProvider::addNotesProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* notesProp = new PartProperty("notes", tr("Notes"), PartProperty::String,
			PartProperty::DisplayHideFromListingTable | PartProperty::DisplayNoSelectionList | PartProperty::DisplayTextArea
			| PartProperty::FullTextIndex | addFlags,
			cat);
	notesProp->setStringMaximumLength(4096);
}


void PartCategoryProvider::addDatasheetsProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* dsheetProp = new PartProperty("datasheet",  tr("Datasheets"), PartProperty::PartLink,
			PartProperty::MultiValue | PartProperty::DisplayHideFromListingTable | PartProperty::DisplayNoSelectionList
			| addFlags,
			cat);
	dsheetProp->setPartLinkCategory(dsheetCat);
}


void PartCategoryProvider::addVendorProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* vendorProp = new PartProperty("vendor",  tr("Vendor"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | addFlags,
			cat);
	vendorProp->setStringMaximumLength(128);
}


void PartCategoryProvider::addNumStockProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* numStockProp = new PartProperty("numstock",  tr("Number In Stock"), PartProperty::Integer,
			PartProperty::DisplayNoSelectionList | addFlags,
			cat);
	numStockProp->setIntegerStorageType(PartProperty::Int16);
	numStockProp->setIntegerSignedness(false);
}


void PartCategoryProvider::addTempMaxProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* tempMaxProp = new PartProperty("tempmax", tr("Maximum Temperature"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits | PartProperty::DisplayUnitPrefixNone | addFlags,
			cat);
	tempMaxProp->setUnitSuffix(QString("%1C").arg(QChar(0xB0)));
	tempMaxProp->setDecimalConstraint(9, 3);
}


void PartCategoryProvider::addPackageProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* pkgProp = new PartProperty("package", tr("Package"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | addFlags,
			cat);
	pkgProp->setStringMaximumLength(32);
}


void PartCategoryProvider::addToleranceProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* tolProp = new PartProperty("tolerance", tr("Tolerance"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits | addFlags,
			cat);
	tolProp->setDecimalConstraint(20, 10);
}


void PartCategoryProvider::addKeywordsProperty(PartCategory* cat, flags_t addFlags)
{
	PartProperty* kwProp = new PartProperty("keyword", tr("Keywords"), PartProperty::String,
			PartProperty::MultiValue | PartProperty::DisplayMultiInSingleField | PartProperty::DisplayHideFromListingTable
			| PartProperty::DisplayTextArea | PartProperty::FullTextIndex,
			cat);
	kwProp->setStringMaximumLength(64);
}*/





void PartCategoryProvider::applyMetaData(PartProperty* prop, const SQLResult& res, flags_t& explicitFlags)
{
	CString typeStr;

	if (!res.isNull("type")) {
		typeStr = res.getStringUTF8("type").lower();
	} else {
		typeStr = res.getStringUTF8("id").lower();
	}

	bool isCoreType = true;

	if (typeStr == "string") {
		prop->setType(PartProperty::String);
	} else if (typeStr == "int"  ||  typeStr == "integer") {
		prop->setType(PartProperty::Integer);
	} else if (typeStr == "decimal"  ||  typeStr == "float"  ||  typeStr == "double") {
		prop->setType(PartProperty::Decimal);
	} else if (typeStr == "boolean"  ||  typeStr == "bool") {
		prop->setType(PartProperty::Boolean);
	} else if (typeStr == "partlink"  ||  typeStr == "plink") {
		prop->setType(PartProperty::PartLink);
	} else if (typeStr == "file") {
		prop->setType(PartProperty::File);
	} else {
		isCoreType = false;

		if (!applyMetaType(prop, typeStr, explicitFlags)) {
			throw InvalidValueException(QString("Property '%1.%2' has invalid type: %3")
					.arg(prop->getPartCategory()->getID()).arg(prop->getFieldName()).arg(typeStr));
		}
	}

	if (!res.isNull("name")) {
		prop->setUserReadableName(res.getString("name"));
	}

	flags_t flags = prop->getFlags();

	struct FlagEntry
	{
		CString fieldName;
		flags_t flagCode;
	};

	const FlagEntry flagEntries[] = {
			{"flag_full_text_indexed", PartProperty::FullTextIndex},
			{"flag_multi_value", PartProperty::MultiValue},
			{"flag_si_prefixes_default_to_base2", PartProperty::SIPrefixesDefaultToBase2},
			{"flag_display_with_units", PartProperty::DisplayWithUnits},
			{"flag_display_no_selection_list", PartProperty::DisplayNoSelectionList},
			{"flag_display_hide_from_listing_table", PartProperty::DisplayHideFromListingTable},
			{"flag_display_text_area", PartProperty::DisplayTextArea},
			{"flag_display_multi_in_single_field", PartProperty::DisplayMultiInSingleField},
			{"flag_display_dynamic_enum", PartProperty::DisplayDynamicEnum}
	};

	for (size_t i = 0 ; i < sizeof(flagEntries) / sizeof(FlagEntry) ; i++) {
		const FlagEntry& entry = flagEntries[i];

		if (!res.isNull(entry.fieldName)) {
			explicitFlags |= entry.flagCode;

			if (res.getBool(entry.fieldName)) {
				flags |= entry.flagCode;
			} else {
				flags &= ~entry.flagCode;
			}
		}
	}

	if (!res.isNull("display_unit_affix")) {
		CString affix = res.getStringUTF8("display_unit_affix").lower();

		if (affix == "sibase2") {
			flags |= PartProperty::DisplayUnitPrefixSIBase2;
			explicitFlags |= PartProperty::DisplayUnitPrefixSIBase2;
		} else if (affix == "sibase10") {
			// Absence of a DisplayUnitPrefix* flag signals SI base 10
		} else if (affix == "percent") {
			flags |= PartProperty::DisplayUnitPrefixPercent;
			explicitFlags |= PartProperty::DisplayUnitPrefixPercent;
		} else if (affix == "ppm") {
			flags |= PartProperty::DisplayUnitPrefixPPM;
			explicitFlags |= PartProperty::DisplayUnitPrefixPPM;
		} else if (affix == "none") {
			flags |= PartProperty::DisplayUnitPrefixNone;
			explicitFlags |= PartProperty::DisplayUnitPrefixNone;
		} else {
			// TODO: Error
		}
	}

	prop->setFlags(flags);

	if (!res.isNull("string_max_length")) {
		unsigned int len = res.getUInt32("string_max_length");

		prop->setStringMaximumLength(len);
	}

	if (!res.isNull("ft_user_prefix")) {
		prop->setFullTextSearchUserPrefix(res.getString("ft_user_prefix"));
	}

	if (!res.isNull("unit_suffix")) {
		prop->setUnitSuffix(res.getString("unit_suffix"));
	}

	if (!res.isNull("sql_natural_order_code")) {
		prop->setSQLNaturalOrderCode(res.getString("sql_natural_order_code"));
	}
	if (!res.isNull("sql_ascending_order_code")) {
		prop->setSQLAscendingOrderCode(res.getString("sql_ascending_order_code"));
	}
	if (!res.isNull("sql_descending_order_code")) {
		prop->setSQLDescendingOrderCode(res.getString("sql_descending_order_code"));
	}

	if (prop->getType() == PartProperty::PartLink) {
		CString plCatName;

		if (!res.isNull("pl_category")) {
			plCatName = res.getStringUTF8("pl_category").lower();
		} else if (isCoreType  &&  !prop->getPartLinkCategory()) {
			plCatName = prop->getFieldName().toLower();
		}

		if (!plCatName.isNull()  ||  isCoreType) {
			PartCategory* finalPlCat = NULL;

			for (PartCategory* plCat : cats) {
				if (plCat->getID().toLower() == plCatName) {
					finalPlCat = plCat;
					break;
				}
			}

			if (!finalPlCat) {
				throw InvalidValueException(QString("pl_category field of property '%1.%2' references an invalid "
						"part category: '%3'").arg(prop->getPartCategory()->getID()).arg(prop->getFieldName()).arg(plCatName));
			}

			prop->setPartLinkCategory(finalPlCat);
		}
	}

	QString boolTrueText = prop->getBooleanTrueDisplayText();
	QString boolFalseText = prop->getBooleanFalseDisplayText();

	if (isCoreType) {
		boolTrueText = "true";
		boolFalseText = "false";
	}

	if (!res.isNull("bool_true_text")) {
		boolTrueText = res.getString("bool_true_text");
	}
	if (!res.isNull("bool_false_text")) {
		boolFalseText = res.getString("bool_false_text");
	}

	prop->setBooleanDisplayText(boolTrueText, boolFalseText);

	int64_t intRangeMin = prop->getIntegerMinimum();
	int64_t intRangeMax = prop->getIntegerMaximum();
	double decimalRangeMin = prop->getDecimalMinimum();
	double decimalRangeMax = prop->getDecimalMaximum();

	if (!res.isNull("int_range_min")) {
		intRangeMin = res.getInt64("int_range_min");
	}
	if (!res.isNull("int_range_max")) {
		intRangeMax = res.getInt64("int_range_max");
	}

	if (!res.isNull("decimal_range_min")) {
		decimalRangeMin = res.getDouble("decimal_range_min");
	}
	if (!res.isNull("decimal_range_max")) {
		decimalRangeMax = res.getDouble("decimal_range_max");
	}

	prop->setIntegerValidRange(intRangeMin, intRangeMax);
	prop->setDecimalValidRange(decimalRangeMin, decimalRangeMax);

	if (!res.isNull("textarea_min_height")) {
		prop->setTextAreaMinimumHeight(res.getUInt32("textarea_min_height"));
	}
}


bool PartCategoryProvider::applyMetaType(PartProperty* prop, const QString& metaType, flags_t& explicitFlags)
{
	System* sys = System::getInstance();

	DatabaseConnection* conn = sys->getCurrentDatabaseConnection();
	SQLDatabase db = sys->getCurrentSQLDatabase();

	SQLPreparedStatement stmt = db.createPreparedStatement(u"SELECT * FROM meta_type WHERE id=?");
	stmt.bindString(0, metaType.toLower());

	SQLResult res = stmt.execute();

	if (!res.nextRecord()) {
		return false;
	}

	applyMetaData(prop, res, explicitFlags);

	return true;
}


QList<PartCategory*> PartCategoryProvider::buildCategories()
{
	System* sys = System::getInstance();

	cats.clear();

	if (!sys->hasValidDatabaseConnection()) {
		return cats;
	}

	DatabaseConnection* conn = sys->getCurrentDatabaseConnection();
	SQLDatabase db = sys->getCurrentSQLDatabase();

	SQLResult res = db.sendQuery(u"SELECT id, name, desc_pattern FROM part_category ORDER BY sortidx, name ASC");

	while (res.nextRecord()) {
		QString catName = res.getString("id");

		PartCategory* cat = new PartCategory(catName, res.getString("name"));
		cat->setDescriptionPattern(res.getString("desc_pattern"));

		cats << cat;
	}

	for (PartCategory* cat : cats) {
		QString catID = cat->getID();

		SQLResult res;

		QMap<QString, QString> propDefaultValues;
		QMap<QString, unsigned int> propMaxLengths;
		QMap<QString, int64_t> propIntStorageRangeMins;
		QMap<QString, int64_t> propIntStorageRangeMaxs;

		if (conn->getType() == DatabaseConnection::SQLite) {
			res = db.sendQuery(UString(u"PRAGMA table_info(").append(cat->getTableName()).append(u")"));

			while (res.nextRecord()) {
				QString propName(res.getString("name"));
				QString propDef;

				if (!res.isNull("dflt_value")) {
					propDef = res.getString("dflt_value");
				}

				propDefaultValues.insert(propName, propDef);

				// SQLite has no inherent maximum length for a string (apart from storage limits of course)

				if (res.getStringUTF8("type") == CString("INTEGER")) {
					// SQLite supports storing of INTEGERs with at most 8 bytes, but they are always signed.
					propIntStorageRangeMins.insert(propName, INT64_MIN);
					propIntStorageRangeMaxs.insert(propName, INT64_MAX);
				}
			}
		} else {
			res = db.sendQuery(UString(
					u"SELECT COLUMN_NAME, COLUMN_DEFAULT, CHARACTER_MAXIMUM_LENGTH, DATA_TYPE, COLUMN_TYPE "
					u"FROM INFORMATION_SCHEMA.COLUMNS "
					u"WHERE TABLE_NAME='").append(UString(cat->getTableName())).append(u"'"));

			while (res.nextRecord()) {
				QString propName(res.getString("COLUMN_NAME"));
				QString propDef;

				if (!res.isNull("COLUMN_DEFAULT")) {
					propDef = res.getString("COLUMN_DEFAULT");
				}

				propDefaultValues.insert(propName, propDef);

				if (!res.isNull("CHARACTER_MAXIMUM_LENGTH")) {
					propMaxLengths.insert(propName, res.getUInt32("CHARACTER_MAXIMUM_LENGTH"));
				}

				QString colType(res.getString("DATA_TYPE"));
				colType = colType.toLower();

				bool isUnsigned = QString(res.getString("COLUMN_TYPE")).toLower().contains("unsigned");

				if (colType == "tinyint") {
					if (isUnsigned) {
						propIntStorageRangeMins.insert(propName, 0);
						propIntStorageRangeMaxs.insert(propName, UINT8_MAX);
					} else {
						propIntStorageRangeMins.insert(propName, INT8_MIN);
						propIntStorageRangeMaxs.insert(propName, INT8_MAX);
					}
				} else if (colType == "smallint") {
					if (isUnsigned) {
						propIntStorageRangeMins.insert(propName, 0);
						propIntStorageRangeMaxs.insert(propName, UINT16_MAX);
					} else {
						propIntStorageRangeMins.insert(propName, INT16_MIN);
						propIntStorageRangeMaxs.insert(propName, INT16_MAX);
					}
				} else if (colType == "mediumint") {
					if (isUnsigned) {
						propIntStorageRangeMins.insert(propName, 0);
						propIntStorageRangeMaxs.insert(propName, 16777215);
					} else {
						propIntStorageRangeMins.insert(propName, -8388608);
						propIntStorageRangeMaxs.insert(propName, 8388607);
					}
				} else if (colType == "int") {
					if (isUnsigned) {
						propIntStorageRangeMins.insert(propName, 0);
						propIntStorageRangeMaxs.insert(propName, UINT32_MAX);
					} else {
						propIntStorageRangeMins.insert(propName, INT32_MIN);
						propIntStorageRangeMaxs.insert(propName, INT32_MAX);
					}
				} else if (colType == "bigint") {
					if (isUnsigned) {
						propIntStorageRangeMins.insert(propName, 0);
						propIntStorageRangeMaxs.insert(propName, UINT64_MAX);
					} else {
						propIntStorageRangeMins.insert(propName, INT64_MIN);
						propIntStorageRangeMaxs.insert(propName, INT64_MAX);
					}
				}
			}
		}

		res = db.sendQuery(QString("SELECT * FROM pcatmeta_%1 ORDER BY sortidx, id ASC").arg(catID));

		while (res.nextRecord()) {
			PartProperty::Type type;

			QString propID = res.getString("id");

			int64_t storageIntMin = INT64_MIN;
			int64_t storageIntMax = INT64_MAX;

			if (propIntStorageRangeMins.contains(propID)) {
				storageIntMin = propIntStorageRangeMins[propID];
				storageIntMax = propIntStorageRangeMaxs[propID];
			}

			PartProperty* prop = new PartProperty(propID, "", PartProperty::String, 0, cat);

			flags_t explicitFlags = 0;

			applyMetaData(prop, res, explicitFlags);

			// FullTextIndex is set unless explicitly stated otherwise
			if ((explicitFlags & PartProperty::FullTextIndex) == 0) {
				prop->setFlags(prop->getFlags() | PartProperty::FullTextIndex);
			}

			type = prop->getType();

			if ((explicitFlags & PartProperty::DisplayWithUnits) == 0  &&
					(type == PartProperty::Integer || type == PartProperty::Float || type == PartProperty::Decimal)) {
				prop->setFlags(prop->getFlags() | PartProperty::DisplayWithUnits);
			}

			if (storageIntMin > prop->getIntegerMinimum()) {
				prop->setIntegerValidRange(storageIntMin, prop->getIntegerMaximum());
			}
			if (storageIntMax < prop->getIntegerMaximum()) {
				prop->setIntegerValidRange(prop->getIntegerMinimum(), storageIntMax);
			}

			QString defVal = propDefaultValues[prop->getFieldName()];

			if (!defVal.isNull()) {
				prop->setInitialValue(defVal);
			}

			if (propMaxLengths.contains(prop->getFieldName())) {
				unsigned int maxLen = propMaxLengths[prop->getFieldName()];
				prop->setStringMaximumLength(maxLen);
			}
		}

		cat->rebuildFullTextIndex();
	}

	return cats;

	/*QList<PartCategory*> cats;




	dsheetCat = new PartCategory("datasheet", tr("Datasheets"));
	dsheetCat->setDescriptionPattern("Datasheet: $title");
	cats << dsheetCat;

	PartProperty* dsheetTitleProp = new PartProperty("title", tr("Title"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			dsheetCat);
	dsheetTitleProp->setStringMaximumLength(256);

	PartProperty* dsheetFileProp = new PartProperty("file", tr("File"), PartProperty::File,
			PartProperty::FullTextIndex,
			dsheetCat);




	PartCategory* capCat = new PartCategory("capacitor", tr("Capacitors"));
	capCat->setDescriptionPattern("$capacitance $tempcoeff $type $package $technology Capacitor");
	cats << capCat;

	PartProperty* capTechProp = new PartProperty("technology", tr("Technology"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			capCat);
	capTechProp->setStringMaximumLength(32);

	PartProperty* capCapProp = new PartProperty("capacitance", tr("Capacitance"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits | PartProperty::DescriptionProperty,
			capCat);
	capCapProp->setUnitSuffix("F");
	capCapProp->setDecimalConstraint(36, 24);

	addPackageProperty(capCat, PartProperty::DescriptionProperty);
	addToleranceProperty(capCat, PartProperty::DisplayUnitPrefixPercent);

	PartProperty* capTypeProp = new PartProperty("type", tr("Type"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			capCat);
	capTypeProp->setStringMaximumLength(64);

	addTempMaxProperty(capCat);

	PartProperty* capTempCoeffProp = new PartProperty("tempcoeff", tr("Temperature Coefficient"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::DescriptionProperty,
			capCat);
	capTempCoeffProp->setStringMaximumLength(8);

	PartProperty* capVoltageProp = new PartProperty("voltage", tr("Voltage"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits,
			capCat);
	capVoltageProp->setUnitSuffix("V");
	capVoltageProp->setDecimalConstraint(27, 12);


	addVendorProperty(capCat);
	addDatasheetsProperty(capCat);
	addNotesProperty(capCat);
	addNumStockProperty(capCat);




	PartCategory* icCat = new PartCategory("ic", tr("Integrated Circuits"));
	icCat->setDescriptionPattern("IC: $name");
	cats << icCat;

	PartProperty* icNameProp = new PartProperty("name", tr("Name"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			icCat);
	icNameProp->setStringMaximumLength(128);

	PartProperty* icDescProp = new PartProperty("description", tr("Description"), PartProperty::String,
			PartProperty::FullTextIndex,
			icCat);
	icDescProp->setStringMaximumLength(256);

	addPackageProperty(icCat);

	addKeywordsProperty(icCat);
	addVendorProperty(icCat);
	addDatasheetsProperty(icCat);
	addNotesProperty(icCat);
	addNumStockProperty(icCat);




	PartCategory* ucCat = new PartCategory("microcontroller", tr("Microcontrollers"));
	ucCat->setDescriptionPattern("$model $arch $processor Microcontroller");
	cats << ucCat;

	PartProperty* ucModelProp = new PartProperty("model", tr("Model"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			ucCat);
	ucModelProp->setStringMaximumLength(128);

	PartProperty* ucArchProp = new PartProperty("arch", tr("Architecture"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			ucCat);
	ucArchProp->setStringMaximumLength(32);

	PartProperty* ucProcProp = new PartProperty("processor", tr("Processor"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			ucCat);
	ucProcProp->setStringMaximumLength(64);

	addPackageProperty(ucCat);

	PartProperty* ucFreqmaxProp = new PartProperty("freqmax", tr("Maximum Frequency"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits,
			ucCat);
	ucFreqmaxProp->setUnitSuffix("Hz");
	ucFreqmaxProp->setDecimalConstraint(24, 9);

	PartProperty* ucIOCountProp = new PartProperty("iocount", tr("IO Count"), PartProperty::Integer,
			0,
			ucCat);
	ucIOCountProp->setIntegerStorageType(PartProperty::Int16);
	ucIOCountProp->setIntegerSignedness(false);

	PartProperty* ucProgmemProp = new PartProperty("progmemsize", tr("Program Memory"), PartProperty::Integer,
			PartProperty::DisplayWithUnits | PartProperty::DisplayUnitPrefixSIBase2,
			ucCat);
	ucProgmemProp->setUnitSuffix("B");
	ucProgmemProp->setIntegerSignedness(false);

	PartProperty* ucRAMProp = new PartProperty("ramsize", tr("RAM"), PartProperty::Integer,
			PartProperty::DisplayWithUnits | PartProperty::DisplayUnitPrefixSIBase2,
			ucCat);
	ucRAMProp->setUnitSuffix("B");
	ucRAMProp->setIntegerSignedness(false);

	PartProperty* ucEEPROMProp = new PartProperty("eepromsize", tr("EEPROM Size"), PartProperty::Integer,
			PartProperty::DisplayWithUnits | PartProperty::DisplayUnitPrefixSIBase2,
			ucCat);
	ucEEPROMProp->setUnitSuffix("B");
	ucEEPROMProp->setIntegerSignedness(false);

	addTempMaxProperty(ucCat);

	PartProperty* ucVminProp = new PartProperty("vmin", tr("Minimum Voltage"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits,
			ucCat);
	ucVminProp->setUnitSuffix("V");
	ucVminProp->setDecimalConstraint(18, 6);

	PartProperty* ucVmaxProp = new PartProperty("vmax", tr("Maximum Voltage"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits,
			ucCat);
	ucVmaxProp->setUnitSuffix("V");
	ucVmaxProp->setDecimalConstraint(18, 6);

	addVendorProperty(ucCat);
	addDatasheetsProperty(ucCat);
	addNotesProperty(ucCat);
	addNumStockProperty(ucCat);




	PartCategory* oscCat = new PartCategory("oscillator", tr("Oscillators"));
	oscCat->setDescriptionPattern("$model $frequency $type");
	cats << oscCat;

	PartProperty* oscModelProp = new PartProperty("model", tr("Model"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			oscCat);
	oscModelProp->setStringMaximumLength(128);

	PartProperty* oscTypeProp = new PartProperty("type", tr("Type"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			oscCat);
	oscTypeProp->setStringMaximumLength(32);

	addPackageProperty(oscCat);

	PartProperty* oscFreqProp = new PartProperty("frequency", tr("Frequency"), PartProperty::Integer,
			PartProperty::DisplayWithUnits | PartProperty::DescriptionProperty,
			oscCat);
	oscFreqProp->setUnitSuffix("Hz");
	oscFreqProp->setDecimalConstraint(27, 12);

	addToleranceProperty(oscCat, PartProperty::DisplayUnitPrefixPPM);
	addTempMaxProperty(oscCat);

	PartProperty* oscLoadcapProp = new PartProperty("loadcap", tr("Load Capacitance"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits,
			oscCat);
	oscLoadcapProp->setUnitSuffix("F");
	oscLoadcapProp->setDecimalConstraint(36, 24);

	PartProperty* oscVminProp = new PartProperty("vmin", tr("Minimum Voltage"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits,
			oscCat);
	oscVminProp->setUnitSuffix("V");
	oscVminProp->setDecimalConstraint(18, 6);

	PartProperty* oscVmaxProp = new PartProperty("vmax", tr("Maximum Voltage"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits,
			oscCat);
	oscVmaxProp->setUnitSuffix("V");
	oscVmaxProp->setDecimalConstraint(18, 6);

	addVendorProperty(oscCat);
	addDatasheetsProperty(oscCat);
	addNotesProperty(oscCat);
	addNumStockProperty(oscCat);




	PartCategory* resCat = new PartCategory("resistor", tr("Resistors"));
	resCat->setDescriptionPattern("$resistance $power $tolerance $technology Resistor");
	cats << resCat;

	PartProperty* resTechProp = new PartProperty("technology", tr("Technology"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			resCat);
	resTechProp->setStringMaximumLength(32);

	PartProperty* resTypeProp = new PartProperty("type", tr("Type"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex,
			resCat);
	resTypeProp->setStringMaximumLength(64);

	PartProperty* resResProp = new PartProperty("resistance", tr("Resistance"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits | PartProperty::DescriptionProperty,
			resCat);
	resResProp->setUnitSuffix("Ohm");
	resResProp->setDecimalConstraint(40, 20);

	addPackageProperty(resCat);
	addToleranceProperty(resCat, PartProperty::DescriptionProperty | PartProperty::DisplayUnitPrefixPercent);
	addTempMaxProperty(resCat);

	PartProperty* resPowerProp = new PartProperty("power", tr("Power Rating"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits | PartProperty::DescriptionProperty,
			resCat);
	resPowerProp->setUnitSuffix("W");
	resPowerProp->setDecimalConstraint(40, 20);

	addVendorProperty(resCat);
	addDatasheetsProperty(resCat);
	addNotesProperty(resCat);
	addNumStockProperty(resCat);




	PartCategory* indCat = new PartCategory("inductor", tr("Inductors"));
	indCat->setDescriptionPattern("$inductance $type $package $technology Inductor");
	cats << indCat;

	PartProperty* indTechProp = new PartProperty("technology", tr("Technology"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty
			| PartProperty::DescriptionProperty,
			indCat);
	indTechProp->setStringMaximumLength(32);

	PartProperty* indProp = new PartProperty("inductance", tr("Inductance"), PartProperty::Decimal,
			PartProperty::DisplayWithUnits | PartProperty::DescriptionProperty,
			indCat);
	indProp->setUnitSuffix("H");
	indProp->setDecimalConstraint(24, 12);

	addPackageProperty(indCat, PartProperty::DescriptionProperty);
	addToleranceProperty(indCat, PartProperty::DisplayUnitPrefixPercent);

	PartProperty* indTypeProp = new PartProperty("type", tr("Type"), PartProperty::String,
			PartProperty::DisplayDynamicEnum | PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			indCat);
	indTypeProp->setStringMaximumLength(64);

	addTempMaxProperty(indCat);
	addVendorProperty(indCat);
	addDatasheetsProperty(indCat);
	addNotesProperty(indCat);
	addNumStockProperty(indCat);




	PartCategory* diodeCat = new PartCategory("diode", tr("Diodes"));
	diodeCat->setDescriptionPattern("$model $type Diode");
	cats << diodeCat;

	PartProperty* diodeModelProp = new PartProperty("model", tr("Model"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			diodeCat);
	diodeModelProp->setStringMaximumLength(128);

	PartProperty* diodeTypeProp = new PartProperty("type", tr("Type"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DisplayDynamicEnum | PartProperty::DescriptionProperty,
			diodeCat);
	diodeTypeProp->setStringMaximumLength(64);

	addVendorProperty(diodeCat);
	addDatasheetsProperty(diodeCat);
	addNotesProperty(diodeCat);
	addNumStockProperty(diodeCat);




	PartCategory* transCat = new PartCategory("transistor", tr("Transistors"));
	transCat->setDescriptionPattern("$model $type Transistor");
	cats << transCat;

	PartProperty* transModelProp = new PartProperty("model", tr("Model"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			transCat);
	transModelProp->setStringMaximumLength(128);

	PartProperty* transTypeProp = new PartProperty("type", tr("Type"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DisplayDynamicEnum | PartProperty::DescriptionProperty,
			transCat);
	transTypeProp->setStringMaximumLength(64);

	addVendorProperty(transCat);
	addDatasheetsProperty(transCat);
	addNotesProperty(transCat);
	addNumStockProperty(transCat);




	PartCategory* miscCat = new PartCategory("misc", tr("Miscellaneous"));
	miscCat->setDescriptionPattern("$name");
	cats << miscCat;

	PartProperty* miscNameProp = new PartProperty("name", tr("Name"), PartProperty::String,
			PartProperty::FullTextIndex | PartProperty::DescriptionProperty,
			miscCat);
	miscNameProp->setStringMaximumLength(128);

	addKeywordsProperty(miscCat);
	addVendorProperty(miscCat);
	addDatasheetsProperty(miscCat);
	addNotesProperty(miscCat);
	addNumStockProperty(miscCat);




	return cats;*/
}
