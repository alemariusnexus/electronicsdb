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

#include "../test_global.h"

#include <memory>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QTime>
#include <gtest/gtest.h>
#include <nxcommon/log.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../db/DatabaseConnection.h"
#include "../../command/sql/SQLInsertCommand.h"
#include "../../command/sql/SQLUpdateCommand.h"
#include "../../command/DummyEditCommand.h"
#include "../../command/EditStack.h"
#include "../../command/SQLEditCommand.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../model/container/PartContainer.h"
#include "../../model/container/PartContainerFactory.h"
#include "../../model/part/Part.h"
#include "../../model/part/PartFactory.h"
#include "../../model/PartCategory.h"
#include "../../model/PartLinkType.h"
#include "../../model/PartProperty.h"
#include "../../model/PartPropertyMetaType.h"
#include "../../model/StaticModelManager.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../TestManager.h"
#include "../testutil.h"

namespace electronicsdb
{


class DynamicModelTest : public ::testing::Test
{
protected:
    enum SchemaFlags
    {
        SchemaStatic = 0x01,
        SchemaContainers = 0x02,
        SchemaParts = 0x04,
        SchemaPartContainerAssocs = 0x08,
        SchemaPartLinkAssocs = 0x10,

        SchemaAll = SchemaStatic | SchemaContainers | SchemaParts | SchemaPartContainerAssocs | SchemaPartLinkAssocs
    };

protected:
    DynamicModelTest()
            : dbw(nullptr)
    {
    }

    void SetUp() override
    {
        System& sys = *System::getInstance();
        EditStack* editStack = sys.getEditStack();

        editStack->setUndoStackSize(100);
        editStack->setRedoStackSize(100);
    }

    void TearDown() override
    {
        delete dbw;
    }

    void setupBaseSchema(const QString& dbSuffix, int flags)
    {
        System& sys = *System::getInstance();
        TestManager& testMgr = TestManager::getInstance();

        db = QSqlDatabase();
        delete dbw;

        testMgr.openTestDatabase(dbSuffix);

        db = QSqlDatabase::database();
        dbw = SQLDatabaseWrapperFactory::getInstance().create(db);

        setupAppModel(flags);
        setupTestData(flags);
    }

private:
    void setupAppModel(int flags)
    {
        if ((flags & SchemaStatic) == 0) {
            return;
        }

        System& sys = *System::getInstance();
        TestManager& testMgr = TestManager::getInstance();
        TestEventSniffer& evtSniffer = testMgr.getEventSniffer();

        StaticModelManager& modelMgr = *StaticModelManager::getInstance();

        QList<PartPropertyMetaType*> mtypes;
        PartPropertyMetaType* uintType = nullptr;
        PartPropertyMetaType* resType = nullptr;
        PartPropertyMetaType* freqType = nullptr;
        PartPropertyMetaType* dynEnumType = nullptr;
        PartPropertyMetaType* bytesizeType = nullptr;
        PartPropertyMetaType* notesType = nullptr;
        PartPropertyMetaType* docType = nullptr;

        QList<PartCategory*> pcats;
        QStringList schemaStmts;

        PartCategory* pcatRes = nullptr;
        PartCategory* pcatMicro = nullptr;
        PartCategory* pcatDsheet = nullptr;

        {
            uintType = new PartPropertyMetaType("uint", PartProperty::Integer);
            uintType->intRangeMin = 0;
            mtypes << uintType;

            resType = new PartPropertyMetaType("resistance", PartProperty::Float);
            resType->userReadableName = "Resistance";
            resType->unitSuffix = "Ohm";
            resType->floatRangeMin = 0.0;
            mtypes << resType;

            freqType = new PartPropertyMetaType("frequency", PartProperty::Float);
            freqType->userReadableName = "Frequency";
            freqType->unitSuffix = "Hz";
            freqType->floatRangeMin = 0.0;
            mtypes << freqType;

            dynEnumType = new PartPropertyMetaType("dynenum", PartProperty::String);
            dynEnumType->flagsWithNonNullValue = PartProperty::DisplayDynamicEnum;
            dynEnumType->flags = PartProperty::DisplayDynamicEnum;
            mtypes << dynEnumType;

            bytesizeType = new PartPropertyMetaType("bytesize", PartProperty::Integer, uintType);
            bytesizeType->unitSuffix = "B";
            bytesizeType->unitPrefixType = PartProperty::UnitPrefixSIBase2;
            bytesizeType->flagsWithNonNullValue = PartProperty::SIPrefixesDefaultToBase2;
            bytesizeType->flags = PartProperty::SIPrefixesDefaultToBase2;
            mtypes << bytesizeType;

            notesType = new PartPropertyMetaType("notes", PartProperty::String);
            notesType->userReadableName = "Notes";
            notesType->flagsWithNonNullValue = PartProperty::DisplaySelectionList
                    | PartProperty::DisplayHideFromListingTable | PartProperty::DisplayTextArea;
            notesType->flags = PartProperty::DisplayHideFromListingTable | PartProperty::DisplayTextArea;
            mtypes << notesType;

            docType = new PartPropertyMetaType("document", PartProperty::File);
            docType->flagsWithNonNullValue = PartProperty::DisplaySelectionList
                    | PartProperty::DisplayHideFromListingTable;
            docType->flags = PartProperty::DisplayHideFromListingTable;
            mtypes << docType;

            for (PartPropertyMetaType* mtype : mtypes) {
                schemaStmts << modelMgr.generateMetaTypeCode(mtype);
            }
        }

        {
            PartCategory* pcat = new PartCategory("resistor", "Resistors");
            pcat->setDescriptionPattern("Resistor $resistance ($type, $tolerance)");

            PartProperty* resProp = new PartProperty("resistance", resType, pcat);
            resProp->setSortIndex(100);

            PartProperty* typeProp = new PartProperty("type", dynEnumType, pcat);
            typeProp->setStringMaximumLength(64);
            typeProp->setUserReadableName("Type");
            typeProp->setSortIndex(200);

            PartProperty* tolProp = new PartProperty("tolerance", PartProperty::Float, pcat);
            tolProp->setUserReadableName("Tolerance");
            tolProp->setDecimalMinimum(0.0);
            tolProp->setUnitPrefixType(PartProperty::UnitPrefixPercent);
            tolProp->setSortIndex(300);

            PartProperty* notesProp = new PartProperty("notes", notesType, pcat);
            notesProp->setStringMaximumLength(32767);
            notesProp->setSortIndex(400);

            PartProperty* stockProp = new PartProperty("numstock", uintType, pcat);
            stockProp->setUserReadableName("Number In Stock");
            stockProp->setSortIndex(500);

            pcatRes = pcat;
            pcats << pcat;
            schemaStmts << modelMgr.generateCategorySchemaDeltaCode({pcat}, {}, {}, {});
        }

        {
            PartCategory* pcat = new PartCategory("microcontroller", "Microcontrollers");
            pcat->setDescriptionPattern("$model");

            PartProperty* modelProp = new PartProperty("model", dynEnumType, pcat);
            modelProp->setUserReadableName("Model");
            modelProp->setStringMaximumLength(128);
            modelProp->setSortIndex(100);

            PartProperty* freqProp = new PartProperty("freqmax", freqType, pcat);
            freqProp->setUserReadableName("Max. CPU Frequency");
            freqProp->setSortIndex(200);

            PartProperty* ramProp = new PartProperty("ramsize", bytesizeType, pcat);
            ramProp->setUserReadableName("RAM Size");
            ramProp->setSortIndex(300);

            PartProperty* vminProp = new PartProperty("vmin", PartProperty::Decimal, pcat);
            vminProp->setUserReadableName("Min. Voltage");
            vminProp->setUnitSuffix("V");
            vminProp->setSortIndex(400);

            PartProperty* periphsProp = new PartProperty("peripherals", PartProperty::String, pcat);
            periphsProp->setUserReadableName("Peripherals");
            periphsProp->setStringMaximumLength(128);
            periphsProp->getMetaType()->flagsWithNonNullValue = PartProperty::MultiValue
                    | PartProperty::DisplayDynamicEnum;
            periphsProp->getMetaType()->flags = PartProperty::MultiValue | PartProperty::DisplayDynamicEnum;
            periphsProp->setSortIndex(500);

            PartProperty* stockProp = new PartProperty("numstock", uintType, pcat);
            stockProp->setUserReadableName("Number In Stock");
            stockProp->setSortIndex(600);

            PartProperty* nullProp = new PartProperty("NULL", PartProperty::Integer, pcat);
            nullProp->setUserReadableName("(Just testing SQL keywords as property names)");
            nullProp->setSortIndex(700);

            PartProperty* kwProp = new PartProperty("keywords", PartProperty::String, pcat);
            kwProp->setUserReadableName("Keywords");
            kwProp->setStringMaximumLength(64);
            kwProp->getMetaType()->flagsWithNonNullValue = PartProperty::MultiValue;
            kwProp->getMetaType()->flags = PartProperty::MultiValue;
            kwProp->setSortIndex(800);

            pcatMicro = pcat;
            pcats << pcat;
            schemaStmts << modelMgr.generateCategorySchemaDeltaCode({pcat}, {}, {}, {});
        }

        {
            PartCategory* pcat = new PartCategory("datasheet", "Datasheets");
            pcat->setDescriptionPattern("$title");

            PartProperty* titleProp = new PartProperty("title", PartProperty::String, pcat);
            titleProp->setStringMaximumLength(512);
            titleProp->setSortIndex(100);

            PartProperty* fileProp = new PartProperty("file", docType, pcat);
            fileProp->setSortIndex(200);

            PartProperty* addTimeProp = new PartProperty("addtime", PartProperty::DateTime, pcat);
            addTimeProp->setUserReadableName("Add Time");
            addTimeProp->setSortIndex(300);

            PartProperty* pubDateProp = new PartProperty("pubdate", PartProperty::Date, pcat);
            pubDateProp->setUserReadableName("Publication Date");
            pubDateProp->setSortIndex(400);

            PartProperty* pubTimeProp = new PartProperty("pubtime", PartProperty::Time, pcat);
            pubTimeProp->setUserReadableName("Publication Time");
            pubTimeProp->setSortIndex(500);

            pcatDsheet = pcat;
            pcats << pcat;
            schemaStmts << modelMgr.generateCategorySchemaDeltaCode({pcat}, {}, {}, {});
        }

        for (PartPropertyMetaType* mtype : mtypes) {
            delete mtype;
        }
        mtypes.clear();

        {
            PartLinkType* datasheetLink = new PartLinkType("part_datasheet", {pcatDsheet}, {pcatDsheet},
                                                           PartLinkType::PatternTypeBNegative);
            datasheetLink->setNameA("Parts");
            datasheetLink->setNameB("Datasheets");
            datasheetLink->setFlags(PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInB
                    | PartLinkType::DisplaySelectionListA);
            datasheetLink->setSortIndexInCategory(pcatDsheet, 20000);
            datasheetLink->setSortIndexInCategory(pcatRes, 20100);
            datasheetLink->setSortIndexInCategory(pcatMicro, 20200);

            PartLinkType* microResLink = new PartLinkType("microcontroller_resistor", {pcatMicro}, {pcatRes},
                                                          PartLinkType::PatternTypeBothPositive);
            microResLink->setNameA("Resistors");
            microResLink->setNameB("Microcontrollers");
            microResLink->setFlags(PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInA
                    | PartLinkType::EditInB | PartLinkType::DisplaySelectionListA | PartLinkType::DisplaySelectionListB);
            microResLink->setSortIndexInCategory(pcatMicro, 21000);
            microResLink->setSortIndexInCategory(pcatRes, 21100);

            // Just so we have two categories that can link by more than one link type
            PartLinkType* mlessLink = new PartLinkType("meaningless", {pcatRes}, {pcatMicro},
                                                       PartLinkType::PatternTypeBothPositive);
            mlessLink->setNameA("Microcontrollers");
            mlessLink->setNameB("Resistors");
            mlessLink->setFlags(PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInA
                    | PartLinkType::EditInB | PartLinkType::DisplaySelectionListA | PartLinkType::DisplaySelectionListB);
            mlessLink->setSortIndexInCategory(pcatRes, 22000);
            mlessLink->setSortIndexInCategory(pcatMicro, 22100);

            schemaStmts << modelMgr.generatePartLinkTypeDeltaCode({datasheetLink, microResLink, mlessLink}, {}, {});

            delete datasheetLink;
            delete microResLink;
            delete mlessLink;
        }

        for (PartCategory* pcat : pcats) {
            delete pcat;
        }

        modelMgr.executeSchemaStatements(schemaStmts);

        auto sniffToken = evtSniffer.startSniffing();

        sys.loadAppModel();

        ASSERT_GE(evtSniffer.getAppModelAboutToBeResetData().size(), 1);
        ASSERT_GE(evtSniffer.getAppModelResetData().size(), 1);
    }

