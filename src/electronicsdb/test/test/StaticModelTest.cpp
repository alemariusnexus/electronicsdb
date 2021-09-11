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
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../model/container/PartContainer.h"
#include "../../model/part/Part.h"
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


class StaticModelTest : public ::testing::Test
{
protected:
    StaticModelTest()
            : dbw(nullptr)
    {
    }

    void SetUp() override
    {
        System& sys = *System::getInstance();
        TestManager& testMgr = TestManager::getInstance();
    }

    void TearDown() override
    {
        delete dbw;
    }

    void useTestDatabase(const QString& suffix, bool forceNew = true)
    {
        db = QSqlDatabase();
        delete dbw;

        TestManager& testMgr = TestManager::getInstance();

        if (forceNew) {
            testMgr.openFreshTestDatabase(suffix);
        } else {
            testMgr.openTestDatabase(suffix);
        }

        db = QSqlDatabase::database();
        dbw = SQLDatabaseWrapperFactory::getInstance().create(db);
    }

protected:
    QSqlDatabase db;
    SQLDatabaseWrapper* dbw;
};


TEST_F(StaticModelTest, CreateLoadBasic)
{
    useTestDatabase("clschema");

    System* sys = System::getInstance();
    StaticModelManager* smMgr = StaticModelManager::getInstance();

    {
        PartCategory* c1 = new PartCategory("c1", "Test Category 1");
        c1->setSortIndex(1000);
        c1->setDescriptionPattern("Test $str $int");

        PartCategory* c2 = new PartCategory("c2", "Test Category 2");
        c2->setSortIndex(500);
        c2->setDescriptionPattern("Test $float");

        PartProperty* p1StrProp = new PartProperty("str", PartProperty::String);
        p1StrProp->setDefaultValue("Test Default");
        p1StrProp->setUserReadableName("Some String");
        c1->addProperty(p1StrProp);

        PartProperty* p1IntProp = new PartProperty("int", PartProperty::Integer);
        p1IntProp->setDefaultValue(-1337);
        p1IntProp->setUserReadableName("Some Int");
        c1->addProperty(p1IntProp);

        PartProperty* p2FloatProp = new PartProperty("float", PartProperty::Float);
        p2FloatProp->setDefaultValue(420.69);
        p2FloatProp->setUserReadableName("Some Float");
        c2->addProperty(p2FloatProp);

        smMgr->executeSchemaStatements(smMgr->generateCategorySchemaDeltaCode({c1, c2}, {}, {}, {}));

        delete c1;
        delete c2;
    }

    {
        sys->loadAppModel();

        EXPECT_EQ(sys->getPartCategories().size(), 2);

        PartCategory* c1 = sys->getPartCategory("c1");
        PartCategory* c2 = sys->getPartCategory("c2");

        ASSERT_TRUE(c1);
        ASSERT_TRUE(c2);

        EXPECT_EQ(sys->getPartCategories()[0], c2);
        EXPECT_EQ(sys->getPartCategories()[1], c1);

        EXPECT_EQ(c1->getSortIndex(), 1000);
        EXPECT_EQ(c2->getSortIndex(), 500);

        EXPECT_EQ(c1->getDescriptionPattern(), QString("Test $str $int"));
        EXPECT_EQ(c2->getDescriptionPattern(), QString("Test $float"));

        PartProperty* p1StrProp = c1->getProperty("str");
        PartProperty* p1IntProp = c1->getProperty("int");

        PartProperty* p2FloatProp = c2->getProperty("float");

        ASSERT_TRUE(p1StrProp);
        ASSERT_TRUE(p1IntProp);
        ASSERT_TRUE(p2FloatProp);

        EXPECT_EQ(p1StrProp->getType(), PartProperty::String);
        EXPECT_EQ(p1IntProp->getType(), PartProperty::Integer);
        EXPECT_EQ(p2FloatProp->getType(), PartProperty::Float);

        EXPECT_EQ(p1StrProp->getDefaultValueFunction(), PartProperty::DefaultValueConst);
        EXPECT_EQ(p1IntProp->getDefaultValueFunction(), PartProperty::DefaultValueConst);
        EXPECT_EQ(p2FloatProp->getDefaultValueFunction(), PartProperty::DefaultValueConst);

        EXPECT_EQ(p1StrProp->getDefaultValue().toString(), QString("Test Default"));
        EXPECT_EQ(p1IntProp->getDefaultValue().toInt(), -1337);
        EXPECT_DOUBLE_EQ(p2FloatProp->getDefaultValue().toDouble(), 420.69);

        EXPECT_EQ(p1StrProp->getUserReadableName(), QString("Some String"));
        EXPECT_EQ(p1IntProp->getUserReadableName(), QString("Some Int"));
        EXPECT_EQ(p2FloatProp->getUserReadableName(), QString("Some Float"));
    }
}

