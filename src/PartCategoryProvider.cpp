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




void PartCategoryProvider::addNotesProperty(PartCategory* cat, flags_t addFlags)
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
}





QList<PartCategory*> PartCategoryProvider::buildCategories()
{
	QList<PartCategory*> cats;




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




	return cats;
}