    void setupTestData(int flags)
    {
        System& sys = *System::getInstance();
        TestEventSniffer& evtSniffer = TestManager::getInstance().getEventSniffer();
        PartContainerFactory& contFact = PartContainerFactory::getInstance();
        PartFactory& partFact = PartFactory::getInstance();

        PartCategory* pcatRes = sys.getPartCategory("resistor");
        PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
        PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

        PartLinkType* linkDsheet = sys.getPartLinkType("part_datasheet");
        PartLinkType* linkMicroRes = sys.getPartLinkType("microcontroller_resistor");
        PartLinkType* linkMless = sys.getPartLinkType("meaningless");

        ASSERT_TRUE(pcatRes);
        ASSERT_TRUE(pcatMicro);
        ASSERT_TRUE(pcatDsheet);

        ASSERT_TRUE(linkDsheet);
        ASSERT_TRUE(linkMicroRes);
        ASSERT_TRUE(linkMless);

        if ((flags & SchemaContainers) != 0) {
            PartContainerList conts;

            PartContainer a1(PartContainer::CreateBlankTag{});
            a1.setName("Container A1");
            conts << a1;

            PartContainer a2(PartContainer::CreateBlankTag{});
            a2.setName("Container A2");
            conts << a2;

            PartContainer b1(PartContainer::CreateBlankTag{});
            b1.setName("Container B1");
            conts << b1;

            PartContainer c1(PartContainer::CreateBlankTag{});
            c1.setName("Container C1");
            conts << c1;

            EXPECT_FALSE(a1.hasID());
            EXPECT_FALSE(a2.hasID());
            EXPECT_FALSE(b1.hasID());
            EXPECT_FALSE(c1.hasID());

            auto sniffToken = evtSniffer.startSniffing();
            contFact.insertItems(conts.begin(), conts.end());
            {
                SCOPED_TRACE("");
                evtSniffer.expectContainersChangedData(conts, {}, {});
                evtSniffer.expectNoPartsChangedData();
                sniffToken.reset();
            }

            EXPECT_TRUE(a1.hasID());
            EXPECT_TRUE(a2.hasID());
            EXPECT_TRUE(b1.hasID());
            EXPECT_TRUE(c1.hasID());

            commonContIDs.clear();
            commonContIDs["a1"] = a1.getID();
            commonContIDs["a2"] = a2.getID();
            commonContIDs["b1"] = b1.getID();
            commonContIDs["c1"] = c1.getID();

            ExpectValidIDs(commonContIDs.values());
        }

        if ((flags & SchemaParts) != 0) {
            PartList parts;

            Part r1(pcatRes, Part::CreateBlankTag{});
            r1.setData(pcatRes->getProperty("resistance"), 220);
            r1.setData(pcatRes->getProperty("type"), "Metal Film");
            r1.setData(pcatRes->getProperty("tolerance"), 0.1);
            r1.setData(pcatRes->getProperty("notes"), "For bright LEDs");
            r1.setData(pcatRes->getProperty("numstock"), 20);
            parts << r1;

            Part r2(pcatRes, Part::CreateBlankTag{});
            r2.setData(pcatRes->getProperty("resistance"), 560);
            r2.setData(pcatRes->getProperty("type"), "Metal Film");
            r2.setData(pcatRes->getProperty("tolerance"), 0.1);
            r2.setData(pcatRes->getProperty("notes"), "For dim LEDs");
            r2.setData(pcatRes->getProperty("numstock"), 15);
            parts << r2;

            Part r3(pcatRes, Part::CreateBlankTag{});
            r3.setData(pcatRes->getProperty("resistance"), 4700);
            r3.setData(pcatRes->getProperty("type"), "Metal Film");
            r3.setData(pcatRes->getProperty("tolerance"), 0.1);
            r3.setData(pcatRes->getProperty("notes"), "Your favorite pull-down");
            r3.setData(pcatRes->getProperty("numstock"), 10);
            parts << r3;

            Part r4(pcatRes, Part::CreateBlankTag{});
            r4.setData(pcatRes->getProperty("resistance"), 10e6);
            r4.setData(pcatRes->getProperty("type"), "Metal Film");
            r4.setData(pcatRes->getProperty("tolerance"), 0.1);
            r4.setData(pcatRes->getProperty("notes"), "Basically an open circuit");
            r4.setData(pcatRes->getProperty("numstock"), 5);
            parts << r4;

            Part r5(pcatRes, Part::CreateBlankTag{});
            r5.setData(pcatRes->getProperty("resistance"), 1000);
            r5.setData(pcatRes->getProperty("type"), "Metal Oxide");
            r5.setData(pcatRes->getProperty("tolerance"), 0.01);
            r5.setData(pcatRes->getProperty("notes"), "The good child");
            r5.setData(pcatRes->getProperty("numstock"), 3);
            parts << r5;

            Part r6(pcatRes, Part::CreateBlankTag{});
            r6.setData(pcatRes->getProperty("resistance"), 1000);
            r6.setData(pcatRes->getProperty("type"), "Carbon Film");
            r6.setData(pcatRes->getProperty("tolerance"), 0.2);
            r6.setData(pcatRes->getProperty("notes"), "The bad child");
            r6.setData(pcatRes->getProperty("numstock"), 3);
            parts << r6;

            Part m1(pcatMicro, Part::CreateBlankTag{});
            m1.setData(pcatMicro->getProperty("model"), "ATmega16A-PU");
            m1.setData(pcatMicro->getProperty("freqmax"), 16e6);
            m1.setData(pcatMicro->getProperty("ramsize"), 1024);
            m1.setData(pcatMicro->getProperty("vmin"), 2.7);
            m1.setData(pcatMicro->getProperty("peripherals"), QStringList{"GPIO", "I2C", "SPI"});
            m1.setData(pcatMicro->getProperty("numstock"), 2);
            m1.setData(pcatMicro->getProperty("NULL"), 101);
            parts << m1;

            Part m2(pcatMicro, Part::CreateBlankTag{});
            m2.setData(pcatMicro->getProperty("model"), "ATtiny25-20PU");
            m2.setData(pcatMicro->getProperty("freqmax"), 20e6);
            m2.setData(pcatMicro->getProperty("ramsize"), 128);
            m2.setData(pcatMicro->getProperty("vmin"), 2.7);
            m2.setData(pcatMicro->getProperty("peripherals"), QStringList{"GPIO", "I2C"});
            m2.setData(pcatMicro->getProperty("numstock"), 5);
            m2.setData(pcatMicro->getProperty("NULL"), 102);
            parts << m2;

            Part m3(pcatMicro, Part::CreateBlankTag{});
            m3.setData(pcatMicro->getProperty("model"), "STM32 F405VGT6");
            m3.setData(pcatMicro->getProperty("freqmax"), 168e6);
            m3.setData(pcatMicro->getProperty("ramsize"), 192*1024);
            m3.setData(pcatMicro->getProperty("vmin"), 1.8);
            m3.setData(pcatMicro->getProperty("peripherals"), QStringList{"GPIO", "I2C", "SPI", "PWM", "SDIO"});
            m3.setData(pcatMicro->getProperty("numstock"), 3);
            m3.setData(pcatMicro->getProperty("NULL"), 103);
            parts << m3;

            Part m4(pcatMicro, Part::CreateBlankTag{});
            m4.setData(pcatMicro->getProperty("model"), "ESP32-WROOM-32");
            m4.setData(pcatMicro->getProperty("freqmax"), 240e6);
            m4.setData(pcatMicro->getProperty("ramsize"), 520*1024);
            m4.setData(pcatMicro->getProperty("vmin"), 3.0);
            m4.setData(pcatMicro->getProperty("peripherals"), QStringList{"GPIO", "I2C", "SPI", "PWM", "SDIO", "WIFI",
                                                                        "Bluetooth", "Ethernet PHY"});
            m4.setData(pcatMicro->getProperty("numstock"), 2);
            m4.setData(pcatMicro->getProperty("NULL"), 104);
            parts << m4;

            Part dr1a(pcatDsheet, Part::CreateBlankTag{});
            dr1a.setData(pcatDsheet->getProperty("title"), "Resistor Datasheet (220)");
            dr1a.setData(pcatDsheet->getProperty("file"), "res220.pdf");
            parts << dr1a;

            Part dr2a(pcatDsheet, Part::CreateBlankTag{});
            dr2a.setData(pcatDsheet->getProperty("title"), "Resistor Datasheet (560)");
            dr2a.setData(pcatDsheet->getProperty("file"), "res560.pdf");
            parts << dr2a;

            Part dr3a(pcatDsheet, Part::CreateBlankTag{});
            dr3a.setData(pcatDsheet->getProperty("title"), "Resistor Datasheet (4.7k)");
            dr3a.setData(pcatDsheet->getProperty("file"), "res4k7.pdf");
            parts << dr3a;

            Part dr4a(pcatDsheet, Part::CreateBlankTag{});
            dr4a.setData(pcatDsheet->getProperty("title"), "Resistor Datasheet (10M)");
            dr4a.setData(pcatDsheet->getProperty("file"), "res10M.pdf");
            parts << dr4a;

            Part dr56a(pcatDsheet, Part::CreateBlankTag{});
            dr56a.setData(pcatDsheet->getProperty("title"), "Resistor Datasheet (1k good/bad)");
            dr56a.setData(pcatDsheet->getProperty("file"), "res1k.pdf");
            parts << dr56a;

            Part dm1a(pcatDsheet, Part::CreateBlankTag{});
            dm1a.setData(pcatDsheet->getProperty("title"), "ATmega16-A Datasheet");
            dm1a.setData(pcatDsheet->getProperty("file"), "atmega16-a-ds.pdf");
            parts << dm1a;

            Part dm2a(pcatDsheet, Part::CreateBlankTag{});
            dm2a.setData(pcatDsheet->getProperty("title"), "ATtiny25-20 Datasheet");
            dm2a.setData(pcatDsheet->getProperty("file"), "attiny25-20-ds.pdf");
            parts << dm2a;

            Part dm12b(pcatDsheet, Part::CreateBlankTag{});
            dm12b.setData(pcatDsheet->getProperty("title"), "AVR Instruction Set Manual");
            dm12b.setData(pcatDsheet->getProperty("file"), "avr-ism.pdf");
            parts << dm12b;

            Part dm3a(pcatDsheet, Part::CreateBlankTag{});
            dm3a.setData(pcatDsheet->getProperty("title"), "STM32 F405VGT6 Datasheet");
            dm3a.setData(pcatDsheet->getProperty("file"), "STM32-F405VGT6-ds.pdf");
            parts << dm3a;

            Part dm3b(pcatDsheet, Part::CreateBlankTag{});
            dm3b.setData(pcatDsheet->getProperty("title"), "STM32 F405VGT6 Application Notes");
            dm3b.setData(pcatDsheet->getProperty("file"), "STM32-F405VGT6-appnotes.pdf");
            parts << dm3b;

            Part dm4a(pcatDsheet, Part::CreateBlankTag{});
            dm4a.setData(pcatDsheet->getProperty("title"), "ESP32-WROOM-32 Datasheet");
            dm4a.setData(pcatDsheet->getProperty("file"), "esp32-wroom-32-ds.pdf");
            parts << dm4a;

            Part dm4b(pcatDsheet, Part::CreateBlankTag{});
            dm4b.setData(pcatDsheet->getProperty("title"), "ES32 Datasheet");
            dm4b.setData(pcatDsheet->getProperty("file"), "esp32-ds.pdf");
            parts << dm4b;

            Part dm4c(pcatDsheet, Part::CreateBlankTag{});
            dm4c.setData(pcatDsheet->getProperty("title"), "ES32 Technical Specifications");
            dm4c.setData(pcatDsheet->getProperty("file"), "esp32-techspecs.pdf");
            parts << dm4c;

            auto sniffToken = evtSniffer.startSniffing();
            partFact.insertItems(parts.begin(), parts.end());
            {
                SCOPED_TRACE("");
                evtSniffer.expectPartsChangedData(parts, {}, {});
                evtSniffer.expectNoContainersChangedData();
                sniffToken.reset();
            }

            commonPartIDs.clear();
            commonPartIDs["r1"] = r1.getID();
            commonPartIDs["r2"] = r2.getID();
            commonPartIDs["r3"] = r3.getID();
            commonPartIDs["r4"] = r4.getID();
            commonPartIDs["r5"] = r5.getID();
            commonPartIDs["r6"] = r6.getID();
            commonPartIDs["m1"] = m1.getID();
            commonPartIDs["m2"] = m2.getID();
            commonPartIDs["m3"] = m3.getID();
            commonPartIDs["m4"] = m4.getID();
            commonPartIDs["dr1a"] = dr1a.getID();
            commonPartIDs["dr2a"] = dr2a.getID();
            commonPartIDs["dr3a"] = dr3a.getID();
            commonPartIDs["dr4a"] = dr4a.getID();
            commonPartIDs["dr56a"] = dr56a.getID();
            commonPartIDs["dm1a"] = dm1a.getID();
            commonPartIDs["dm2a"] = dm2a.getID();
            commonPartIDs["dm12b"] = dm12b.getID();
            commonPartIDs["dm3a"] = dm3a.getID();
            commonPartIDs["dm3b"] = dm3b.getID();
            commonPartIDs["dm4a"] = dm4a.getID();
            commonPartIDs["dm4b"] = dm4b.getID();
            commonPartIDs["dm4c"] = dm4c.getID();

            ExpectValidIDs(commonPartIDs.values(), false);
        }

        if ((flags & SchemaPartContainerAssocs) != 0) {
            PartContainer ca1 = contFact.findContainerByID(commonContIDs["a1"]);
            PartContainer ca2 = contFact.findContainerByID(commonContIDs["a2"]);
            PartContainer cb1 = contFact.findContainerByID(commonContIDs["b1"]);
            PartContainer cc1 = contFact.findContainerByID(commonContIDs["c1"]);

            Part r1 = partFact.findPartByID(pcatRes, commonPartIDs["r1"]);
            Part r2 = partFact.findPartByID(pcatRes, commonPartIDs["r2"]);
            Part r3 = partFact.findPartByID(pcatRes, commonPartIDs["r3"]);
            Part r4 = partFact.findPartByID(pcatRes, commonPartIDs["r4"]);
            Part r5 = partFact.findPartByID(pcatRes, commonPartIDs["r5"]);
            Part r6 = partFact.findPartByID(pcatRes, commonPartIDs["r6"]);

            Part m1 = partFact.findPartByID(pcatMicro, commonPartIDs["m1"]);
            Part m2 = partFact.findPartByID(pcatMicro, commonPartIDs["m2"]);
            Part m3 = partFact.findPartByID(pcatMicro, commonPartIDs["m3"]);
            Part m4 = partFact.findPartByID(pcatMicro, commonPartIDs["m4"]);

            Part dr1a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr1a"]);
            Part dr2a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr2a"]);
            Part dr3a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr3a"]);
            Part dr4a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr4a"]);
            Part dr56a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr56a"]);
            Part dm1a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm1a"]);
            Part dm2a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm2a"]);
            Part dm12b = partFact.findPartByID(pcatDsheet, commonPartIDs["dm12b"]);
            Part dm3a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm3a"]);
            Part dm3b = partFact.findPartByID(pcatDsheet, commonPartIDs["dm3b"]);
            Part dm4a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm4a"]);
            Part dm4b = partFact.findPartByID(pcatDsheet, commonPartIDs["dm4b"]);
            Part dm4c = partFact.findPartByID(pcatDsheet, commonPartIDs["dm4c"]);

            ca1.loadParts({r1, r2, r3});
            ca2.loadParts({r4});
            cb1.loadParts({});
            cc1.loadParts({m3, m4});

            PartContainerList conts = {ca1, ca2, cb1, cc1};

            for (const PartContainer& cont : conts) {
                ASSERT_TRUE(cont.isPartAssocsLoaded());
            }

            auto sniffToken = evtSniffer.startSniffing();
            contFact.updateItems(conts.begin(), conts.end());
            {
                SCOPED_TRACE("");
                evtSniffer.expectContainersChangedData({}, {ca1, ca2, cc1}, {});
                evtSniffer.expectPartsChangedData({}, {r1, r2, r3, r4, m3, m4}, {});
                sniffToken.reset();
            }
        }

        if ((flags & SchemaPartLinkAssocs) != 0) {
            PartList partsRes = {
                    partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                    partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                    partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                    partFact.findPartByID(pcatRes, commonPartIDs["r4"]),
                    partFact.findPartByID(pcatRes, commonPartIDs["r5"]),
                    partFact.findPartByID(pcatRes, commonPartIDs["r6"])
            };
            PartList partsMicro = {
                    partFact.findPartByID(pcatMicro, commonPartIDs["m1"]),
                    partFact.findPartByID(pcatMicro, commonPartIDs["m2"]),
                    partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
                    partFact.findPartByID(pcatMicro, commonPartIDs["m4"])
            };
            PartList partsDsheet = {
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dr1a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dr2a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dr3a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dr4a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dr56a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm1a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm2a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm12b"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm3a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm3b"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm4a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm4b"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm4c"])
            };

            QList<PartList*> partLists({&partsRes, &partsMicro, &partsDsheet});
            for (PartList* parts : partLists) {
                for (Part& part : *parts) {
                    QList<Part> dummyList;
                    part.loadLinkedParts(dummyList.begin(), dummyList.end());
                    part.clearAssocDirty<Part::PartLinkAssocIdx>();
                }
            }

            auto applyPartChanges = [&](const PartList& parts, const PartList& expChangedParts) {
                auto sniffToken = evtSniffer.startSniffing();

                partFact.updateItems(parts.begin(), parts.end());

                evtSniffer.expectPartsChangedData({}, expChangedParts, {});
                evtSniffer.expectNoContainersChangedData();
                sniffToken.reset();
            };
            auto checkNothingDirty = [&]() {
                QList<PartList*> partLists({&partsRes, &partsMicro, &partsDsheet});
                for (PartList* parts : partLists) {
                    for (Part& part : *parts) {
                        EXPECT_FALSE(part.isAssocDirty<Part::PartLinkAssocIdx>());
                    }
                }
            };

            Part& r1 = partsRes[0];
            Part& r2 = partsRes[1];
            Part& r3 = partsRes[2];
            Part& r4 = partsRes[3];
            Part& r5 = partsRes[4];
            Part& r6 = partsRes[5];

            Part& m1 = partsMicro[0];
            Part& m2 = partsMicro[1];
            Part& m3 = partsMicro[2];
            Part& m4 = partsMicro[3];

            Part& dr1a = partsDsheet[0];
            Part& dr2a = partsDsheet[1];
            Part& dr3a = partsDsheet[2];
            Part& dr4a = partsDsheet[3];
            Part& dr56a = partsDsheet[4];
            Part& dm1a = partsDsheet[5];
            Part& dm2a = partsDsheet[6];
            Part& dm12b = partsDsheet[7];
            Part& dm3a = partsDsheet[8];
            Part& dm3b = partsDsheet[9];
            Part& dm4a = partsDsheet[10];
            Part& dm4b = partsDsheet[11];
            Part& dm4c = partsDsheet[12];

            {
                SCOPED_TRACE("");
                checkNothingDirty();
            }

            dr1a.linkPart(r1, {linkDsheet});
            dr2a.linkPart(r2, {linkDsheet});
            dr3a.linkPart(r3, {linkDsheet});
            dr4a.linkPart(r4, {linkDsheet});
            dr56a.linkPart(r5, {linkDsheet});
            dr56a.linkPart(r6, {linkDsheet});

            EXPECT_EQ(dr1a.getLinkedParts(), QList<Part>({r1}));
            EXPECT_TRUE(dr1a.getLinkedPartAssoc(r1));
            EXPECT_EQ(dr1a.getLinkedPartAssoc(r1).getLinkTypes(), QSet<PartLinkType*>({linkDsheet}));
            EXPECT_TRUE(dr1a.isAssocDirty<Part::PartLinkAssocIdx>());

            {
                SCOPED_TRACE("");
                applyPartChanges(partsDsheet, {dr1a, dr2a, dr3a, dr4a, dr56a, r1, r2, r3, r4, r5, r6});
                checkNothingDirty();
            }

            m1.linkPart(dm1a, {linkDsheet});
            m1.linkPart(dm12b, {linkDsheet});
            m2.linkPart(dm2a, {linkDsheet});
            m2.linkPart(dm12b, {linkDsheet});
            m3.linkPart(dm3a, {linkDsheet});
            m3.linkPart(dm3b, {linkDsheet});
            m4.linkPart(dm4a, {linkDsheet});
            m4.linkPart(dm4b, {linkDsheet});
            m4.linkPart(dm4c, {linkDsheet});

            m3.linkPart(r1, {linkMicroRes});
            m4.linkPart(r3, {linkMicroRes});

            {
                SCOPED_TRACE("");
                applyPartChanges(partsMicro, {m1, m2, m3, m4, dm1a, dm12b, dm2a, dm3a, dm3b, dm4a, dm4b, dm4c, r1, r3});
                checkNothingDirty();
            }

            r2.linkPart(m3, {linkMicroRes, linkMless});

            {
                SCOPED_TRACE("");
                applyPartChanges(partsRes, {r2, m3, dr2a}); // dr2a wasn't actually changed, but it's linked to r2 by
                                                            // the same assoc as m3, and assocs are completely rebuilt
                                                            // in the database when they change, so dr2a is logged as
                                                            // updated as well.
                checkNothingDirty();
            }
        }
    }