TEST_F(StaticModelTest, PropTypes)
{
    useTestDatabase("proptypes");

    System* sys = System::getInstance();
    StaticModelManager* smMgr = StaticModelManager::getInstance();

    {
        PartCategory* c = new PartCategory("c", "Test Category");

        PartProperty* pStr = new PartProperty("prop_str", PartProperty::String);
        pStr->setUserReadableName("Prop Str");
        pStr->setFullTextSearchUserPrefix("pfxStr");
        pStr->setSQLNaturalOrderCode("ORDER BY prop_str DESC");
        pStr->setSQLAscendingOrderCode("ORDER BY prop_str DESC");
        pStr->setSQLDescendingOrderCode("ORDER BY prop_str ASC");
        pStr->setStringMaximumLength(64);
        pStr->setSortIndex(1337);
        pStr->setTextAreaMinimumHeight(69);
        pStr->getMetaType()->flags = PartProperty::FullTextIndex | PartProperty::DisplayDynamicEnum;
        pStr->getMetaType()->flagsWithNonNullValue = PartProperty::FullTextIndex | PartProperty::DisplayDynamicEnum;
        c->addProperty(pStr);

        PartProperty* pInt = new PartProperty("prop_int", PartProperty::Integer);
        pInt->setUserReadableName("Prop Int");
        pInt->setFullTextSearchUserPrefix("pfxInt");
        pInt->setIntegerMinimum(-69);
        pInt->setIntegerMaximum(1337);
        c->addProperty(pInt);

        PartProperty* pIntBig = new PartProperty("prop_intbig", PartProperty::Integer);
        pIntBig->setUserReadableName("Prop IntBig");
        pIntBig->setIntegerMinimum(std::numeric_limits<int64_t>::min()+2);
        pIntBig->setIntegerMaximum(std::numeric_limits<int64_t>::max()-4);
        c->addProperty(pIntBig);

        PartProperty* pFloat = new PartProperty("prop_float", PartProperty::Float);
        pFloat->setUserReadableName("Prop Float");
        pFloat->setDecimalMinimum(-4.2e-100);
        pFloat->setDecimalMaximum(42e133);
        c->addProperty(pFloat);

        PartProperty* pDecimal = new PartProperty("prop_decimal", PartProperty::Decimal);
        pDecimal->setUserReadableName("Prop Decimal");
        pDecimal->setDecimalMinimum(-1234.56789);
        pDecimal->setDecimalMaximum(987.654321);
        c->addProperty(pDecimal);

        PartProperty* pBool = new PartProperty("prop_bool", PartProperty::Boolean);
        pBool->setUserReadableName("Prop Bool");
        pBool->setBooleanTrueDisplayText("hell yeah brotha");
        pBool->setBooleanFalseDisplayText("nah man");
        c->addProperty(pBool);

        PartProperty* pFile = new PartProperty("prop_file", PartProperty::File);
        pFile->setUserReadableName("Prop File");
        c->addProperty(pFile);

        PartProperty* pDate = new PartProperty("prop_date", PartProperty::Date);
        pDate->setUserReadableName("Prop Date");
        c->addProperty(pDate);

        PartProperty* pTime = new PartProperty("prop_time", PartProperty::Time);
        pTime->setUserReadableName("Prop Time");
        c->addProperty(pTime);

        PartProperty* pDateTime = new PartProperty("prop_datetime", PartProperty::DateTime);
        pDateTime->setUserReadableName("Prop DateTime");
        c->addProperty(pDateTime);

        smMgr->executeSchemaStatements(smMgr->generateCategorySchemaDeltaCode({c}, {}, {}, {}));

        delete c;
    }

    {
        sys->loadAppModel();

        ASSERT_EQ(sys->getPartCategories().size(), 1);

        PartCategory* c = sys->getPartCategory("c");
        ASSERT_TRUE(c);

        ASSERT_EQ(c->getProperties().size(), 10);

        {
            PartProperty* pStr = c->getProperty("prop_str");
            ASSERT_TRUE(pStr);
            EXPECT_EQ(pStr->getType(), PartProperty::String);
            EXPECT_EQ(pStr->getUserReadableName(), QString("Prop Str"));
            EXPECT_EQ(pStr->getFullTextSearchUserPrefix(), QString("pfxStr"));
            EXPECT_EQ(pStr->getSQLNaturalOrderCode(), "ORDER BY prop_str DESC");
            EXPECT_EQ(pStr->getSQLAscendingOrderCode(), "ORDER BY prop_str DESC");
            EXPECT_EQ(pStr->getSQLDescendingOrderCode(), "ORDER BY prop_str ASC");
            EXPECT_EQ(pStr->getStringMaximumLength(), 64);
            EXPECT_EQ(pStr->getTextAreaMinimumHeight(), 69);
            EXPECT_EQ(pStr->getSortIndex(), 1337);
            EXPECT_TRUE((pStr->getFlags() & PartProperty::FullTextIndex) != 0);
            EXPECT_TRUE((pStr->getFlags() & PartProperty::DisplayDynamicEnum) != 0);
            EXPECT_TRUE((pStr->getFlags() & PartProperty::MultiValue) == 0);
        }

        {
            PartProperty* pInt = c->getProperty("prop_int");
            ASSERT_TRUE(pInt);
            EXPECT_EQ(pInt->getType(), PartProperty::Integer);
            EXPECT_EQ(pInt->getUserReadableName(), QString("Prop Int"));
            EXPECT_EQ(pInt->getFullTextSearchUserPrefix(), QString("pfxInt"));
            EXPECT_EQ(pInt->getIntegerMinimum(), -69);
            EXPECT_EQ(pInt->getIntegerMaximum(), 1337);
        }

        {
            PartProperty* pIntBig = c->getProperty("prop_intbig");
            ASSERT_TRUE(pIntBig);
            EXPECT_EQ(pIntBig->getType(), PartProperty::Integer);
            EXPECT_EQ(pIntBig->getUserReadableName(), QString("Prop IntBig"));
            EXPECT_EQ(pIntBig->getIntegerMinimum(), std::numeric_limits<int64_t>::min()+2);
            EXPECT_EQ(pIntBig->getIntegerMaximum(), std::numeric_limits<int64_t>::max()-4);
        }

        {
            PartProperty* pFloat = c->getProperty("prop_float");
            ASSERT_TRUE(pFloat);
            EXPECT_EQ(pFloat->getType(), PartProperty::Float);
            EXPECT_EQ(pFloat->getUserReadableName(), QString("Prop Float"));
            EXPECT_DOUBLE_EQ(pFloat->getFloatMinimum(), -4.2e-100);
            EXPECT_DOUBLE_EQ(pFloat->getFloatMaximum(), 42e133);
        }

        {
            PartProperty* pDecimal = c->getProperty("prop_decimal");
            ASSERT_TRUE(pDecimal);
            EXPECT_EQ(pDecimal->getType(), PartProperty::Decimal);
            EXPECT_EQ(pDecimal->getUserReadableName(), QString("Prop Decimal"));
            EXPECT_DOUBLE_EQ(pDecimal->getDecimalMinimum(), -1234.56789);
            EXPECT_DOUBLE_EQ(pDecimal->getDecimalMaximum(), 987.654321);
        }

        {
            PartProperty* pBool = c->getProperty("prop_bool");
            ASSERT_TRUE(pBool);
            EXPECT_EQ(pBool->getType(), PartProperty::Boolean);
            EXPECT_EQ(pBool->getUserReadableName(), QString("Prop Bool"));
            EXPECT_EQ(pBool->getBooleanTrueDisplayText(), QString("hell yeah brotha"));
            EXPECT_EQ(pBool->getBooleanFalseDisplayText(), QString("nah man"));
        }

        {
            {
                PartProperty* pFile = c->getProperty("prop_file");
                ASSERT_TRUE(pFile);
                EXPECT_EQ(pFile->getType(), PartProperty::File);
                EXPECT_EQ(pFile->getUserReadableName(), QString("Prop File"));
            }
        }

        {
            PartProperty* pDate = c->getProperty("prop_date");
            ASSERT_TRUE(pDate);
            EXPECT_EQ(pDate->getType(), PartProperty::Date);
            EXPECT_EQ(pDate->getUserReadableName(), QString("Prop Date"));
            EXPECT_EQ(pDate->getDefaultValueFunction(), PartProperty::DefaultValueNow);
            EXPECT_LE(std::abs(pDate->getDefaultValue().toDate().daysTo(QDate::currentDate())), 1);
        }

        {
            PartProperty* pTime = c->getProperty("prop_time");
            ASSERT_TRUE(pTime);
            EXPECT_EQ(pTime->getType(), PartProperty::Time);
            EXPECT_EQ(pTime->getUserReadableName(), QString("Prop Time"));
            EXPECT_EQ(pTime->getDefaultValueFunction(), PartProperty::DefaultValueNow);

            // NOTE: This can theoretically fail very close to midnight, but it's such a corner case that I don't care.
            EXPECT_LE(std::abs(pTime->getDefaultValue().toTime().msecsTo(QTime::currentTime())), 10000);
        }

        {
            PartProperty* pDateTime = c->getProperty("prop_datetime");
            ASSERT_TRUE(pDateTime);
            EXPECT_EQ(pDateTime->getType(), PartProperty::DateTime);
            EXPECT_EQ(pDateTime->getUserReadableName(), QString("Prop DateTime"));
            EXPECT_EQ(pDateTime->getDefaultValueFunction(), PartProperty::DefaultValueNow);
            EXPECT_LE(std::abs(pDateTime->getDefaultValue().toDateTime().msecsTo(QDateTime::currentDateTime())), 10000);
        }
    }
}