protected:
    QSqlDatabase db;
    SQLDatabaseWrapper* dbw;

    QMap<QString, dbid_t> commonContIDs;
    QMap<QString, dbid_t> commonPartIDs;
};


TEST_F(DynamicModelTest, PartContainer)
{
    setupBaseSchema("cont", SchemaStatic);

    System& sys = *System::getInstance();
    TestEventSniffer& evtSniffer = TestManager::getInstance().getEventSniffer();
    PartContainerFactory& contFact = PartContainerFactory::getInstance();
    EditStack* editStack = sys.getEditStack();

    QList<dbid_t> contIDs;

    DummyEditCommand* testBeginEditCmd = new DummyEditCommand;
    editStack->push(testBeginEditCmd);

    // Test implicit sharing
    {
        PartContainer a(PartContainer::CreateBlankTag{});
        a.setName("Test Container A");

        PartContainer b(PartContainer::CreateBlankTag{});
        b.setName("Test Container B");

        EXPECT_TRUE(a);
        EXPECT_TRUE(b);

        EXPECT_NE(a, b);

        PartContainer aCpy(a);
        PartContainer bCpy(b);

        EXPECT_EQ(a, aCpy);
        EXPECT_EQ(b, bCpy);

        EXPECT_NE(&a, &aCpy);
        EXPECT_NE(&b, &bCpy);

        a.setName("Renamed Container A");

        EXPECT_EQ(a, aCpy);
        EXPECT_EQ(aCpy.getName(), QString("Renamed Container A"));
    }

    // Test DB INSERT
    {
        PartContainerList conts;

        PartContainer a1(PartContainer::CreateBlankTag{});
        a1.setName("Container A1");
        conts << a1;

        PartContainer a2(PartContainer::CreateBlankTag{});
        a2.setName("Container A2");
        conts << a2;

        PartContainer b1(PartContainer::CreateBlankTag{});
        b1.setName("Container B1");
        conts << b1;

        EXPECT_FALSE(a1.hasID());
        EXPECT_FALSE(a2.hasID());
        EXPECT_FALSE(b1.hasID());

        auto sniffToken = evtSniffer.startSniffing();
        contFact.insertItems(conts.begin(), conts.end());
        {
            SCOPED_TRACE("");
            evtSniffer.expectContainersChangedData(conts, {}, {});
            evtSniffer.expectNoPartsChangedData();
            sniffToken.reset();
        }

        EXPECT_TRUE(a1.hasID());
        EXPECT_TRUE(a2.hasID());
        EXPECT_TRUE(b1.hasID());

        PartContainer c1(PartContainer::CreateBlankTag{});
        c1.setName("Container C1");

        EXPECT_FALSE(c1.hasID());

        sniffToken = evtSniffer.startSniffing();
        contFact.insertItem(c1);
        {
            SCOPED_TRACE("");
            evtSniffer.expectContainersChangedData({c1}, {}, {});
            evtSniffer.expectNoPartsChangedData();
            sniffToken.reset();
        }

        EXPECT_TRUE(c1.hasID());

        contIDs << a1.getID() << a2.getID() << b1.getID() << c1.getID();

        ExpectValidIDs(contIDs);

        PartContainerList weakConts = contFact.findContainersByID(contIDs);
        for (const PartContainer& cont : weakConts) {
            EXPECT_TRUE(cont);
            EXPECT_TRUE(cont.isNameLoaded());
        }
    }

    // Check that unreferenced containers are actually released from memory
    {
        PartContainerList weakConts = contFact.findContainersByID(contIDs);
        for (const PartContainer& cont : weakConts) {
            EXPECT_TRUE(cont);
            EXPECT_FALSE(cont.isNameLoaded());
        }
    }

    // Test loading from DB
    {
        PartContainerList allConts = contFact.loadItems(QString(), "ORDER BY name DESC");

        EXPECT_EQ(allConts.size(), 4);

        EXPECT_EQ(allConts[0].getName(), QString("Container C1"));
        EXPECT_EQ(allConts[1].getName(), QString("Container B1"));
        EXPECT_EQ(allConts[2].getName(), QString("Container A2"));
        EXPECT_EQ(allConts[3].getName(), QString("Container A1"));

        EXPECT_EQ(allConts[0].getID(), contIDs[3]);
        EXPECT_EQ(allConts[1].getID(), contIDs[2]);
        EXPECT_EQ(allConts[2].getID(), contIDs[1]);
        EXPECT_EQ(allConts[3].getID(), contIDs[0]);
    }

    // Test partial loading from DB
    {
        PartContainerList someConts = contFact.loadItems(QString("WHERE name LIKE '%A_'"), QString("ORDER BY name ASC"));

        EXPECT_EQ(someConts.size(), 2);

        EXPECT_EQ(someConts[0].getName(), QString("Container A1"));
        EXPECT_EQ(someConts[1].getName(), QString("Container A2"));

        EXPECT_EQ(someConts[0].getID(), contIDs[0]);
        EXPECT_EQ(someConts[1].getID(), contIDs[1]);
    }

    // Test loading with pre-existing objects
    {
        PartContainerList contsToLoad;

        PartContainer a1 = contFact.findContainerByID(contIDs[0]);
        PartContainer c1 = contFact.findContainerByID(contIDs[3]);

        PartContainer a2 = contFact.findContainerByID(contIDs[1]);

        contsToLoad << a1 << c1;

        EXPECT_TRUE(a1);
        EXPECT_TRUE(c1);

        EXPECT_FALSE(a1.isNameLoaded());
        EXPECT_FALSE(c1.isNameLoaded());

        EXPECT_FALSE(a2.isNameLoaded());

        contFact.loadItems(contsToLoad.begin(), contsToLoad.end());

        EXPECT_TRUE(a1.isNameLoaded());
        EXPECT_TRUE(c1.isNameLoaded());

        EXPECT_FALSE(a2.isNameLoaded());

        EXPECT_EQ(a1.getName(), QString("Container A1"));
        EXPECT_EQ(c1.getName(), QString("Container C1"));
    }

    // Test loading with empty list
    {
        PartContainerList conts;

        EXPECT_NO_THROW(contFact.loadItems(conts.begin(), conts.end()));
        EXPECT_EQ(conts.size(), 0);
    }

    // Test auto-reloading from DB
    {
        sys.truncateChangelogs();

        SQLUpdateCommand* updCmd1 = new SQLUpdateCommand("container", "id", contIDs[1]);
        updCmd1->addFieldValue("name", "Manually changed A2");

        SQLUpdateCommand* updCmd2 = new SQLUpdateCommand("container", "id", contIDs[2]);
        updCmd2->addFieldValue("name", "Manually changed B1");

        SQLEditCommand* editCmd = new SQLEditCommand;
        editCmd->addSQLCommand(updCmd1);
        editCmd->addSQLCommand(updCmd2);

        PartContainerList allConts = contFact.loadItems(QString(), "ORDER BY name DESC");
        EXPECT_EQ(allConts.size(), 4);

        EXPECT_EQ(allConts[0].getName(), QString("Container C1"));
        EXPECT_EQ(allConts[1].getName(), QString("Container B1"));
        EXPECT_EQ(allConts[2].getName(), QString("Container A2"));
        EXPECT_EQ(allConts[3].getName(), QString("Container A1"));

        auto sniffToken = evtSniffer.startSniffing();

        // This should trigger an auto-reload
        editStack->push(editCmd);

        {
            SCOPED_TRACE("");
            evtSniffer.expectContainersChangedData({}, {
                contFact.findContainerByID(contIDs[1]),
                contFact.findContainerByID(contIDs[2])
                }, {});
            evtSniffer.expectNoPartsChangedData();
            sniffToken.reset();
        }

        EXPECT_EQ(allConts[0].getName(), QString("Container C1"));
        EXPECT_EQ(allConts[1].getName(), QString("Manually changed B1"));
        EXPECT_EQ(allConts[2].getName(), QString("Manually changed A2"));
        EXPECT_EQ(allConts[3].getName(), QString("Container A1"));
    }

    // Test updating in DB
    {
        {
            PartContainerList allConts = contFact.loadItems(QString(), "ORDER BY id ASC");

            allConts[0].setName("Programmatically changed A1");
            allConts[2].setName("Programmatically changed B1");

            PartContainerList updConts = {allConts[0], allConts[2]};

            auto sniffToken = evtSniffer.startSniffing();
            contFact.updateItems(updConts.begin(), updConts.end());
            {
                SCOPED_TRACE("");
                evtSniffer.expectContainersChangedData({}, updConts, {});
                evtSniffer.expectNoPartsChangedData();
                sniffToken.reset();
            }

            EXPECT_EQ(allConts[0].getName(), QString("Programmatically changed A1"));
            EXPECT_EQ(allConts[1].getName(), QString("Manually changed A2"));
            EXPECT_EQ(allConts[2].getName(), QString("Programmatically changed B1"));
            EXPECT_EQ(allConts[3].getName(), QString("Container C1"));
        }

        {
            PartContainerList allConts = contFact.loadItems(QString(), "ORDER BY id ASC");

            EXPECT_EQ(allConts[0].getName(), QString("Programmatically changed A1"));
            EXPECT_EQ(allConts[1].getName(), QString("Manually changed A2"));
            EXPECT_EQ(allConts[2].getName(), QString("Programmatically changed B1"));
            EXPECT_EQ(allConts[3].getName(), QString("Container C1"));
        }
    }

    // Test deleting in DB
    {
        {
            PartContainerList allConts = contFact.loadItems(QString(), "ORDER BY id ASC");
            EXPECT_EQ(allConts.size(), 4);

            PartContainerList delConts = {allConts[0], allConts[3]};

            auto sniffToken = evtSniffer.startSniffing();
            contFact.deleteItems(delConts.begin(), delConts.end());
            {
                SCOPED_TRACE("");
                evtSniffer.expectContainersChangedData({}, {}, delConts);
                evtSniffer.expectNoPartsChangedData();
                sniffToken.reset();
            }
        }

        {
            PartContainerList allConts = contFact.loadItems(QString(), "ORDER BY id ASC");
            EXPECT_EQ(allConts.size(), 2);

            EXPECT_EQ(allConts[0].getName(), QString("Manually changed A2"));
            EXPECT_EQ(allConts[1].getName(), QString("Programmatically changed B1"));

            EXPECT_EQ(allConts[0].getID(), contIDs[1]);
            EXPECT_EQ(allConts[1].getID(), contIDs[2]);
        }
    }

    // Insert some more containers, checking that IDs aren't reused
    {
        {
            PartContainer z1(PartContainer::CreateBlankTag{});
            z1.setName("Container Z1");

            auto sniffToken = evtSniffer.startSniffing();
            contFact.insertItem(z1);
            {
                SCOPED_TRACE("");
                evtSniffer.expectContainersChangedData({z1}, {}, {});
                evtSniffer.expectNoPartsChangedData();
                sniffToken.reset();
            }
        }

        {
            PartContainerList allConts = contFact.loadItems(QString(), "ORDER BY id ASC");
            EXPECT_EQ(allConts.size(), 3);

            ExpectValidIDs(QList<dbid_t>() << allConts[0].getID() << allConts[1].getID() << allConts[2].getID());
        }
    }

    // Undo and redo everything a few times
    {
        for (int i = 0 ; i < 2 ; i++) {
            PartContainerList allConts = contFact.loadItems(QString(), QString());
            EXPECT_EQ(allConts.size(), 3);

            EXPECT_EQ(allConts[0].getName(), QString("Manually changed A2"));
            EXPECT_EQ(allConts[1].getName(), QString("Programmatically changed B1"));
            EXPECT_EQ(allConts[2].getName(), QString("Container Z1"));

            auto sniffToken = evtSniffer.startSniffing();

            ASSERT_TRUE(editStack->undoUntil(testBeginEditCmd));

            EXPECT_GT(evtSniffer.getContainersChangedData().size(), 0);
            sniffToken.reset();

            allConts = contFact.loadItems(QString(), QString());
            EXPECT_EQ(allConts.size(), 0);

            sniffToken = evtSniffer.startSniffing();

            ASSERT_TRUE(editStack->redoUntil(nullptr));

            EXPECT_GT(evtSniffer.getContainersChangedData().size(), 0);
            sniffToken.reset();

            allConts = contFact.loadItems(QString(), QString("ORDER BY id ASC"));
            EXPECT_EQ(allConts.size(), 3);

            EXPECT_EQ(allConts[0].getName(), QString("Manually changed A2"));
            EXPECT_EQ(allConts[1].getName(), QString("Programmatically changed B1"));
            EXPECT_EQ(allConts[2].getName(), QString("Container Z1"));
        }
    }
}

TEST_F(DynamicModelTest, Part)
{
    setupBaseSchema("part", SchemaStatic | SchemaParts);

    System& sys = *System::getInstance();
    PartFactory& partFact = PartFactory::getInstance();
    TestEventSniffer& evtSniffer = TestManager::getInstance().getEventSniffer();
    EditStack* editStack = sys.getEditStack();

    editStack->clear();

    DummyEditCommand* testBeginEditCmd = new DummyEditCommand;
    editStack->push(testBeginEditCmd);

    PartCategory* pcatRes = sys.getPartCategory("resistor");
    PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
    PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

    ASSERT_TRUE(pcatRes);
    ASSERT_TRUE(pcatMicro);
    ASSERT_TRUE(pcatDsheet);

    // Some more quick static model tests
    {
        EXPECT_NE((pcatRes->getProperty("type")->getFlags() & PartProperty::DisplayDynamicEnum), 0);
        EXPECT_NE((pcatMicro->getProperty("model")->getFlags() & PartProperty::DisplayDynamicEnum), 0);
    }

    // Test implicit sharing
    {
        Part a(pcatRes, Part::CreateBlankTag{});
        a.setData(pcatRes->getProperty("resistance"), 123);

        Part b(pcatRes, Part::CreateBlankTag{});
        b.setData(pcatRes->getProperty("resistance"), 987);

        EXPECT_TRUE(a);
        EXPECT_TRUE(b);

        EXPECT_NE(a, b);

        Part aCpy(a);
        Part bCpy(b);

        EXPECT_EQ(a, aCpy);
        EXPECT_EQ(b, bCpy);

        EXPECT_NE(&a, &aCpy);
        EXPECT_NE(&b, &bCpy);

        a.setData(pcatRes->getProperty("resistance"), 555);

        EXPECT_EQ(a, aCpy);
        EXPECT_EQ(aCpy.getData(pcatRes->getProperty("resistance")).toInt(), 555);
    }

    // Check that unreferenced parts are actually released from memory
    {
        Part m2 = partFact.findPartByID(pcatMicro, commonPartIDs["m2"]);
        EXPECT_TRUE(m2);
        EXPECT_FALSE(m2.isDataLoaded(pcatMicro->getProperty("model")));
    }

    // Test single-category full loading from DB
    {
        PartList micros = partFact.loadItemsSingleCategory(pcatRes, QString(), QString(), QString());
        EXPECT_EQ(micros.size(), 6);
    }

    // Test single-category limit loading from DB
    {
        PartList micros = partFact.loadItemsSingleCategory(pcatRes, QString(), "ORDER BY resistance ASC",
                                                           "LIMIT 4 OFFSET 1");
        EXPECT_EQ(micros.size(), 4);

        EXPECT_EQ(micros[0].getData(pcatRes->getProperty("resistance")).toLongLong(), 560);
        EXPECT_EQ(micros[1].getData(pcatRes->getProperty("resistance")).toLongLong(), 1000);
        EXPECT_EQ(micros[2].getData(pcatRes->getProperty("resistance")).toLongLong(), 1000);
        EXPECT_EQ(micros[3].getData(pcatRes->getProperty("resistance")).toLongLong(), 4700);
    }

    // Test single-category partial loading from DB
    {
        PartList micros = partFact.loadItemsSingleCategory(pcatMicro, "WHERE ramsize >= 1024", "ORDER BY model ASC",
                                                           QString());
        EXPECT_EQ(micros.size(), 3);

        EXPECT_TRUE(micros[0].isDataFullyLoaded());
        EXPECT_TRUE(micros[1].isDataFullyLoaded());
        EXPECT_TRUE(micros[2].isDataFullyLoaded());

        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("model")).toString(), QString("ATmega16A-PU"));
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("freqmax")).toLongLong(), 16e6);
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("ramsize")).toLongLong(), 1024);
        EXPECT_DOUBLE_EQ(micros[0].getData(pcatMicro->getProperty("vmin")).toDouble(), 2.7);
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>() << QString("GPIO") << QString("I2C") << QString("SPI"));
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("numstock")).toLongLong(), 2);
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("NULL")).toLongLong(), 101);
        EXPECT_EQ(micros[0].getDescription(), QString("ATmega16A-PU"));

        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("model")).toString(), QString("ESP32-WROOM-32"));
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("freqmax")).toLongLong(), 240e6);
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("ramsize")).toLongLong(), 520*1024);
        EXPECT_DOUBLE_EQ(micros[1].getData(pcatMicro->getProperty("vmin")).toDouble(), 3.0);
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"GPIO", "I2C", "SPI", "PWM", "SDIO", "WIFI", "Bluetooth", "Ethernet PHY"}));
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("numstock")).toLongLong(), 2);
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("NULL")).toLongLong(), 104);
        EXPECT_EQ(micros[1].getDescription(), QString("ESP32-WROOM-32"));

        EXPECT_EQ(micros[2].getData(pcatMicro->getProperty("model")).toString(), QString("STM32 F405VGT6"));
        EXPECT_EQ(micros[2].getData(pcatMicro->getProperty("freqmax")).toLongLong(), 168e6);
        EXPECT_EQ(micros[2].getData(pcatMicro->getProperty("ramsize")).toLongLong(), 192*1024);
        EXPECT_DOUBLE_EQ(micros[2].getData(pcatMicro->getProperty("vmin")).toDouble(), 1.8);
        EXPECT_EQ(micros[2].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"GPIO", "I2C", "SPI", "PWM", "SDIO"}));
        EXPECT_EQ(micros[2].getData(pcatMicro->getProperty("numstock")).toLongLong(), 3);
        EXPECT_EQ(micros[2].getData(pcatMicro->getProperty("NULL")).toLongLong(), 103);
        EXPECT_EQ(micros[2].getDescription(), QString("STM32 F405VGT6"));
    }

    // Test single-category loading by multi-value property
    {
        PartList micros = partFact.loadItemsSingleCategory(pcatMicro, "WHERE peripherals='SDIO'", "ORDER BY model ASC",
                                                           QString());
        EXPECT_EQ(micros.size(), 2);

        EXPECT_TRUE(micros[0].isDataFullyLoaded());
        EXPECT_TRUE(micros[1].isDataFullyLoaded());

        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("model")).toString(), QString("ESP32-WROOM-32"));
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("freqmax")).toLongLong(), 240e6);
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("ramsize")).toLongLong(), 520*1024);
        EXPECT_DOUBLE_EQ(micros[0].getData(pcatMicro->getProperty("vmin")).toDouble(), 3.0);
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"GPIO", "I2C", "SPI", "PWM", "SDIO", "WIFI", "Bluetooth", "Ethernet PHY"}));
        EXPECT_EQ(micros[0].getData(pcatMicro->getProperty("numstock")).toLongLong(), 2);

        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("model")).toString(), QString("STM32 F405VGT6"));
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("freqmax")).toLongLong(), 168e6);
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("ramsize")).toLongLong(), 192*1024);
        EXPECT_DOUBLE_EQ(micros[1].getData(pcatMicro->getProperty("vmin")).toDouble(), 1.8);
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"GPIO", "I2C", "SPI", "PWM", "SDIO"}));
        EXPECT_EQ(micros[1].getData(pcatMicro->getProperty("numstock")).toLongLong(), 3);
    }

    // Test single-category loading from DB into pre-existing objects
    {
        Part r3 = partFact.findPartByID(pcatRes, commonPartIDs["r3"]);
        Part r4 = partFact.findPartByID(pcatRes, commonPartIDs["r4"]);

        ASSERT_TRUE(r3);
        ASSERT_TRUE(r4);
        ASSERT_FALSE(r3.isDataFullyLoaded());
        ASSERT_FALSE(r4.isDataFullyLoaded());

        PartList parts = {r3, r4};
        partFact.loadItemsSingleCategory(parts.begin(), parts.end());

        EXPECT_TRUE(r3.isDataFullyLoaded());
        EXPECT_TRUE(r4.isDataFullyLoaded());

        EXPECT_EQ(r3.getID(), commonPartIDs["r3"]);
        EXPECT_EQ(r4.getID(), commonPartIDs["r4"]);

        EXPECT_EQ(r3.getData(pcatRes->getProperty("resistance")).toLongLong(), 4700);
        EXPECT_EQ(r4.getData(pcatRes->getProperty("resistance")).toLongLong(), 10e6);

        Part m1 = partFact.findPartByID(pcatMicro, commonPartIDs["m1"]);
        Part m2 = partFact.findPartByID(pcatMicro, commonPartIDs["m2"]);

        parts = {m1, m2};

        ASSERT_TRUE(m1);
        ASSERT_TRUE(m2);
        ASSERT_FALSE(m1.isDataLoaded(pcatMicro->getProperty("model")));
        ASSERT_FALSE(m2.isDataLoaded(pcatMicro->getProperty("model")));

        partFact.loadItemsSingleCategory(parts.begin(), parts.end(), 0, {
            pcatMicro->getProperty("model"), pcatMicro->getProperty("ramsize")
        });

        EXPECT_TRUE(m1.isDataLoaded(pcatMicro->getProperty("model")));
        EXPECT_TRUE(m1.isDataLoaded(pcatMicro->getProperty("ramsize")));
        EXPECT_FALSE(m1.isDataLoaded(pcatMicro->getProperty("freqmax")));
        EXPECT_FALSE(m1.isDataLoaded(pcatMicro->getProperty("vmin")));
        EXPECT_FALSE(m1.isDataLoaded(pcatMicro->getProperty("peripherals")));
        EXPECT_FALSE(m1.isDataLoaded(pcatMicro->getProperty("numstock")));
        EXPECT_EQ(m1.getData(pcatMicro->getProperty("model")).toString(), QString("ATmega16A-PU"));
        EXPECT_EQ(m1.getData(pcatMicro->getProperty("ramsize")).toLongLong(), 1024);

        EXPECT_TRUE(m2.isDataLoaded(pcatMicro->getProperty("model")));
        EXPECT_TRUE(m2.isDataLoaded(pcatMicro->getProperty("ramsize")));
        EXPECT_FALSE(m2.isDataLoaded(pcatMicro->getProperty("freqmax")));
        EXPECT_FALSE(m2.isDataLoaded(pcatMicro->getProperty("vmin")));
        EXPECT_FALSE(m2.isDataLoaded(pcatMicro->getProperty("peripherals")));
        EXPECT_FALSE(m2.isDataLoaded(pcatMicro->getProperty("numstock")));
        EXPECT_EQ(m2.getData(pcatMicro->getProperty("model")).toString(), QString("ATtiny25-20PU"));
        EXPECT_EQ(m2.getData(pcatMicro->getProperty("ramsize")).toLongLong(), 128);
    }

    // Test loading with empty list
    {
        PartList parts;

        EXPECT_NO_THROW(partFact.loadItems(parts.begin(), parts.end()));
        EXPECT_EQ(parts.size(), 0);
    }

    // Test multi-category loading
    {
        Part r5 = partFact.findPartByID(pcatRes, commonPartIDs["r5"]);
        Part r6 = partFact.findPartByID(pcatRes, commonPartIDs["r6"]);

        Part m4 = partFact.findPartByID(pcatMicro, commonPartIDs["m4"]);

        Part dr4a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr4a"]);
        Part dr56a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr56a"]);
        Part dm2a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm2a"]);

        PartList parts = {r5, m4, dr56a, dr4a, r6, dm2a};

        EXPECT_FALSE(r5.isDataLoaded(pcatRes->getProperty("resistance")));
        EXPECT_FALSE(r5.isDataLoaded(pcatRes->getProperty("type")));
        EXPECT_FALSE(r5.isDataLoaded(pcatRes->getProperty("tolerance")));

        EXPECT_FALSE(r6.isDataLoaded(pcatRes->getProperty("resistance")));
        EXPECT_FALSE(r6.isDataLoaded(pcatRes->getProperty("type")));
        EXPECT_FALSE(r6.isDataLoaded(pcatRes->getProperty("tolerance")));

        EXPECT_FALSE(m4.isDataLoaded(pcatMicro->getProperty("model")));

        EXPECT_FALSE(dr4a.isDataLoaded(pcatDsheet->getProperty("title")));
        EXPECT_FALSE(dr56a.isDataLoaded(pcatDsheet->getProperty("title")));
        EXPECT_FALSE(dm2a.isDataLoaded(pcatDsheet->getProperty("title")));

        partFact.loadItems(parts.begin(), parts.end(), 0, [](PartCategory* pcat) {
            return pcat->getDescriptionProperties();
        });

        EXPECT_FALSE(r5.isDataLoaded(pcatRes->getProperty("notes")));
        EXPECT_FALSE(r5.isDataLoaded(pcatRes->getProperty("numstock")));

        EXPECT_FALSE(r6.isDataLoaded(pcatRes->getProperty("notes")));
        EXPECT_FALSE(r6.isDataLoaded(pcatRes->getProperty("numstock")));

        EXPECT_FALSE(m4.isDataLoaded(pcatMicro->getProperty("ramsize")));
        EXPECT_FALSE(m4.isDataLoaded(pcatMicro->getProperty("peripherals")));

        EXPECT_FALSE(dr4a.isDataLoaded(pcatDsheet->getProperty("file")));
        EXPECT_FALSE(dr56a.isDataLoaded(pcatDsheet->getProperty("file")));
        EXPECT_FALSE(dm2a.isDataLoaded(pcatDsheet->getProperty("file")));

        EXPECT_EQ(r5.getData(pcatRes->getProperty("resistance")).toLongLong(), 1000);
        EXPECT_EQ(r5.getData(pcatRes->getProperty("type")).toString(), QString("Metal Oxide"));
        EXPECT_EQ(r5.getData(pcatRes->getProperty("tolerance")).toDouble(), 0.01);
        EXPECT_EQ(r5.getDescription(), QString("Resistor 1kOhm (Metal Oxide, 1%)"));

        EXPECT_EQ(r6.getData(pcatRes->getProperty("resistance")).toLongLong(), 1000);
        EXPECT_EQ(r6.getData(pcatRes->getProperty("type")).toString(), QString("Carbon Film"));
        EXPECT_EQ(r6.getData(pcatRes->getProperty("tolerance")).toDouble(), 0.2);
        EXPECT_EQ(r6.getDescription(), QString("Resistor 1kOhm (Carbon Film, 20%)"));

        EXPECT_EQ(m4.getData(pcatMicro->getProperty("model")).toString(), QString("ESP32-WROOM-32"));

        EXPECT_EQ(dr4a.getData(pcatDsheet->getProperty("title")), QString("Resistor Datasheet (10M)"));
        EXPECT_EQ(dr4a.getDescription(), QString("Resistor Datasheet (10M)"));
        EXPECT_EQ(dr56a.getData(pcatDsheet->getProperty("title")), QString("Resistor Datasheet (1k good/bad)"));
        EXPECT_EQ(dm2a.getData(pcatDsheet->getProperty("title")), QString("ATtiny25-20 Datasheet"));
    }

    // Test part cloning
    {
        Part r4 = partFact.findPartByID(pcatRes, commonPartIDs["r4"]);

        partFact.loadItems(&r4, &r4+1);

        Part r4Cloned = r4.clone();

        EXPECT_TRUE(r4Cloned);
        EXPECT_NE(r4, r4Cloned);
        EXPECT_FALSE(r4Cloned.hasID());
        EXPECT_TRUE(r4.isDataFullyLoaded());
        EXPECT_TRUE(r4Cloned.isDataFullyLoaded());
        EXPECT_EQ(r4Cloned.getData(), r4.getData());
        EXPECT_FALSE(r4.isContainerAssocsLoaded());
        EXPECT_FALSE(r4Cloned.isContainerAssocsLoaded());
        EXPECT_FALSE(r4.isDirty());
        EXPECT_TRUE(r4Cloned.isDirty());
    }

    // Test auto-reloading from DB (single-value props)
    {
        sys.truncateChangelogs();

        SQLUpdateCommand* updCmd1 = new SQLUpdateCommand("pcat_resistor", "id", commonPartIDs["r3"]);
        updCmd1->addFieldValue("type", "Heavy Metal");

        SQLUpdateCommand* updCmd2 = new SQLUpdateCommand("pcat_datasheet", "id", commonPartIDs["dr3a"]);
        updCmd2->addFieldValue("title", "Resistor Datasheet (4.7k, modified)");

        SQLEditCommand* editCmd = new SQLEditCommand;
        editCmd->addSQLCommand(updCmd1);
        editCmd->addSQLCommand(updCmd2);

        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                partFact.findPartByID(pcatDsheet, commonPartIDs["dr3a"])
        };
        partFact.loadItems(parts.begin(), parts.end());

        EXPECT_EQ(parts[0].getData(pcatRes->getProperty("resistance")).toLongLong(), 4700);
        EXPECT_EQ(parts[0].getData(pcatRes->getProperty("type")).toString(), QString("Metal Film"));

        EXPECT_EQ(parts[1].getData(pcatDsheet->getProperty("title")).toString(), QString("Resistor Datasheet (4.7k)"));

        // This should trigger an auto-reload
        editStack->push(editCmd);

        EXPECT_EQ(parts[0].getData(pcatRes->getProperty("resistance")).toLongLong(), 4700);
        EXPECT_EQ(parts[0].getData(pcatRes->getProperty("type")).toString(), QString("Heavy Metal"));

        EXPECT_EQ(parts[1].getData(pcatDsheet->getProperty("title")).toString(),
                  QString("Resistor Datasheet (4.7k, modified)"));
    }

    // Test auto-reloading from DB (multi-value props)
    {
        sys.truncateChangelogs();

        PartProperty* periphProp = pcatMicro->getProperty("peripherals");

        QMap<QString, QVariant> data1 = {
                std::pair<QString, QVariant>(periphProp->getMultiValueIDFieldName(), commonPartIDs["m3"]),
                std::pair<QString, QVariant>(periphProp->getFieldName(), "Death Ray"),
                std::pair<QString, QVariant>("idx", -10)
        };
        SQLInsertCommand* insCmd1 = new SQLInsertCommand(periphProp->getMultiValueForeignTableName(),
                                                         periphProp->getMultiValueIDFieldName(),
                                                         data1);

        SQLDeleteCommand* delCmd1 = new SQLDeleteCommand(periphProp->getMultiValueForeignTableName(),
                QString("%1=%2 AND %3='I2C'").arg(dbw->escapeIdentifier(periphProp->getMultiValueIDFieldName()))
                                             .arg(commonPartIDs["m2"])
                                             .arg(periphProp->getFieldName()));

        SQLEditCommand* editCmd = new SQLEditCommand;
        editCmd->addSQLCommand(insCmd1);
        editCmd->addSQLCommand(delCmd1);

        PartList parts = {
                partFact.findPartByID(pcatMicro, commonPartIDs["m2"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"])
        };
        partFact.loadItems(parts.begin(), parts.end());

        EXPECT_EQ(parts[0].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"GPIO", "I2C"}));
        EXPECT_EQ(parts[1].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"GPIO", "I2C", "SPI", "PWM", "SDIO"}));

        // This should trigger an auto-reload
        editStack->push(editCmd);

        EXPECT_EQ(parts[0].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"GPIO"}));
        EXPECT_EQ(parts[1].getData(pcatMicro->getProperty("peripherals")).toList(),
                  QList<QVariant>({"Death Ray", "GPIO", "I2C", "SPI", "PWM", "SDIO"}));
    }

    // Test updating in DB
    {
        {
            Part r3 = partFact.findPartByID(pcatRes, commonPartIDs["r3"]);
            Part m4 = partFact.findPartByID(pcatMicro, commonPartIDs["m4"]);

            r3.setData(pcatRes->getProperty("resistance"), 10000);
            r3.setData(pcatRes->getProperty("notes"), QString("Your NEW favorite pull-down"));

            m4.setData(pcatMicro->getProperty("peripherals"), QList<QVariant>({"Death Ray", "Fluffy Kittens"}));
            m4.setData(pcatMicro->getProperty("numstock"), 4);

            PartList parts = {r3, m4};

            auto sniffToken = evtSniffer.startSniffing();
            partFact.updateItems(parts.begin(), parts.end());
            {
                SCOPED_TRACE("");
                evtSniffer.expectPartsChangedData({}, parts, {});
                evtSniffer.expectNoContainersChangedData();
                sniffToken.reset();
            }
        }

        {
            Part r3 = partFact.findPartByID(pcatRes, commonPartIDs["r3"]);
            Part m4 = partFact.findPartByID(pcatMicro, commonPartIDs["m4"]);

            EXPECT_FALSE(r3.isDataLoaded(pcatRes->getProperty("resistance")));
            EXPECT_FALSE(r3.isDataLoaded(pcatRes->getProperty("notes")));

            EXPECT_FALSE(m4.isDataLoaded(pcatMicro->getProperty("peripherals")));
            EXPECT_FALSE(m4.isDataLoaded(pcatMicro->getProperty("numstock")));

            PartList parts = {r3, m4};
            partFact.loadItems(parts.begin(), parts.end());

            EXPECT_EQ(r3.getData(pcatRes->getProperty("resistance")).toLongLong(), 10000);
            EXPECT_EQ(r3.getData(pcatRes->getProperty("notes")).toString(), QString("Your NEW favorite pull-down"));

            EXPECT_EQ(m4.getData(pcatMicro->getProperty("model")).toString(), QString("ESP32-WROOM-32"));
            EXPECT_EQ(m4.getData(pcatMicro->getProperty("peripherals")).toList(),
                      QList<QVariant>({"Death Ray", "Fluffy Kittens"}));
            EXPECT_EQ(m4.getData(pcatMicro->getProperty("numstock")).toLongLong(), 4);
        }
    }

    // Test deleting in DB
    {
        {
            Part r4 = partFact.findPartByID(pcatRes, commonPartIDs["r4"]);
            Part m1 = partFact.findPartByID(pcatMicro, commonPartIDs["m1"]);
            Part m2 = partFact.findPartByID(pcatMicro, commonPartIDs["m2"]);

            PartList parts = {r4, m1, m2};

            auto sniffToken = evtSniffer.startSniffing();
            partFact.deleteItems(parts.begin(), parts.end());
            {
                SCOPED_TRACE("");
                evtSniffer.expectPartsChangedData({}, {}, parts);
                evtSniffer.expectNoContainersChangedData();
                sniffToken.reset();
            }
        }

        {
            PartList resParts = partFact.loadItemsSingleCategory(pcatRes, QString(), QString(), QString());
            EXPECT_EQ(resParts.size(), 5);

            QList<dbid_t> ids;
            for (Part part : resParts) {
                ids << part.getID();
            }

            EXPECT_TRUE(ids.contains(commonPartIDs["r1"]));
            EXPECT_TRUE(ids.contains(commonPartIDs["r2"]));
            EXPECT_TRUE(ids.contains(commonPartIDs["r3"]));
            EXPECT_FALSE(ids.contains(commonPartIDs["r4"]));
            EXPECT_TRUE(ids.contains(commonPartIDs["r5"]));
            EXPECT_TRUE(ids.contains(commonPartIDs["r6"]));
        }

        {
            PartList microParts = partFact.loadItemsSingleCategory(pcatMicro, QString(), QString(), QString());
            EXPECT_EQ(microParts.size(), 2);

            QList<dbid_t> ids;
            for (Part part : microParts) {
                ids << part.getID();
            }

            EXPECT_FALSE(ids.contains(commonPartIDs["m1"]));
            EXPECT_FALSE(ids.contains(commonPartIDs["m2"]));
            EXPECT_TRUE(ids.contains(commonPartIDs["m3"]));
            EXPECT_TRUE(ids.contains(commonPartIDs["m4"]));
        }
    }

    // Test inserting blank items
    {
        dbid_t mBlank1ID;
        dbid_t mBlank2ID;

        {
            Part mBlank1(pcatMicro, Part::CreateBlankTag{});

            Part mBlank2(pcatMicro, Part::CreateBlankTag{});
            mBlank2.setData(pcatMicro->getProperty("model"), QString("Blank Test"));
            mBlank2.setData(pcatMicro->getProperty("peripherals"), QStringList{"Nur", "einige", "Testwerte"});

            PartList parts = {mBlank1, mBlank2};
            partFact.insertItems(parts.begin(), parts.end());

            mBlank1ID = mBlank1.getID();
            mBlank2ID = mBlank2.getID();

            ExpectValidIDs({mBlank1ID, mBlank2ID});
        }

        {
            PartList parts = {
                    partFact.findPartByID(pcatMicro, mBlank1ID),
                    partFact.findPartByID(pcatMicro, mBlank2ID)
            };

            Part& mBlank1 = parts[0];
            Part& mBlank2 = parts[1];

            EXPECT_FALSE(mBlank1.isDataLoaded(pcatMicro->getProperty("model")));
            EXPECT_FALSE(mBlank1.isDataLoaded(pcatMicro->getProperty("vmin")));
            EXPECT_FALSE(mBlank1.isDataLoaded(pcatMicro->getProperty("peripherals")));

            EXPECT_FALSE(mBlank2.isDataLoaded(pcatMicro->getProperty("model")));
            EXPECT_FALSE(mBlank2.isDataLoaded(pcatMicro->getProperty("vmin")));
            EXPECT_FALSE(mBlank2.isDataLoaded(pcatMicro->getProperty("peripherals")));

            partFact.loadItems(parts.begin(), parts.end());

            EXPECT_TRUE(mBlank1.isDataLoaded(pcatMicro->getProperty("model")));
            EXPECT_TRUE(mBlank1.isDataLoaded(pcatMicro->getProperty("vmin")));
            EXPECT_TRUE(mBlank1.isDataLoaded(pcatMicro->getProperty("peripherals")));

            EXPECT_TRUE(mBlank2.isDataLoaded(pcatMicro->getProperty("model")));
            EXPECT_TRUE(mBlank2.isDataLoaded(pcatMicro->getProperty("vmin")));
            EXPECT_TRUE(mBlank2.isDataLoaded(pcatMicro->getProperty("peripherals")));

            EXPECT_EQ(mBlank1.getData(pcatMicro->getProperty("model")).toString(), QString(""));
            EXPECT_DOUBLE_EQ(mBlank1.getData(pcatMicro->getProperty("vmin")).toDouble(), 0.0);
            EXPECT_EQ(mBlank1.getData(pcatMicro->getProperty("peripherals")).toList(), QList<QVariant>({}));

            EXPECT_EQ(mBlank2.getData(pcatMicro->getProperty("model")).toString(), QString("Blank Test"));
            EXPECT_DOUBLE_EQ(mBlank2.getData(pcatMicro->getProperty("vmin")).toDouble(), 0.0);
            EXPECT_EQ(mBlank2.getData(pcatMicro->getProperty("peripherals")).toList(),
                      QList<QVariant>({"Nur", "einige", "Testwerte"}));
        }
    }

    // Undo and redo everything a few times
    {
        for (int i = 0 ; i < 2 ; i++) {
            ASSERT_TRUE(editStack->undoUntil(testBeginEditCmd));
            ASSERT_TRUE(editStack->redoUntil(nullptr));
        }
    }
}