TEST_F(StaticModelTest, PropFormat)
{
    {
        PartProperty* pStr = new PartProperty("prop_str", PartProperty::String);

        EXPECT_EQ(pStr->formatValueForDisplay(QString("Hello World")), QString("Hello World"));
        EXPECT_EQ(pStr->formatValueForDisplay(QString("123")), QString("123"));

        delete pStr;
    }

    {
        PartProperty* pInt = new PartProperty("prop_int", PartProperty::Integer);

        EXPECT_EQ(pInt->formatValueForDisplay(1234), QString("1.234k"));
        EXPECT_EQ(pInt->formatValueForDisplay(14500000), QString("14.5M"));
        EXPECT_EQ(pInt->formatValueForDisplay(-58110), QString("-58.11k"));
        EXPECT_EQ(pInt->formatValueForDisplay(0), QString("0"));

        delete pInt;
    }

    {
        PartProperty* pInt = new PartProperty("prop_int", PartProperty::Integer);
        pInt->getMetaType()->flagsWithNonNullValue = PartProperty::DisplayWithUnits;
        pInt->getMetaType()->flags = 0;

        EXPECT_EQ(pInt->formatValueForDisplay(1234), QString("1234"));
        EXPECT_EQ(pInt->formatValueForDisplay(14500000), QString("14500000"));
        EXPECT_EQ(pInt->formatValueForDisplay(-58110), QString("-58110"));
        EXPECT_EQ(pInt->formatValueForDisplay(0), QString("0"));

        delete pInt;
    }

    {
        PartProperty* pFloat = new PartProperty("prop_float", PartProperty::Float);
        pFloat->getMetaType()->flagsWithNonNullValue = PartProperty::DisplayWithUnits;
        pFloat->getMetaType()->flags = 0;

        EXPECT_TRUE(pFloat->formatValueForDisplay(123.456).startsWith("123.456"));
        EXPECT_TRUE(pFloat->formatValueForDisplay(-0.57).startsWith("-0.57"));

        delete pFloat;
    }

    {
        PartCategory* pcat = new PartCategory("desc1", "Descriptions 1");
        pcat->setDescriptionPattern("$prop1, $prop2invalid, valid$prop2, ${prop3}");

        PartProperty* prop1 = new PartProperty("prop1", PartProperty::String, pcat);

        PartProperty* prop2 = new PartProperty("prop2", PartProperty::Integer, pcat);
        prop2->getMetaType()->flags = PartProperty::DisplayWithUnits;
        prop2->getMetaType()->flagsWithNonNullValue = PartProperty::DisplayWithUnits;
        prop2->setUnitSuffix("Hz");

        PartProperty* prop3 = new PartProperty("prop3", PartProperty::String, pcat);
        prop3->getMetaType()->flags = PartProperty::MultiValue;
        prop3->getMetaType()->flagsWithNonNullValue = PartProperty::MultiValue;

        Part p1(pcat, Part::CreateBlankTag{});
        p1.setData(prop1, "Hello World");
        p1.setData(prop2, 4700);
        p1.setData(prop3, QList<QVariant>{"Just", "a", "test"});

        EXPECT_EQ(p1.getDescription(), QString("Hello World, $prop2invalid, valid4.7kHz, Just, a, test"));

        EXPECT_EQ(prop3->formatSingleValueForDisplay("just one string"), QString("just one string"));
    }
}

// TODO: Test part links and multi-value properties
// TODO: Test the following:
//      * Part link types
//      * Multi-value properties
//      * Meta types


}