TEST_F(DynamicModelTest, PartContainerAssocs)
{
    setupBaseSchema("partcont", SchemaStatic | SchemaContainers | SchemaParts | SchemaPartContainerAssocs);

    System& sys = *System::getInstance();
    PartContainerFactory& contFact = PartContainerFactory::getInstance();
    PartFactory& partFact = PartFactory::getInstance();
    TestEventSniffer& evtSniffer = TestManager::getInstance().getEventSniffer();
    EditStack* editStack = sys.getEditStack();

    editStack->clear();

    DummyEditCommand* testBeginEditCmd = new DummyEditCommand;
    editStack->push(testBeginEditCmd);

    PartCategory* pcatRes = sys.getPartCategory("resistor");
    PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
    PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

    ASSERT_TRUE(pcatRes);
    ASSERT_TRUE(pcatMicro);
    ASSERT_TRUE(pcatDsheet);

    // Check that no assocs are loaded
    {
        for (dbid_t id : commonContIDs.values()) {
            PartContainer cont = contFact.findContainerByID(id);
            EXPECT_TRUE(cont);
            EXPECT_FALSE(cont.isPartAssocsLoaded());
            EXPECT_TRUE(cont.getPartAssocs().empty());
        }

        for (const QString& vid : commonPartIDs.keys()) {
            Part part;
            if (vid.startsWith("r")) {
                part = partFact.findPartByID(pcatRes, commonPartIDs[vid]);
            } else if (vid.startsWith("m")) {
                part = partFact.findPartByID(pcatMicro, commonPartIDs[vid]);
            } else if (vid.startsWith("d")) {
                part = partFact.findPartByID(pcatDsheet, commonPartIDs[vid]);
            }
            EXPECT_TRUE(part);
            EXPECT_FALSE(part.isContainerAssocsLoaded());
            EXPECT_TRUE(part.getContainerAssocs().empty());
        }
    }

    // Check that item cloning will clone assocs correctly
    {
        PartContainer a1 = contFact.findContainerByID(commonContIDs["a1"]);

        Part r1 = partFact.findPartByID(pcatRes, commonPartIDs["r1"]);
        Part r2 = partFact.findPartByID(pcatRes, commonPartIDs["r2"]);
        Part r3 = partFact.findPartByID(pcatRes, commonPartIDs["r3"]);

        contFact.loadItems(&a1, &a1+1, PartContainerFactory::LoadContainerParts);

        ASSERT_TRUE(a1.isPartAssocsLoaded());

        PartContainer a1Cloned = a1.clone();

        EXPECT_TRUE(a1Cloned);
        EXPECT_NE(a1, a1Cloned);
        EXPECT_FALSE(a1Cloned.hasID());
        EXPECT_TRUE(a1.isNameLoaded());
        EXPECT_TRUE(a1Cloned.isNameLoaded());
        EXPECT_EQ(a1.getName(), a1Cloned.getName());
        EXPECT_TRUE(a1.isPartAssocsLoaded());
        EXPECT_TRUE(a1Cloned.isPartAssocsLoaded());
        EXPECT_EQ(a1.getParts().size(), 3);
        EXPECT_EQ(a1Cloned.getParts().size(), 3);
        EXPECT_TRUE(a1.getParts().contains(r1));
        EXPECT_TRUE(a1.getParts().contains(r2));
        EXPECT_TRUE(a1.getParts().contains(r3));
        EXPECT_TRUE(a1Cloned.getParts().contains(r1));
        EXPECT_TRUE(a1Cloned.getParts().contains(r2));
        EXPECT_TRUE(a1Cloned.getParts().contains(r3));
        EXPECT_FALSE(a1.isDirty());
        EXPECT_TRUE(a1Cloned.isDirty());
        EXPECT_FALSE(a1.isAssocDirty<PartContainer::PartContainerAssocIdx>());
        EXPECT_TRUE(a1Cloned.isAssocDirty<PartContainer::PartContainerAssocIdx>());
    }

    // Load assocs from container side
    {
        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["a1"]),
                contFact.findContainerByID(commonContIDs["a2"]),
                contFact.findContainerByID(commonContIDs["b1"]),
                contFact.findContainerByID(commonContIDs["c1"])
        };

        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r4"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m4"])
        };

        Part& r1 = parts[0];
        Part& r2 = parts[1];
        Part& r3 = parts[2];
        Part& r4 = parts[3];
        Part& m3 = parts[4];
        Part& m4 = parts[5];

        contFact.loadItems(conts.begin(), conts.end());

        for (const auto& c : conts) {
            EXPECT_FALSE(c.isPartAssocsLoaded());
        }
        for (const auto& p : parts) {
            EXPECT_FALSE(p.isContainerAssocsLoaded());
        }

        contFact.loadItems(conts.begin(), conts.end(), PartContainerFactory::LoadContainerParts);

        for (const auto& c : conts) {
            EXPECT_TRUE(c.isPartAssocsLoaded());
            EXPECT_EQ(c.getParts().size(), c.getPartAssocs().size());
        }
        for (const auto& p : parts) {
            EXPECT_FALSE(p.isContainerAssocsLoaded());
        }

        auto ca1Parts = conts[0].getParts();
        auto ca2Parts = conts[1].getParts();
        auto cb1Parts = conts[2].getParts();
        auto cc1Parts = conts[3].getParts();

        EXPECT_EQ(ca1Parts.size(), 3);
        EXPECT_TRUE(ca1Parts.contains(r1));
        EXPECT_TRUE(ca1Parts.contains(r2));
        EXPECT_TRUE(ca1Parts.contains(r3));

        EXPECT_EQ(ca2Parts.size(), 1);
        EXPECT_TRUE(ca2Parts.contains(r4));

        EXPECT_EQ(cb1Parts.size(), 0);

        EXPECT_EQ(cc1Parts.size(), 2);
        EXPECT_TRUE(cc1Parts.contains(m3));
        EXPECT_TRUE(cc1Parts.contains(m4));
    }

    // Load assocs from part side
    {
        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["a1"]),
                contFact.findContainerByID(commonContIDs["a2"]),
                contFact.findContainerByID(commonContIDs["b1"]),
                contFact.findContainerByID(commonContIDs["c1"])
        };

        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r4"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m4"])
        };

        Part& r1 = parts[0];
        Part& r2 = parts[1];
        Part& r3 = parts[2];
        Part& r4 = parts[3];
        Part& m3 = parts[4];
        Part& m4 = parts[5];

        partFact.loadItems(parts.begin(), parts.end());

        for (const auto& p : parts) {
            EXPECT_FALSE(p.isContainerAssocsLoaded());
        }
        for (const auto& c : conts) {
            EXPECT_FALSE(c.isPartAssocsLoaded());
        }

        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadContainers);

        for (const auto& p : parts) {
            EXPECT_TRUE(p.isContainerAssocsLoaded());
            EXPECT_EQ(p.getContainers().size(), p.getContainerAssocs().size());
        }
        for (const auto& c : conts) {
            EXPECT_FALSE(c.isPartAssocsLoaded());
        }

        auto r1Conts = r1.getContainers();
        auto r2Conts = r2.getContainers();
        auto r3Conts = r3.getContainers();
        auto r4Conts = r4.getContainers();
        auto m3Conts = m3.getContainers();
        auto m4Conts = m4.getContainers();

        EXPECT_EQ(r1Conts.size(), 1);
        EXPECT_TRUE(r1Conts.contains(conts[0]));

        EXPECT_EQ(r2Conts.size(), 1);
        EXPECT_TRUE(r2Conts.contains(conts[0]));

        EXPECT_EQ(r3Conts.size(), 1);
        EXPECT_TRUE(r3Conts.contains(conts[0]));

        EXPECT_EQ(r4Conts.size(), 1);
        EXPECT_TRUE(r4Conts.contains(conts[1]));

        EXPECT_EQ(m3Conts.size(), 1);
        EXPECT_TRUE(m3Conts.contains(conts[3]));

        EXPECT_EQ(m4Conts.size(), 1);
        EXPECT_TRUE(m4Conts.contains(conts[3]));
    }

    // Load assocs from both sides, then change on one side and update to check if the other side reloads
    {
        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["a1"]),
                contFact.findContainerByID(commonContIDs["a2"]),
                contFact.findContainerByID(commonContIDs["b1"]),
                contFact.findContainerByID(commonContIDs["c1"])
        };
        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m4"])
        };

        PartContainer& ca1 = conts[0];
        PartContainer& ca2 = conts[1];
        PartContainer& cb1 = conts[2];
        PartContainer& cc1 = conts[3];

        Part& r1 = parts[0];
        Part& r2 = parts[1];
        Part& r3 = parts[2];
        Part& m3 = parts[3];
        Part& m4 = parts[4];

        contFact.loadItems(conts.begin(), conts.end(), PartContainerFactory::LoadContainerParts);
        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadContainers);

        EXPECT_TRUE(ca1.isPartAssocsLoaded());
        EXPECT_TRUE(ca2.isPartAssocsLoaded());
        EXPECT_TRUE(cb1.isPartAssocsLoaded());
        EXPECT_TRUE(r1.isContainerAssocsLoaded());
        EXPECT_TRUE(r2.isContainerAssocsLoaded());
        EXPECT_TRUE(r3.isContainerAssocsLoaded());
        EXPECT_TRUE(m3.isContainerAssocsLoaded());
        EXPECT_TRUE(m4.isContainerAssocsLoaded());

        ca1.setParts({r1, r3, m3});

        // This should trigger an auto-reload of r1, r3 and m3
        contFact.updateItem(ca1);

        ASSERT_EQ(ca1.getParts().size(), 3);
        EXPECT_TRUE(ca1.getParts().contains(r1));
        EXPECT_TRUE(ca1.getParts().contains(r3));
        EXPECT_TRUE(ca1.getParts().contains(m3));

        EXPECT_EQ(r1.getContainers().size(), 1);
        EXPECT_EQ(r1.getContainers()[0], ca1);

        EXPECT_EQ(r3.getContainers().size(), 1);
        EXPECT_EQ(r3.getContainers()[0], ca1);

        EXPECT_EQ(m3.getContainers().size(), 2);
        EXPECT_TRUE(m3.getContainers().contains(ca1));
        EXPECT_TRUE(m3.getContainers().contains(cc1));

        EXPECT_EQ(r2.getContainers().size(), 0);

        // Revert the changes from this test
        editStack->undo();
    }

    // Insert new part with assoc
    {
        Part r7(pcatRes, Part::CreateBlankTag{});
        r7.setData(pcatRes->getProperty("resistance"), 1e6);
        r7.setData(pcatRes->getProperty("type"), "Metal Film");
        r7.setData(pcatRes->getProperty("tolerance"), 0.15);
        r7.setData(pcatRes->getProperty("notes"), "Just temporarily");
        r7.setData(pcatRes->getProperty("numstock"), 4);

        PartContainer cb1 = contFact.findContainerByID(commonContIDs["b1"]);

        r7.loadContainers({cb1});

        EXPECT_EQ(r7.getContainers()[0], cb1);
        EXPECT_FALSE(cb1.isPartAssocsLoaded());

        contFact.loadItems(&cb1, &cb1+1, PartContainerFactory::LoadContainerParts);

        EXPECT_TRUE(cb1.isPartAssocsLoaded());
        EXPECT_EQ(cb1.getParts().size(), 0);

        // This should trigger an auto-reload of cb1
        partFact.insertItem(r7);

        EXPECT_TRUE(cb1.isPartAssocsLoaded());
        ASSERT_EQ(cb1.getParts().size(), 1);
        EXPECT_EQ(cb1.getParts()[0], r7);
    }

    // Insert new container with assoc
    {
        PartContainer ct1(PartContainer::CreateBlankTag{});
        ct1.setName("Container T1");

        Part r5 = partFact.findPartByID(pcatRes, commonPartIDs["r5"]);

        ct1.loadParts({r5});

        EXPECT_EQ(ct1.getParts()[0], r5);
        EXPECT_FALSE(r5.isContainerAssocsLoaded());

        partFact.loadItems(&r5, &r5+1, PartFactory::LoadContainers);

        EXPECT_TRUE(r5.isContainerAssocsLoaded());
        EXPECT_EQ(r5.getContainers().size(), 0);

        // This should trigger an auto-reload of r5
        contFact.insertItem(ct1);

        EXPECT_TRUE(r5.isContainerAssocsLoaded());
        ASSERT_EQ(r5.getContainers().size(), 1);
        EXPECT_EQ(r5.getContainers()[0], ct1);
    }

    // Update part with new assocs
    {
        Part m4 = partFact.findPartByID(pcatMicro, commonPartIDs["m4"]);

        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["a2"]),
                contFact.findContainerByID(commonContIDs["c1"])
        };

        PartContainer& ca2 = conts[0];
        PartContainer& cc1 = conts[1];

        EXPECT_FALSE(m4.isContainerAssocsLoaded());
        EXPECT_FALSE(ca2.isPartAssocsLoaded());
        EXPECT_FALSE(cc1.isPartAssocsLoaded());

        m4.loadContainers(&ca2, &ca2+1);

        EXPECT_EQ(m4.getContainers().size(), 1);
        EXPECT_EQ(m4.getContainers()[0], ca2);

        partFact.updateItem(m4);

        EXPECT_FALSE(ca2.isPartAssocsLoaded());
        EXPECT_FALSE(cc1.isPartAssocsLoaded());

        contFact.loadItems(conts.begin(), conts.end(), PartContainerFactory::LoadContainerParts);

        EXPECT_TRUE(ca2.isPartAssocsLoaded());
        EXPECT_TRUE(cc1.isPartAssocsLoaded());

        ASSERT_EQ(ca2.getParts().size(), 2);
        ASSERT_EQ(cc1.getParts().size(), 1);
        EXPECT_EQ(ca2.getParts()[0], partFact.findPartByID(pcatRes, commonPartIDs["r4"]));
        EXPECT_EQ(ca2.getParts()[1], m4);
        EXPECT_EQ(cc1.getParts()[0], partFact.findPartByID(pcatMicro, commonPartIDs["m3"]));
    }

    // Update container with new assocs
    {
        PartContainer ca1 = contFact.findContainerByID(commonContIDs["a1"]);

        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"])
        };

        Part& r1 = parts[0];
        Part& r2 = parts[1];
        Part& r3 = parts[2];
        Part& m3 = parts[3];

        EXPECT_FALSE(ca1.isPartAssocsLoaded());
        EXPECT_FALSE(r1.isContainerAssocsLoaded());
        EXPECT_FALSE(r2.isContainerAssocsLoaded());
        EXPECT_FALSE(r3.isContainerAssocsLoaded());
        EXPECT_FALSE(m3.isContainerAssocsLoaded());

        ca1.loadParts({r3, m3});

        EXPECT_EQ(ca1.getParts().size(), 2);
        EXPECT_EQ(ca1.getParts()[0], r3);
        EXPECT_EQ(ca1.getParts()[1], m3);

        contFact.updateItem(ca1);

        EXPECT_FALSE(r1.isContainerAssocsLoaded());
        EXPECT_FALSE(r2.isContainerAssocsLoaded());
        EXPECT_FALSE(r3.isContainerAssocsLoaded());
        EXPECT_FALSE(m3.isContainerAssocsLoaded());

        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadContainers);

        EXPECT_TRUE(r1.isContainerAssocsLoaded());
        EXPECT_TRUE(r2.isContainerAssocsLoaded());
        EXPECT_TRUE(r3.isContainerAssocsLoaded());
        EXPECT_TRUE(m3.isContainerAssocsLoaded());

        ASSERT_EQ(r1.getContainers().size(), 0);
        ASSERT_EQ(r2.getContainers().size(), 0);
        ASSERT_EQ(r3.getContainers().size(), 1);
        ASSERT_EQ(m3.getContainers().size(), 2);

        EXPECT_EQ(r3.getContainers()[0], ca1);
        EXPECT_EQ(m3.getContainers()[0], contFact.findContainerByID(commonContIDs["c1"]));
        EXPECT_EQ(m3.getContainers()[1], ca1);
    }

    // Delete container and check that assocs are deleted as well
    {
        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["a1"])
        };
        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"])
        };

        PartContainer& ca1 = conts[0];

        Part& r3 = parts[0];
        Part& m3 = parts[1];

        contFact.loadItems(conts.begin(), conts.end(), PartContainerFactory::LoadContainerParts);
        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadContainers);

        EXPECT_EQ(ca1.getParts().size(), 2);
        EXPECT_TRUE(ca1.getParts().contains(r3));
        EXPECT_TRUE(ca1.getParts().contains(m3));

        EXPECT_EQ(r3.getContainers().size(), 1);
        EXPECT_EQ(m3.getContainers().size(), 2);

        QSqlQuery query(db);

        dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM container_part WHERE cid=%1").arg(ca1.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_GT(query.value(0).toInt(), 0);

        // Should trigger auto-update of r3 and m3
        contFact.deleteItem(ca1);

        EXPECT_EQ(r3.getContainers().size(), 0);
        EXPECT_EQ(m3.getContainers().size(), 1);
        EXPECT_EQ(m3.getContainers()[0], contFact.findContainerByID(commonContIDs["c1"]));

        dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM container_part WHERE cid=%1").arg(ca1.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);
    }

    // Delete part and check that assocs are deleted as well
    {
        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r4"]),
        };
        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["a2"])
        };

        Part& r4 = parts[0];

        PartContainer& ca2 = conts[0];

        contFact.loadItems(conts.begin(), conts.end(), PartContainerFactory::LoadContainerParts);
        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadContainers);

        EXPECT_EQ(r4.getContainers().size(), 1);
        EXPECT_EQ(r4.getContainers()[0], ca2);

        EXPECT_EQ(ca2.getParts().size(), 2);
        EXPECT_TRUE(ca2.getParts().contains(r4));
        EXPECT_TRUE(ca2.getParts().contains(partFact.findPartByID(pcatMicro, commonPartIDs["m4"])));

        QSqlQuery query(db);

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM container_part WHERE ptype=%1 AND pid=%2"
        ).arg(dbw->escapeString(pcatRes->getID())).arg(r4.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_GT(query.value(0).toInt(), 0);

        // Should trigger auto-update of ca2
        partFact.deleteItem(r4);

        EXPECT_EQ(ca2.getParts().size(), 1);
        EXPECT_TRUE(ca2.getParts().contains(partFact.findPartByID(pcatMicro, commonPartIDs["m4"])));

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM container_part WHERE ptype=%1 AND pid=%2"
                ).arg(dbw->escapeString(pcatRes->getID())).arg(r4.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);
    }

    // Delete part and check that assocs are deleted as well, even if the part doesn't have assocs loaded
    {
        PartList parts = {
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
        };
        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["c1"])
        };

        Part& m3 = parts[0];

        PartContainer& cc1 = conts[0];

        contFact.loadItems(conts.begin(), conts.end(), PartContainerFactory::LoadContainerParts);

        EXPECT_EQ(cc1.getParts().size(), 1);
        EXPECT_TRUE(cc1.getParts().contains(m3));

        QSqlQuery query(db);

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM container_part WHERE ptype=%1 AND pid=%2"
        ).arg(dbw->escapeString(pcatMicro->getID())).arg(m3.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_GT(query.value(0).toInt(), 0);

        // Should trigger auto-update of cc1
        partFact.deleteItem(m3);

        EXPECT_EQ(cc1.getParts().size(), 0);

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM container_part WHERE ptype=%1 AND pid=%2"
        ).arg(dbw->escapeString(pcatMicro->getID())).arg(m3.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);
    }
}

TEST_F(DynamicModelTest, PartLinkAssocs)
{
    setupBaseSchema("partlink", SchemaStatic | SchemaContainers | SchemaParts | SchemaPartLinkAssocs);

    System& sys = *System::getInstance();
    PartContainerFactory& contFact = PartContainerFactory::getInstance();
    PartFactory& partFact = PartFactory::getInstance();
    TestEventSniffer& evtSniffer = TestManager::getInstance().getEventSniffer();
    EditStack* editStack = sys.getEditStack();

    editStack->clear();

    DummyEditCommand* testBeginEditCmd = new DummyEditCommand;
    editStack->push(testBeginEditCmd);

    PartCategory* pcatRes = sys.getPartCategory("resistor");
    PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
    PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

    PartLinkType* dsheetLink = sys.getPartLinkType("part_datasheet");
    PartLinkType* microResLink = sys.getPartLinkType("microcontroller_resistor");
    PartLinkType* mlessLink = sys.getPartLinkType("meaningless");

    ASSERT_TRUE(pcatRes);
    ASSERT_TRUE(pcatMicro);
    ASSERT_TRUE(pcatDsheet);

    ASSERT_TRUE(dsheetLink);
    ASSERT_TRUE(microResLink);
    ASSERT_TRUE(mlessLink);

    // Check static model
    {
        EXPECT_EQ(dsheetLink->getPatternType(), PartLinkType::PatternTypeBNegative);
        EXPECT_FALSE(dsheetLink->isPartCategoryA(pcatRes));
        EXPECT_FALSE(dsheetLink->isPartCategoryA(pcatMicro));
        EXPECT_TRUE(dsheetLink->isPartCategoryA(pcatDsheet));
        EXPECT_TRUE(dsheetLink->isPartCategoryB(pcatRes));
        EXPECT_TRUE(dsheetLink->isPartCategoryB(pcatMicro));
        EXPECT_FALSE(dsheetLink->isPartCategoryB(pcatDsheet));

        EXPECT_EQ(microResLink->getPatternType(), PartLinkType::PatternTypeBothPositive);
        EXPECT_FALSE(microResLink->isPartCategoryA(pcatRes));
        EXPECT_TRUE(microResLink->isPartCategoryA(pcatMicro));
        EXPECT_FALSE(microResLink->isPartCategoryA(pcatDsheet));
        EXPECT_TRUE(microResLink->isPartCategoryB(pcatRes));
        EXPECT_FALSE(microResLink->isPartCategoryB(pcatMicro));
        EXPECT_FALSE(microResLink->isPartCategoryB(pcatDsheet));

        EXPECT_EQ(dsheetLink->getFlags(), PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInB
                | PartLinkType::DisplaySelectionListA);
        EXPECT_EQ(dsheetLink->getSortIndexInCategory(pcatDsheet), 20000);
        EXPECT_EQ(dsheetLink->getSortIndexInCategory(pcatRes), 20100);
        EXPECT_EQ(dsheetLink->getSortIndexInCategory(pcatMicro), 20200);

        EXPECT_EQ(microResLink->getFlags(), PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInA
                | PartLinkType::EditInB | PartLinkType::DisplaySelectionListA | PartLinkType::DisplaySelectionListB);
        EXPECT_EQ(microResLink->getSortIndexInCategory(pcatMicro), 21000);
        EXPECT_EQ(microResLink->getSortIndexInCategory(pcatRes), 21100);
    }

    // Check that parts are associated to the correct side
    {
        Part r1 = partFact.findPartByID(pcatRes, commonPartIDs["r1"]);
        Part m1 = partFact.findPartByID(pcatMicro, commonPartIDs["m1"]);
        Part dr1a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr1a"]);

        EXPECT_TRUE(dsheetLink->isPartA(dr1a));
        EXPECT_FALSE(dsheetLink->isPartB(dr1a));
        EXPECT_FALSE(dsheetLink->isPartA(r1));
        EXPECT_TRUE(dsheetLink->isPartB(r1));
        EXPECT_FALSE(dsheetLink->isPartA(m1));
        EXPECT_TRUE(dsheetLink->isPartB(m1));

        EXPECT_TRUE(microResLink->isPartA(m1));
        EXPECT_FALSE(microResLink->isPartB(m1));
        EXPECT_FALSE(microResLink->isPartA(r1));
        EXPECT_TRUE(microResLink->isPartB(r1));
        EXPECT_FALSE(microResLink->isPartA(dr1a));
        EXPECT_FALSE(microResLink->isPartB(dr1a));
    }

    // Load assocs
    {
        // Load assocs from resistors
        {
            PartList parts = {
                    partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                    partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                    partFact.findPartByID(pcatRes, commonPartIDs["r3"])
            };

            Part& r1 = parts[0];
            Part& r2 = parts[1];
            Part& r3 = parts[2];

            Part m3 = partFact.findPartByID(pcatMicro, commonPartIDs["m3"]);
            Part m4 = partFact.findPartByID(pcatMicro, commonPartIDs["m4"]);

            Part dr1a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr1a"]);
            Part dr2a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr2a"]);
            Part dr3a = partFact.findPartByID(pcatDsheet, commonPartIDs["dr3a"]);

            EXPECT_FALSE(r1.isLinkedPartAssocsLoaded());
            EXPECT_FALSE(r2.isLinkedPartAssocsLoaded());
            EXPECT_FALSE(r3.isLinkedPartAssocsLoaded());

            partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadPartLinks);

            EXPECT_TRUE(r1.isLinkedPartAssocsLoaded());
            EXPECT_TRUE(r2.isLinkedPartAssocsLoaded());
            EXPECT_TRUE(r3.isLinkedPartAssocsLoaded());

            ASSERT_EQ(r1.getLinkedParts().size(), 2);
            EXPECT_TRUE(r1.getLinkedPartAssoc(dr1a));
            EXPECT_EQ(r1.getLinkedPartAssoc(dr1a).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(r1.getLinkedPartAssoc(m3).getLinkTypes(), QSet<PartLinkType*>({microResLink}));
            EXPECT_FALSE(r1.getLinkedPartAssoc(m4));

            ASSERT_EQ(r2.getLinkedParts().size(), 2);
            EXPECT_TRUE(r2.getLinkedPartAssoc(dr2a));
            EXPECT_TRUE(r2.getLinkedPartAssoc(m3));
            EXPECT_EQ(r2.getLinkedPartAssoc(dr2a).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(r2.getLinkedPartAssoc(m3).getLinkTypes(), QSet<PartLinkType*>({microResLink, mlessLink}));

            ASSERT_EQ(r3.getLinkedParts().size(), 2);
            EXPECT_TRUE(r3.getLinkedPartAssoc(dr3a));
            EXPECT_TRUE(r3.getLinkedPartAssoc(m4));
            EXPECT_EQ(r3.getLinkedPartAssoc(dr3a).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(r3.getLinkedPartAssoc(m4).getLinkTypes(), QSet<PartLinkType*>({microResLink}));
        }

        // Load assocs from micros
        {
            PartList parts = {
                    partFact.findPartByID(pcatMicro, commonPartIDs["m1"]),
                    partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
                    partFact.findPartByID(pcatMicro, commonPartIDs["m4"])
            };

            Part& m1 = parts[0];
            Part& m3 = parts[1];
            Part& m4 = parts[2];

            Part r1 = partFact.findPartByID(pcatRes, commonPartIDs["r1"]);
            Part r2 = partFact.findPartByID(pcatRes, commonPartIDs["r2"]);
            Part r3 = partFact.findPartByID(pcatRes, commonPartIDs["r3"]);

            Part dm1a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm1a"]);
            Part dm12b = partFact.findPartByID(pcatDsheet, commonPartIDs["dm12b"]);
            Part dm3a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm3a"]);
            Part dm3b = partFact.findPartByID(pcatDsheet, commonPartIDs["dm3b"]);
            Part dm4a = partFact.findPartByID(pcatDsheet, commonPartIDs["dm4a"]);
            Part dm4b = partFact.findPartByID(pcatDsheet, commonPartIDs["dm4b"]);
            Part dm4c = partFact.findPartByID(pcatDsheet, commonPartIDs["dm4c"]);

            EXPECT_FALSE(m1.isLinkedPartAssocsLoaded());
            EXPECT_FALSE(m3.isLinkedPartAssocsLoaded());
            EXPECT_FALSE(m4.isLinkedPartAssocsLoaded());

            partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadPartLinks);

            EXPECT_TRUE(m1.isLinkedPartAssocsLoaded());
            EXPECT_TRUE(m3.isLinkedPartAssocsLoaded());
            EXPECT_TRUE(m4.isLinkedPartAssocsLoaded());

            ASSERT_EQ(m1.getLinkedParts().size(), 2);
            EXPECT_TRUE(m1.getLinkedPartAssoc(dm1a));
            EXPECT_TRUE(m1.getLinkedPartAssoc(dm12b));
            EXPECT_EQ(m1.getLinkedPartAssoc(dm1a).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(m1.getLinkedPartAssoc(dm12b).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));

            ASSERT_EQ(m3.getLinkedParts().size(), 4);
            EXPECT_TRUE(m3.getLinkedPartAssoc(dm3a));
            EXPECT_TRUE(m3.getLinkedPartAssoc(dm3b));
            EXPECT_TRUE(m3.getLinkedPartAssoc(r1));
            EXPECT_TRUE(m3.getLinkedPartAssoc(r2));
            EXPECT_EQ(m3.getLinkedPartAssoc(dm3a).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(m3.getLinkedPartAssoc(dm3b).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(m3.getLinkedPartAssoc(r1).getLinkTypes(), QSet<PartLinkType*>({microResLink}));
            EXPECT_EQ(m3.getLinkedPartAssoc(r2).getLinkTypes(), QSet<PartLinkType*>({microResLink, mlessLink}));

            ASSERT_EQ(m4.getLinkedParts().size(), 4);
            EXPECT_TRUE(m4.getLinkedPartAssoc(dm4a));
            EXPECT_TRUE(m4.getLinkedPartAssoc(dm4b));
            EXPECT_TRUE(m4.getLinkedPartAssoc(dm4c));
            EXPECT_TRUE(m4.getLinkedPartAssoc(r3));
            EXPECT_EQ(m4.getLinkedPartAssoc(dm4a).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(m4.getLinkedPartAssoc(dm4b).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(m4.getLinkedPartAssoc(dm4c).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(m4.getLinkedPartAssoc(r3).getLinkTypes(), QSet<PartLinkType*>({microResLink}));
        }

        // Load assocs from datasheets
        {
            PartList parts = {
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dr1a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dr56a"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm12b"]),
                    partFact.findPartByID(pcatDsheet, commonPartIDs["dm3b"])
            };

            Part& dr1a = parts[0];
            Part& dr56a = parts[1];
            Part& dm12b = parts[2];
            Part& dm3b = parts[3];

            Part r1 = partFact.findPartByID(pcatRes, commonPartIDs["r1"]);
            Part r5 = partFact.findPartByID(pcatRes, commonPartIDs["r5"]);
            Part r6 = partFact.findPartByID(pcatRes, commonPartIDs["r6"]);

            Part m1 = partFact.findPartByID(pcatMicro, commonPartIDs["m1"]);
            Part m2 = partFact.findPartByID(pcatMicro, commonPartIDs["m2"]);
            Part m3 = partFact.findPartByID(pcatMicro, commonPartIDs["m3"]);

            EXPECT_FALSE(dr1a.isLinkedPartAssocsLoaded());
            EXPECT_FALSE(dr56a.isLinkedPartAssocsLoaded());
            EXPECT_FALSE(dm12b.isLinkedPartAssocsLoaded());
            EXPECT_FALSE(dm3b.isLinkedPartAssocsLoaded());

            partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadPartLinks);

            EXPECT_TRUE(dr1a.isLinkedPartAssocsLoaded());
            EXPECT_TRUE(dr56a.isLinkedPartAssocsLoaded());
            EXPECT_TRUE(dm12b.isLinkedPartAssocsLoaded());
            EXPECT_TRUE(dm3b.isLinkedPartAssocsLoaded());

            ASSERT_EQ(dr1a.getLinkedParts().size(), 1);
            EXPECT_TRUE(dr1a.getLinkedPartAssoc(r1));
            EXPECT_EQ(dr1a.getLinkedPartAssoc(r1).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));

            ASSERT_EQ(dr56a.getLinkedParts().size(), 2);
            EXPECT_TRUE(dr56a.getLinkedPartAssoc(r5));
            EXPECT_TRUE(dr56a.getLinkedPartAssoc(r6));
            EXPECT_EQ(dr56a.getLinkedPartAssoc(r5).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(dr56a.getLinkedPartAssoc(r6).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));

            ASSERT_EQ(dm12b.getLinkedParts().size(), 2);
            EXPECT_TRUE(dm12b.getLinkedPartAssoc(m1));
            EXPECT_TRUE(dm12b.getLinkedPartAssoc(m2));
            EXPECT_EQ(dm12b.getLinkedPartAssoc(m1).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
            EXPECT_EQ(dm12b.getLinkedPartAssoc(m2).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));

            ASSERT_EQ(dm3b.getLinkedParts().size(), 1);
            EXPECT_TRUE(dm3b.getLinkedPartAssoc(m3));
            EXPECT_EQ(dm3b.getLinkedPartAssoc(m3).getLinkTypes(), QSet<PartLinkType*>({dsheetLink}));
        }
    }

    // Insert new part with assoc
    {
        // Resistor -> Micro
        {
            Part r7(pcatRes, Part::CreateBlankTag{});
            r7.setData(pcatRes->getProperty("resistance"), 2e6);
            r7.setData(pcatRes->getProperty("type"), "Metal Film");
            r7.setData(pcatRes->getProperty("tolerance"), 0.15);
            r7.setData(pcatRes->getProperty("notes"), "Just temporarily 2");
            r7.setData(pcatRes->getProperty("numstock"), 3);

            Part m1 = partFact.findPartByID(pcatMicro, commonPartIDs["m1"]);

            PartLinkAssoc assoc = r7.loadLinkedParts({m1})[0];
            assoc.setLinkTypes({microResLink});

            EXPECT_EQ(r7.getLinkedParts()[0], m1);

            partFact.loadItems(&m1, &m1+1, PartFactory::LoadPartLinks);

            EXPECT_FALSE(m1.getLinkedParts().contains(r7));

            // Should trigger auto-reload of m1
            partFact.insertItem(r7);

            EXPECT_TRUE(m1.getLinkedParts().contains(r7));
            EXPECT_EQ(m1.getLinkedPartAssoc(r7).getLinkTypes(), QSet<PartLinkType*>({microResLink}));
        }

        // Micro -> Resistor
        {
            Part m5(pcatMicro, Part::CreateBlankTag{});
            m5.setData(pcatMicro->getProperty("model"), "ATtiny13A-SSU");
            m5.setData(pcatMicro->getProperty("freqmax"), 20e6);
            m5.setData(pcatMicro->getProperty("ramsize"), 64);
            m5.setData(pcatMicro->getProperty("vmin"), 1.8);
            m5.setData(pcatMicro->getProperty("peripherals"), QStringList{"GPIO", "I2C"});
            m5.setData(pcatMicro->getProperty("numstock"), 1);
            m5.setData(pcatMicro->getProperty("NULL"), 105);

            Part r4 = partFact.findPartByID(pcatRes, commonPartIDs["r4"]);

            PartLinkAssoc assoc = m5.loadLinkedParts({r4})[0];
            assoc.setLinkTypes({microResLink});

            EXPECT_EQ(m5.getLinkedParts()[0], r4);

            partFact.loadItems(&r4, &r4+1, PartFactory::LoadPartLinks);

            EXPECT_FALSE(r4.getLinkedParts().contains(m5));

            // Should trigger auto-reload of r4
            partFact.insertItem(m5);

            EXPECT_TRUE(r4.getLinkedParts().contains(m5));
            EXPECT_EQ(r4.getLinkedPartAssoc(m5).getLinkTypes(), QSet<PartLinkType*>({microResLink}));
        }
    }

    // Update part with new assocs
    {
        Part m4 = partFact.findPartByID(pcatMicro, commonPartIDs["m4"]);

        Part r3 = partFact.findPartByID(pcatRes, commonPartIDs["r3"]);

        partFact.loadItems(&m4, &m4+1, PartFactory::LoadPartLinks);

        EXPECT_FALSE(m4.isAssocDirty<Part::PartLinkAssocIdx>());
        EXPECT_FALSE(r3.isAssocDirty<Part::PartLinkAssocIdx>());

        m4.linkPart(r3, {mlessLink});

        EXPECT_TRUE(m4.isAssocDirty<Part::PartLinkAssocIdx>());
        EXPECT_EQ(m4.getLinkedPartAssoc(r3).getLinkTypes(), QSet<PartLinkType*>({microResLink, mlessLink}));

        partFact.updateItem(m4);

        EXPECT_FALSE(m4.isAssocDirty<Part::PartLinkAssocIdx>());
        EXPECT_FALSE(r3.isAssocDirty<Part::PartLinkAssocIdx>());
        EXPECT_EQ(m4.getLinkedPartAssoc(r3).getLinkTypes(), QSet<PartLinkType*>({microResLink, mlessLink}));

        m4.dropAssocs();

        EXPECT_FALSE(m4.isLinkedPartAssocsLoaded());

        partFact.loadItems(&m4, &m4+1, PartFactory::LoadPartLinks);

        EXPECT_FALSE(m4.isAssocDirty<Part::PartLinkAssocIdx>());
        EXPECT_FALSE(r3.isAssocDirty<Part::PartLinkAssocIdx>());
        EXPECT_EQ(m4.getLinkedPartAssoc(r3).getLinkTypes(), QSet<PartLinkType*>({microResLink, mlessLink}));
    }

    // Delete part and check that assocs are deleted as well
    {
        PartList parts = {
                partFact.findPartByID(pcatDsheet, commonPartIDs["dr56a"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r5"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r6"])
        };

        Part& dr56a = parts[0];

        Part& r5 = parts[1];
        Part& r6 = parts[2];

        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadPartLinks);

        EXPECT_TRUE(dr56a.getLinkedParts().contains(r5));
        EXPECT_TRUE(dr56a.getLinkedParts().contains(r6));
        EXPECT_TRUE(r5.getLinkedParts().contains(dr56a));
        EXPECT_TRUE(r6.getLinkedParts().contains(dr56a));

        QSqlQuery query(db);

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM partlink WHERE (ptype_a=%1 AND pid_a=%2) OR (ptype_b=%1 AND pid_b=%2)"
        ).arg(dbw->escapeString(pcatDsheet->getID())).arg(dr56a.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_GT(query.value(0).toInt(), 0);

        // Should trigger auto-update of r5 and r6
        partFact.deleteItem(dr56a);

        EXPECT_FALSE(r5.getLinkedParts().contains(dr56a));
        EXPECT_FALSE(r6.getLinkedParts().contains(dr56a));

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM partlink WHERE (ptype_a=%1 AND pid_a=%2) OR (ptype_b=%1 AND pid_b=%2)"
                ).arg(dbw->escapeString(pcatDsheet->getID())).arg(dr56a.getID()));
        ASSERT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);
    }
}

TEST_F(DynamicModelTest, Search)
{
    setupBaseSchema("search", SchemaStatic | SchemaParts);

    System& sys = *System::getInstance();
    PartFactory& partFact = PartFactory::getInstance();

    PartCategory* pcatRes = sys.getPartCategory("resistor");
    PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
    PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

    pcatRes->rebuildFullTextIndex();
    pcatMicro->rebuildFullTextIndex();
    pcatDsheet->rebuildFullTextIndex();

    ASSERT_TRUE(pcatRes);
    ASSERT_TRUE(pcatMicro);
    ASSERT_TRUE(pcatDsheet);

    // Do simple SQL search
    {
        {
            Filter f;
            f.setSQLWhereCode("WHERE vmin < 3.0");
            f.setSQLOrderCode("ORDER BY numstock DESC");

            PartList res = pcatMicro->find(&f, 0, 100);

            ASSERT_EQ(res.size(), 3);
            EXPECT_EQ(res[0], partFact.findPartByID(pcatMicro, commonPartIDs["m2"]));
            EXPECT_EQ(res[1], partFact.findPartByID(pcatMicro, commonPartIDs["m3"]));
            EXPECT_EQ(res[2], partFact.findPartByID(pcatMicro, commonPartIDs["m1"]));
        }

        {
            Filter f;
            f.setSQLWhereCode("WHERE model LIKE 'AT%'");

            PartList res = pcatMicro->find(&f, 0, 100);

            ASSERT_EQ(res.size(), 2);
            EXPECT_TRUE(res.contains(partFact.findPartByID(pcatMicro, commonPartIDs["m1"])));
            EXPECT_TRUE(res.contains(partFact.findPartByID(pcatMicro, commonPartIDs["m2"])));
        }
    }

    // Do SQL search with combined single-value and multi-value properties
    {
        Filter f;
        f.setSQLWhereCode("WHERE model NOT LIKE 'AT%' AND (vmin<3.0 OR peripherals='Bluetooth')");
        f.setSQLOrderCode("ORDER BY model ASC");

        PartList res = pcatMicro->find(&f, 0, 100);

        ASSERT_EQ(res.size(), 2);
        EXPECT_EQ(res[0], partFact.findPartByID(pcatMicro, commonPartIDs["m4"]));
        EXPECT_EQ(res[1], partFact.findPartByID(pcatMicro, commonPartIDs["m3"]));
    }

    // Do plain fulltext search
    {
        {
            Filter f;
            f.setFullTextQuery("leds");

            PartList res = pcatRes->find(&f, 0, 100);

            ASSERT_EQ(res.size(), 2);
            EXPECT_TRUE(res.contains(partFact.findPartByID(pcatRes, commonPartIDs["r1"])));
            EXPECT_TRUE(res.contains(partFact.findPartByID(pcatRes, commonPartIDs["r2"])));
        }

        {
            Filter f;
            f.setFullTextQuery("AT*");

            PartList res = pcatMicro->find(&f, 0, 100);

            ASSERT_EQ(res.size(), 2);
            EXPECT_TRUE(res.contains(partFact.findPartByID(pcatMicro, commonPartIDs["m1"])));
            EXPECT_TRUE(res.contains(partFact.findPartByID(pcatMicro, commonPartIDs["m2"])));
        }
    }
}

TEST_F(DynamicModelTest, Misc)
{
    setupBaseSchema("dynmisc", SchemaStatic | SchemaParts | SchemaContainers);

    System& sys = *System::getInstance();
    PartFactory& partFact = PartFactory::getInstance();

    PartCategory* pcatRes = sys.getPartCategory("resistor");
    PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
    PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

    ASSERT_TRUE(pcatRes);
    ASSERT_TRUE(pcatMicro);
    ASSERT_TRUE(pcatDsheet);

    // Test fetching distinct values
    {
        QList<QVariant> resistances = pcatRes->getProperty("resistance")->getDistinctValues();
        EXPECT_EQ(resistances, QList<QVariant>({220, 560, 1000, 4700, 10e6}));

        QList<QVariant> models = pcatMicro->getProperty("model")->getDistinctValues();
        EXPECT_EQ(models, QList<QVariant>({"ATmega16A-PU", "ATtiny25-20PU", "ESP32-WROOM-32", "STM32 F405VGT6"}));

        QList<QVariant> peripherals = pcatMicro->getProperty("peripherals")->getDistinctValues();
        EXPECT_EQ(peripherals, QList<QVariant>({
            "Bluetooth", "Ethernet PHY", "GPIO", "I2C", "PWM", "SDIO", "SPI", "WIFI"
        }));
    }
}

TEST_F(DynamicModelTest, ModifySchemaLinkType)
{
    setupBaseSchema("smod_ltype", SchemaAll);

    System& sys = *System::getInstance();
    PartFactory& partFact = PartFactory::getInstance();
    StaticModelManager& smMgr = *StaticModelManager::getInstance();

    PartCategory* pcatRes = sys.getPartCategory("resistor");
    PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
    PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

    PartLinkType* linkDsheet = sys.getPartLinkType("part_datasheet");
    PartLinkType* linkMicroRes = sys.getPartLinkType("microcontroller_resistor");
    PartLinkType* linkMless = sys.getPartLinkType("meaningless");

    ASSERT_TRUE(pcatRes);
    ASSERT_TRUE(pcatMicro);
    ASSERT_TRUE(pcatDsheet);

    ASSERT_TRUE(linkDsheet);
    ASSERT_TRUE(linkMicroRes);
    ASSERT_TRUE(linkMless);

    // Apply schema changes
    {
        QList<PartLinkType*> added;
        QList<PartLinkType*> removed;
        QMap<PartLinkType*, PartLinkType*> edited;

        // Add link types
        {
            PartLinkType* linkMfull = new PartLinkType("meaningful", {pcatMicro}, {pcatRes},
                                                       PartLinkType::PatternTypeBothPositive);
            linkMfull->setNameA("Resistors");
            linkMfull->setNameB("Microcontrollers");
            linkMfull->setFlags(PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInA
                    | PartLinkType::EditInB | PartLinkType::DisplaySelectionListA | PartLinkType::DisplaySelectionListB);
            linkMfull->setSortIndexInCategory(pcatMicro, 30000);
            linkMfull->setSortIndexInCategory(pcatRes, 30100);
            added << linkMfull;
        }

        // Remove link types
        {
            removed << linkMicroRes;
        }

        // Edit link types
        {
            PartLinkType* linkDsheetEdited = linkDsheet->clone();
            linkDsheetEdited->setNameA("Bauteile");
            linkDsheetEdited->setNameB("Datenblaetter");
            linkDsheetEdited->setSortIndexInCategory(pcatDsheet, 100);
            linkDsheetEdited->setSortIndexInCategory(pcatRes, 110);
            linkDsheetEdited->setSortIndexInCategory(pcatMicro, 120);
            edited[linkDsheet] = linkDsheetEdited;

            PartLinkType* linkMlessEdited = linkMless->clone();
            linkMlessEdited->setID("sinnlos");
            linkMlessEdited->setNameB("Widerstaende");
            edited[linkMless] = linkMlessEdited;
        }

        auto stmts = smMgr.generatePartLinkTypeDeltaCode(added, removed, edited);

        sys.unloadAppModel();

        smMgr.executeSchemaStatements(stmts);

        sys.loadAppModel();

        EXPECT_TRUE(sys.getPartLinkType("part_datasheet"));
        EXPECT_FALSE(sys.getPartLinkType("microcontroller_resistor"));
        EXPECT_FALSE(sys.getPartLinkType("meaningless"));
        EXPECT_TRUE(sys.getPartLinkType("sinnlos"));
        EXPECT_TRUE(sys.getPartLinkType("meaningful"));

        pcatRes = sys.getPartCategory("resistor");
        pcatMicro = sys.getPartCategory("microcontroller");
        pcatDsheet = sys.getPartCategory("datasheet");
    }

    // Check edited static model
    {
        PartLinkType* linkMfull = sys.getPartLinkType("meaningful");
        ASSERT_TRUE(linkMfull);
        EXPECT_EQ(linkMfull->getNameA(), QString("Resistors"));
        EXPECT_EQ(linkMfull->getNameB(), QString("Microcontrollers"));
        EXPECT_EQ(linkMfull->getFlags(), PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInA
                | PartLinkType::EditInB | PartLinkType::DisplaySelectionListA | PartLinkType::DisplaySelectionListB);
        EXPECT_EQ(linkMfull->getSortIndexInCategory(pcatMicro), 30000);
        EXPECT_EQ(linkMfull->getSortIndexInCategory(pcatRes), 30100);

        PartLinkType* linkDsheetEd = sys.getPartLinkType("part_datasheet");
        ASSERT_TRUE(linkDsheetEd);
        EXPECT_EQ(linkDsheetEd->getNameA(), QString("Bauteile"));
        EXPECT_EQ(linkDsheetEd->getNameB(), QString("Datenblaetter"));
        EXPECT_EQ(linkDsheetEd->getSortIndexInCategory(pcatDsheet), 100);
        EXPECT_EQ(linkDsheetEd->getSortIndexInCategory(pcatRes), 110);
        EXPECT_EQ(linkDsheetEd->getSortIndexInCategory(pcatMicro), 120);

        PartLinkType* linkMlessEd = sys.getPartLinkType("sinnlos");
        ASSERT_TRUE(linkMlessEd);
        EXPECT_EQ(linkMlessEd->getNameA(), QString("Microcontrollers"));
        EXPECT_EQ(linkMlessEd->getNameB(), QString("Widerstaende"));
        EXPECT_EQ(linkMlessEd->getFlags(), PartLinkType::ShowInA | PartLinkType::ShowInB | PartLinkType::EditInA
                | PartLinkType::EditInB | PartLinkType::DisplaySelectionListA | PartLinkType::DisplaySelectionListB);
        EXPECT_EQ(linkMlessEd->getSortIndexInCategory(pcatRes), 22000);
        EXPECT_EQ(linkMlessEd->getSortIndexInCategory(pcatMicro), 22100);
    }

    // Check that part links are preserved
    {
        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),

                partFact.findPartByID(pcatMicro, commonPartIDs["m1"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m2"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m4"]),

                partFact.findPartByID(pcatDsheet, commonPartIDs["dr1a"]),
                partFact.findPartByID(pcatDsheet, commonPartIDs["dr2a"]),
                partFact.findPartByID(pcatDsheet, commonPartIDs["dm12b"])
        };

        Part& r1 = parts[0];
        Part& r2 = parts[1];
        Part& r3 = parts[2];

        Part& m1 = parts[3];
        Part& m2 = parts[4];
        Part& m3 = parts[5];
        Part& m4 = parts[6];

        Part& dr1a = parts[7];
        Part& dr2a = parts[8];
        Part& dm12b = parts[9];

        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadPartLinks);

        EXPECT_FALSE(m3.getLinkedParts().contains(r1));
        EXPECT_TRUE(m3.getLinkedPartAssoc(r2));
        EXPECT_EQ(m3.getLinkedPartAssoc(r2).getLinkTypes().size(), 1);
        EXPECT_TRUE(m3.getLinkedPartAssoc(r2).getLinkTypes().contains(sys.getPartLinkType("sinnlos")));
        EXPECT_FALSE(m4.getLinkedParts().contains(r3));

        EXPECT_TRUE(dr1a.getLinkedParts().contains(r1));
        EXPECT_TRUE(dr2a.getLinkedParts().contains(r2));
        EXPECT_TRUE(dm12b.getLinkedParts().contains(m1));
        EXPECT_TRUE(dm12b.getLinkedParts().contains(m2));
    }
}

TEST_F(DynamicModelTest, ModifySchemaCategory)
{
    setupBaseSchema("smod_cat", SchemaAll);

    System& sys = *System::getInstance();
    PartContainerFactory& contFact = PartContainerFactory::getInstance();
    PartFactory& partFact = PartFactory::getInstance();
    StaticModelManager& smMgr = *StaticModelManager::getInstance();

    PartCategory* pcatRes = sys.getPartCategory("resistor");
    PartCategory* pcatMicro = sys.getPartCategory("microcontroller");
    PartCategory* pcatDsheet = sys.getPartCategory("datasheet");

    ASSERT_TRUE(pcatRes);
    ASSERT_TRUE(pcatMicro);
    ASSERT_TRUE(pcatDsheet);

    // Apply schema changes
    {
        QList<PartCategory*> added;
        QList<PartCategory*> removed;
        QMap<PartCategory*, PartCategory*> edited;
        QMap<PartProperty*, PartProperty*> editedProps;

        // Add categories
        {
            PartCategory* pcat = new PartCategory("transistor", "Transistors");
            pcat->setDescriptionPattern("Transistor $model ($type)");
            pcat->setSortIndex(100);

            PartProperty* modelProp = new PartProperty("model", PartProperty::String, pcat);
            modelProp->setStringMaximumLength(64);
            modelProp->setUserReadableName("Model");
            modelProp->setSortIndex(100);

            PartProperty* typeProp = new PartProperty("type", PartProperty::String, pcat);
            typeProp->setStringMaximumLength(64);
            typeProp->setUserReadableName("Type");
            typeProp->setSortIndex(200);

            added << pcat;
        }

        // Remove categories
        {
            removed << pcatDsheet;
        }

        // Edit categories (1)
        {
            PartCategory* pcatResEd = pcatRes->clone();
            pcatResEd->setDescriptionPattern("Widerstand $resistance ($type, $tolerance)");
            pcatResEd->setUserReadableName("Widerstand");
            pcatResEd->setSortIndex(1337);

            // Add properties
            {
                PartProperty* powerProp = new PartProperty("power", PartProperty::Decimal, pcatResEd);
                powerProp->setUserReadableName("Power Rating");
                powerProp->setDecimalMinimum(0.0);
                powerProp->setUnitSuffix("W");
            }

            // Remove properties
            {
                EXPECT_TRUE(pcatResEd->removeProperty(pcatResEd->getProperty("notes")));
            }

            // Edit properties
            {
                PartProperty* stockProp = pcatResEd->getProperty("numstock");
                stockProp->getMetaType()->parent = nullptr;
                stockProp->getMetaType()->coreType = PartProperty::Integer;
                stockProp->setIntegerMinimum(0);
                stockProp->setIntegerMaximum(9999);
                stockProp->setSortIndex(499);

                PartProperty* tolProp = pcatResEd->getProperty("tolerance");
                tolProp->setFieldName("rating");
                tolProp->setUserReadableName("Rating");
                tolProp->getMetaType()->coreType = PartProperty::String;
                editedProps[pcatRes->getProperty("tolerance")] = tolProp;
            }

            edited[pcatRes] = pcatResEd;
        }

        // Edit categories (2)
        {
            PartCategory* pcatMicroEd = pcatMicro->clone();
            pcatMicroEd->setID("uc");
            pcatMicroEd->setUserReadableName("Mikrocontroller");

            PartProperty* nullProp = pcatMicroEd->getProperty("NULL");
            PartProperty* kwProp = pcatMicroEd->getProperty("keywords");

            EXPECT_TRUE(nullProp);
            EXPECT_TRUE(kwProp);

            pcatMicroEd->removeProperty(nullProp);
            pcatMicroEd->removeProperty(kwProp);

            PartProperty* periphProp = pcatMicroEd->getProperty("peripherals");
            ASSERT_TRUE(periphProp);
            periphProp->setUserReadableName("Features");
            periphProp->setFieldName("features");
            editedProps[pcatMicro->getProperty("peripherals")] = periphProp;

            edited[pcatMicro] = pcatMicroEd;
        }

        auto stmts = smMgr.generateCategorySchemaDeltaCode(added, removed, edited, editedProps);

        sys.unloadAppModel();

        if (dbw->supportsTransactionalDDL()) {
            stmts << "THIS IS DEFINITELY NOT VALID SQL;";

            EXPECT_THROW(smMgr.executeSchemaStatements(stmts), SQLException);

            sys.loadAppModel();

            EXPECT_TRUE(sys.getPartCategory("datasheet"));
            EXPECT_TRUE(sys.getPartCategory("microcontroller"));
            EXPECT_FALSE(sys.getPartCategory("transistor"));

            stmts.pop_back();
        }

        smMgr.executeSchemaStatements(stmts);

        sys.loadAppModel();

        EXPECT_TRUE(sys.getPartCategory("resistor"));
        EXPECT_FALSE(sys.getPartCategory("microcontroller"));
        EXPECT_TRUE(sys.getPartCategory("uc"));
        EXPECT_FALSE(sys.getPartCategory("datasheet"));
        EXPECT_TRUE(sys.getPartCategory("transistor"));

        pcatRes = sys.getPartCategory("resistor");
        pcatMicro = sys.getPartCategory("uc");
        pcatDsheet = nullptr;
    }

    // Check edited static model
    {
        {
            PartCategory* pcatTrans = sys.getPartCategory("transistor");
            ASSERT_TRUE(pcatTrans);
            EXPECT_EQ(pcatTrans->getUserReadableName(), QString("Transistors"));
            EXPECT_EQ(pcatTrans->getDescriptionPattern(), QString("Transistor $model ($type)"));
            EXPECT_EQ(pcatTrans->getSortIndex(), 100);

            PartProperty* modelProp = pcatTrans->getProperty("model");
            ASSERT_TRUE(modelProp);
            EXPECT_EQ(modelProp->getType(), PartProperty::String);
            EXPECT_EQ(modelProp->getStringMaximumLength(), 64);
            EXPECT_EQ(modelProp->getUserReadableName(), QString("Model"));
            EXPECT_EQ(modelProp->getSortIndex(), 100);

            PartProperty* typeProp = pcatTrans->getProperty("type");
            ASSERT_TRUE(typeProp);
            EXPECT_EQ(typeProp->getUserReadableName(), QString("Type"));
            EXPECT_EQ(typeProp->getSortIndex(), 200);
        }

        {
            EXPECT_EQ(pcatRes->getUserReadableName(), QString("Widerstand"));
            EXPECT_EQ(pcatRes->getDescriptionPattern(), QString("Widerstand $resistance ($type, $tolerance)"));
            EXPECT_EQ(pcatRes->getSortIndex(), 1337);

            PartProperty* powerProp = pcatRes->getProperty("power");
            ASSERT_TRUE(powerProp);
            EXPECT_EQ(powerProp->getUserReadableName(), QString("Power Rating"));
            EXPECT_DOUBLE_EQ(powerProp->getDecimalMinimum(), 0.0);
            EXPECT_EQ(powerProp->getUnitSuffix(), QString("W"));

            EXPECT_FALSE(pcatRes->getProperty("notes"));

            PartProperty* stockProp = pcatRes->getProperty("numstock");
            ASSERT_TRUE(stockProp);
            EXPECT_FALSE(stockProp->getMetaType()->parent);
            EXPECT_EQ(stockProp->getMetaType()->coreType, PartProperty::Integer);
            EXPECT_EQ(stockProp->getIntegerMinimum(), 0);
            EXPECT_EQ(stockProp->getIntegerMaximum(), 9999);
            EXPECT_EQ(stockProp->getSortIndex(), 499);

            EXPECT_FALSE(pcatRes->getProperty("tolerance"));

            PartProperty* ratingProp = pcatRes->getProperty("rating");
            ASSERT_TRUE(ratingProp);
            EXPECT_EQ(ratingProp->getUserReadableName(), QString("Rating"));
            EXPECT_EQ(ratingProp->getType(), PartProperty::String);
        }

        {
            EXPECT_EQ(pcatMicro->getID(), QString("uc"));
            EXPECT_EQ(pcatMicro->getUserReadableName(), QString("Mikrocontroller"));

            EXPECT_FALSE(pcatMicro->getProperty("NULL"));
            EXPECT_FALSE(pcatMicro->getProperty("keywords"));
            EXPECT_FALSE(pcatMicro->getProperty("peripherals"));

            PartProperty* featProp = pcatMicro->getProperty("features");
            ASSERT_TRUE(featProp);
            EXPECT_EQ(featProp->getUserReadableName(), QString("Features"));
            EXPECT_TRUE((featProp->getFlags() & PartProperty::MultiValue) != 0);
        }
    }

    // Check that the dynamic model is preserved
    {
        PartCategory* pcatTrans = sys.getPartCategory("transistor");

        PartContainerList conts = {
                contFact.findContainerByID(commonContIDs["a1"]),
                contFact.findContainerByID(commonContIDs["a2"]),
                contFact.findContainerByID(commonContIDs["b1"]),
                contFact.findContainerByID(commonContIDs["c1"])
        };
        PartList parts = {
                partFact.findPartByID(pcatRes, commonPartIDs["r1"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r2"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r3"]),
                partFact.findPartByID(pcatRes, commonPartIDs["r4"]),

                partFact.findPartByID(pcatMicro, commonPartIDs["m1"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m2"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m3"]),
                partFact.findPartByID(pcatMicro, commonPartIDs["m4"])
        };

        PartContainer& ca1 = conts[0];
        PartContainer& ca2 = conts[1];
        PartContainer& cb1 = conts[2];
        PartContainer& cc1 = conts[3];

        Part& r1 = parts[0];
        Part& r2 = parts[1];
        Part& r3 = parts[2];
        Part& r4 = parts[3];

        Part& m1 = parts[4];
        Part& m2 = parts[5];
        Part& m3 = parts[6];
        Part& m4 = parts[7];

        contFact.loadItems(conts.begin(), conts.end(), PartContainerFactory::LoadAll);
        partFact.loadItems(parts.begin(), parts.end(), PartFactory::LoadAll);

        EXPECT_TRUE(r1.isDataFullyLoaded());
        EXPECT_TRUE(m1.isDataFullyLoaded());

        EXPECT_TRUE(r1.getData().contains(pcatRes->getProperty("power")));
        EXPECT_DOUBLE_EQ(r1.getData(pcatRes->getProperty("power")).toDouble(), 0.0);
        EXPECT_EQ(r1.getData(pcatRes->getProperty("resistance")).toInt(), 220);
        EXPECT_EQ(r1.getData(pcatRes->getProperty("type")).toString(), QString("Metal Film"));
        EXPECT_EQ(r1.getData(pcatRes->getProperty("numstock")).toInt(), 20);
        EXPECT_EQ(r1.getData(pcatRes->getProperty("rating")).toString(), QString("0.1"));

        EXPECT_EQ(r2.getData(pcatRes->getProperty("resistance")).toInt(), 560);
        EXPECT_EQ(r2.getData(pcatRes->getProperty("numstock")).toInt(), 15);
        EXPECT_EQ(r2.getData(pcatRes->getProperty("rating")).toString(), QString("0.1"));

        EXPECT_EQ(m1.getData(pcatMicro->getProperty("model")).toString(), QString("ATmega16A-PU"));
        EXPECT_EQ(m1.getData(pcatMicro->getProperty("freqmax")).toLongLong(), 16e6);
        EXPECT_EQ(m1.getData(pcatMicro->getProperty("features")).toList(), QList<QVariant>({"GPIO", "I2C", "SPI"}));
        EXPECT_EQ(m1.getData(pcatMicro->getProperty("numstock")).toInt(), 2);

        EXPECT_DOUBLE_EQ(m2.getData(pcatMicro->getProperty("vmin")).toDouble(), 2.7);
        EXPECT_EQ(m2.getData(pcatMicro->getProperty("features")).toList(), QList<QVariant>({"GPIO", "I2C"}));

        EXPECT_EQ(m3.getData(pcatMicro->getProperty("features")).toList(),
                  QList<QVariant>({"GPIO", "I2C", "SPI", "PWM", "SDIO"}));

        PartList transParts = partFact.loadItemsSingleCategory(pcatTrans, QString(), QString(), QString());
        EXPECT_EQ(transParts.size(), 0);

        EXPECT_EQ(ca1.getParts().size(), 3);
        EXPECT_EQ(ca2.getParts().size(), 1);
        EXPECT_EQ(cb1.getParts().size(), 0);
        EXPECT_EQ(cc1.getParts().size(), 2);

        EXPECT_TRUE(ca1.getParts().contains(r1));
        EXPECT_TRUE(ca1.getParts().contains(r2));
        EXPECT_TRUE(ca1.getParts().contains(r3));

        EXPECT_TRUE(ca2.getParts().contains(r4));

        EXPECT_TRUE(cc1.getParts().contains(m3));
        EXPECT_TRUE(cc1.getParts().contains(m4));

        EXPECT_TRUE(m3.getLinkedParts().contains(r1));
        EXPECT_TRUE(m3.getLinkedParts().contains(r2));
        EXPECT_TRUE(m4.getLinkedParts().contains(r3));
    }
}


}
