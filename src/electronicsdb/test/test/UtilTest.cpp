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

#include <QList>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <gtest/gtest.h>
#include <nxcommon/log.h>
#include "../../command/DummyEditCommand.h"
#include "../../command/EditStack.h"
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/InvalidCredentialsException.h"
#include "../../util/elutil.h"
#include "../../util/Filter.h"
#include "../../util/KeyVault.h"
#include "../../System.h"
#include "../TestManager.h"
#include "../testutil.h"

namespace electronicsdb
{


class UtilTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        System& sys = *System::getInstance();
        TestManager& testMgr = TestManager::getInstance();

        testMgr.openTestDatabase("misc");

        db = QSqlDatabase::database();
        dbw = SQLDatabaseWrapperFactory::getInstance().create(db);
    }

    void TearDown() override
    {
        delete dbw;
    }

protected:
    QSqlDatabase db;
    SQLDatabaseWrapper* dbw;
};



TEST_F(UtilTest, EditStack)
{
    // NOTE: There are related and more practical tests in CommandTest.EditStack.

    EditStack* editStack = System::getInstance()->getEditStack();

    editStack->clear();

    ASSERT_EQ(editStack->getUndoStackUsedSize(), 0);
    ASSERT_EQ(editStack->getRedoStackUsedSize(), 0);

    editStack->setUndoStackSize(3);
    editStack->setRedoStackSize(3);

    EXPECT_FALSE(editStack->canUndo());
    EXPECT_FALSE(editStack->canRedo());

    auto createDummyCommand = [](EditCommand::Event* evtVar) {
        DummyEditCommand* cmd = new DummyEditCommand();
        cmd->setListener([=](EditCommand::Event evt) {
            if (evtVar) {
                *evtVar = evt;
            }
        });
        return cmd;
    };

    EditCommand::Event c1Evt = EditCommand::EventInvalid;
    EditCommand::Event c2Evt = EditCommand::EventInvalid;
    EditCommand::Event c3Evt = EditCommand::EventInvalid;
    EditCommand::Event c4Evt = EditCommand::EventInvalid;

    auto c1 = createDummyCommand(&c1Evt);
    auto c2 = createDummyCommand(&c2Evt);
    auto c3 = createDummyCommand(&c3Evt);
    auto c4 = createDummyCommand(&c4Evt);

    editStack->push(c1);
    editStack->push(c2);
    editStack->push(c3);

    EXPECT_TRUE(editStack->canUndo());
    EXPECT_FALSE(editStack->canRedo());
    EXPECT_EQ(c1Evt, EditCommand::EventCommit);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
    EXPECT_EQ(c3Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 3);

    editStack->undo();
    EXPECT_TRUE(editStack->canUndo());
    EXPECT_TRUE(editStack->canRedo());
    EXPECT_EQ(c3Evt, EditCommand::EventRevert);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 2);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 1);

    editStack->undo();
    EXPECT_TRUE(editStack->canUndo());
    EXPECT_TRUE(editStack->canRedo());
    EXPECT_EQ(c3Evt, EditCommand::EventRevert);
    EXPECT_EQ(c2Evt, EditCommand::EventRevert);
    EXPECT_EQ(c1Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 1);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 2);

    editStack->undo();
    EXPECT_FALSE(editStack->canUndo());
    EXPECT_TRUE(editStack->canRedo());
    EXPECT_EQ(c3Evt, EditCommand::EventRevert);
    EXPECT_EQ(c2Evt, EditCommand::EventRevert);
    EXPECT_EQ(c1Evt, EditCommand::EventRevert);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 0);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 3);

    editStack->redo();
    EXPECT_TRUE(editStack->canUndo());
    EXPECT_TRUE(editStack->canRedo());
    EXPECT_EQ(c3Evt, EditCommand::EventRevert);
    EXPECT_EQ(c2Evt, EditCommand::EventRevert);
    EXPECT_EQ(c1Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 1);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 2);

    editStack->redo();
    EXPECT_TRUE(editStack->canUndo());
    EXPECT_TRUE(editStack->canRedo());
    EXPECT_EQ(c3Evt, EditCommand::EventRevert);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
    EXPECT_EQ(c1Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 2);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 1);

    editStack->redo();
    EXPECT_TRUE(editStack->canUndo());
    EXPECT_FALSE(editStack->canRedo());
    EXPECT_EQ(c3Evt, EditCommand::EventCommit);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
    EXPECT_EQ(c1Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 3);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 0);

    editStack->push(c4);
    EXPECT_TRUE(editStack->canUndo());
    EXPECT_FALSE(editStack->canRedo());
    EXPECT_EQ(c4Evt, EditCommand::EventCommit);
    EXPECT_EQ(c3Evt, EditCommand::EventCommit);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 3);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 0);

    editStack->setUndoStackSize(2);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 2);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 0);

    editStack->undo();
    editStack->undo();
    EXPECT_FALSE(editStack->canUndo());
    EXPECT_EQ(c4Evt, EditCommand::EventRevert);
    EXPECT_EQ(c3Evt, EditCommand::EventRevert);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 0);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 2);

    editStack->setRedoStackSize(1);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 0);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 1);

    editStack->redo();
    EXPECT_TRUE(editStack->canUndo());
    EXPECT_FALSE(editStack->canRedo());
    EXPECT_EQ(c4Evt, EditCommand::EventRevert);
    EXPECT_EQ(c3Evt, EditCommand::EventCommit);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 1);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 0);

    editStack->clear();
    EXPECT_EQ(editStack->getUndoStackUsedSize(), 0);
    EXPECT_EQ(editStack->getRedoStackUsedSize(), 0);
    EXPECT_EQ(c4Evt, EditCommand::EventRevert);
    EXPECT_EQ(c3Evt, EditCommand::EventCommit);
    EXPECT_EQ(c2Evt, EditCommand::EventCommit);
}

TEST_F(UtilTest, ParseVersionNumber)
{
    int major, minor, patch;
    auto reset = [&]() {
        major = -10;
        minor = -10;
        patch = -10;
    };

    {
        reset();
        EXPECT_TRUE(ParseVersionNumber("3.15.7", &major, &minor, &patch));
        EXPECT_EQ(major, 3);
        EXPECT_EQ(minor, 15);
        EXPECT_EQ(patch, 7);

        reset();
        EXPECT_TRUE(ParseVersionNumber("17.3", &major, &minor, &patch));
        EXPECT_EQ(major, 17);
        EXPECT_EQ(minor, 3);
        EXPECT_EQ(patch, 0);

        reset();
        EXPECT_TRUE(ParseVersionNumber("2.5.3", &major, &minor, nullptr));
        EXPECT_EQ(major, 2);
        EXPECT_EQ(minor, 5);
        EXPECT_EQ(patch, -10);

        reset();
        EXPECT_TRUE(ParseVersionNumber("10.5.3-some-suffix", &major, &minor, &patch));
        EXPECT_EQ(major, 10);
        EXPECT_EQ(minor, 5);
        EXPECT_EQ(patch, 3);
    }
}

TEST_F(UtilTest, FormatSuffices)
{
    {
        EXPECT_EQ(FormatIntSuffices(0, SuffixFormatSIBase10), QString("0"));
        EXPECT_EQ(FormatIntSuffices(5, SuffixFormatSIBase10), QString("5"));
        EXPECT_EQ(FormatIntSuffices(123, SuffixFormatSIBase10), QString("123"));
        EXPECT_EQ(FormatIntSuffices(500, SuffixFormatSIBase10), QString("500"));
        EXPECT_EQ(FormatIntSuffices(12345, SuffixFormatSIBase10), QString("12.345k"));
        EXPECT_EQ(FormatIntSuffices(12870, SuffixFormatSIBase10), QString("12.87k"));
        EXPECT_EQ(FormatIntSuffices(128000000, SuffixFormatSIBase10), QString("128M"));
        EXPECT_EQ(FormatIntSuffices(17000000000000ll, SuffixFormatSIBase10), QString("17T"));
        EXPECT_EQ(FormatIntSuffices(17850000000000ll, SuffixFormatSIBase10), QString("17.85T"));
        EXPECT_EQ(FormatIntSuffices(17851000000000ll, SuffixFormatSIBase10), QString("17.851T"));
        EXPECT_EQ(FormatIntSuffices(17851290000000ll, SuffixFormatSIBase10), QString("17.851T"));
        EXPECT_EQ(FormatIntSuffices(17851690000000ll, SuffixFormatSIBase10), QString("17.852T"));
        EXPECT_EQ(FormatIntSuffices(17851690000000000ll, SuffixFormatSIBase10), QString("17.852P"));
        EXPECT_EQ(FormatIntSuffices(-0, SuffixFormatSIBase10), QString("0"));
        EXPECT_EQ(FormatIntSuffices(-5, SuffixFormatSIBase10), QString("-5"));
        EXPECT_EQ(FormatIntSuffices(-123, SuffixFormatSIBase10), QString("-123"));
        EXPECT_EQ(FormatIntSuffices(-500, SuffixFormatSIBase10), QString("-500"));
        EXPECT_EQ(FormatIntSuffices(-12345, SuffixFormatSIBase10), QString("-12.345k"));
        EXPECT_EQ(FormatIntSuffices(-12870, SuffixFormatSIBase10), QString("-12.87k"));
        EXPECT_EQ(FormatIntSuffices(-128000000, SuffixFormatSIBase10), QString("-128M"));
        EXPECT_EQ(FormatIntSuffices(-17000000000000ll, SuffixFormatSIBase10), QString("-17T"));
        EXPECT_EQ(FormatIntSuffices(-17850000000000ll, SuffixFormatSIBase10), QString("-17.85T"));
        EXPECT_EQ(FormatIntSuffices(-17851000000000ll, SuffixFormatSIBase10), QString("-17.851T"));
        EXPECT_EQ(FormatIntSuffices(-17851290000000ll, SuffixFormatSIBase10), QString("-17.851T"));
        EXPECT_EQ(FormatIntSuffices(-17851690000000ll, SuffixFormatSIBase10), QString("-17.852T"));
        EXPECT_EQ(FormatIntSuffices(-17851690000000000ll, SuffixFormatSIBase10), QString("-17.852P"));

        EXPECT_EQ(FormatIntSuffices(0, SuffixFormatSIBase2), QString("0"));
        EXPECT_EQ(FormatIntSuffices(5, SuffixFormatSIBase2), QString("5"));
        EXPECT_EQ(FormatIntSuffices(123, SuffixFormatSIBase2), QString("123"));
        EXPECT_EQ(FormatIntSuffices(1023, SuffixFormatSIBase2), QString("1023"));
        EXPECT_EQ(FormatIntSuffices(1024, SuffixFormatSIBase2), QString("1Ki"));
        EXPECT_EQ(FormatIntSuffices(54400, SuffixFormatSIBase2), QString("53.125Ki"));
        EXPECT_EQ(FormatIntSuffices(55040, SuffixFormatSIBase2), QString("53.75Ki"));
        EXPECT_EQ(FormatIntSuffices(87665, SuffixFormatSIBase2), QString("85.61Ki"));
        EXPECT_EQ(FormatIntSuffices(95668, SuffixFormatSIBase2), QString("93.426Ki"));
        EXPECT_EQ(FormatIntSuffices(25165824, SuffixFormatSIBase2), QString("24Mi"));
        EXPECT_EQ(FormatIntSuffices(7487674185154ll, SuffixFormatSIBase2), QString("6.81Ti"));
        EXPECT_EQ(FormatIntSuffices(-0, SuffixFormatSIBase2), QString("0"));
        EXPECT_EQ(FormatIntSuffices(-5, SuffixFormatSIBase2), QString("-5"));
        EXPECT_EQ(FormatIntSuffices(-123, SuffixFormatSIBase2), QString("-123"));
        EXPECT_EQ(FormatIntSuffices(-1023, SuffixFormatSIBase2), QString("-1023"));
        EXPECT_EQ(FormatIntSuffices(-1024, SuffixFormatSIBase2), QString("-1Ki"));
        EXPECT_EQ(FormatIntSuffices(-54400, SuffixFormatSIBase2), QString("-53.125Ki"));
        EXPECT_EQ(FormatIntSuffices(-55040, SuffixFormatSIBase2), QString("-53.75Ki"));
        EXPECT_EQ(FormatIntSuffices(-87665, SuffixFormatSIBase2), QString("-85.61Ki"));
        EXPECT_EQ(FormatIntSuffices(-95668, SuffixFormatSIBase2), QString("-93.426Ki"));
        EXPECT_EQ(FormatIntSuffices(-25165824, SuffixFormatSIBase2), QString("-24Mi"));
        EXPECT_EQ(FormatIntSuffices(-7487674185154ll, SuffixFormatSIBase2), QString("-6.81Ti"));

        EXPECT_EQ(FormatIntSuffices(0, SuffixFormatPercent), QString("0%"));
        EXPECT_EQ(FormatIntSuffices(1, SuffixFormatPercent), QString("100%"));
        EXPECT_EQ(FormatIntSuffices(6, SuffixFormatPercent), QString("600%"));
        EXPECT_EQ(FormatIntSuffices(13, SuffixFormatPercent), QString("1300%"));
        EXPECT_EQ(FormatIntSuffices(1557, SuffixFormatPercent), QString("155700%"));
        EXPECT_EQ(FormatIntSuffices(-0, SuffixFormatPercent), QString("0%"));
        EXPECT_EQ(FormatIntSuffices(-1, SuffixFormatPercent), QString("-100%"));
        EXPECT_EQ(FormatIntSuffices(-6, SuffixFormatPercent), QString("-600%"));
        EXPECT_EQ(FormatIntSuffices(-13, SuffixFormatPercent), QString("-1300%"));
        EXPECT_EQ(FormatIntSuffices(-1557, SuffixFormatPercent), QString("-155700%"));

        EXPECT_EQ(FormatIntSuffices(0, SuffixFormatPPM), QString("0ppm"));
        EXPECT_EQ(FormatIntSuffices(1, SuffixFormatPPM), QString("1000000ppm"));
        EXPECT_EQ(FormatIntSuffices(6, SuffixFormatPPM), QString("6000000ppm"));
        EXPECT_EQ(FormatIntSuffices(13, SuffixFormatPPM), QString("13000000ppm"));
        EXPECT_EQ(FormatIntSuffices(1557, SuffixFormatPPM), QString("1557000000ppm"));
        EXPECT_EQ(FormatIntSuffices(-0, SuffixFormatPPM), QString("0ppm"));
        EXPECT_EQ(FormatIntSuffices(-1, SuffixFormatPPM), QString("-1000000ppm"));
        EXPECT_EQ(FormatIntSuffices(-6, SuffixFormatPPM), QString("-6000000ppm"));
        EXPECT_EQ(FormatIntSuffices(-13, SuffixFormatPPM), QString("-13000000ppm"));
        EXPECT_EQ(FormatIntSuffices(-1557, SuffixFormatPPM), QString("-1557000000ppm"));

        EXPECT_EQ(FormatIntSuffices(0, SuffixFormatNone), QString("0"));
        EXPECT_EQ(FormatIntSuffices(5, SuffixFormatNone), QString("5"));
        EXPECT_EQ(FormatIntSuffices(123, SuffixFormatNone), QString("123"));
        EXPECT_EQ(FormatIntSuffices(500, SuffixFormatNone), QString("500"));
        EXPECT_EQ(FormatIntSuffices(12345, SuffixFormatNone), QString("12345"));
        EXPECT_EQ(FormatIntSuffices(12870, SuffixFormatNone), QString("12870"));
        EXPECT_EQ(FormatIntSuffices(128000000, SuffixFormatNone), QString("128000000"));
        EXPECT_EQ(FormatIntSuffices(17000000000000ll, SuffixFormatNone), QString("17000000000000"));
        EXPECT_EQ(FormatIntSuffices(17850000000000ll, SuffixFormatNone), QString("17850000000000"));
        EXPECT_EQ(FormatIntSuffices(17851000000000ll, SuffixFormatNone), QString("17851000000000"));
        EXPECT_EQ(FormatIntSuffices(17851290000000ll, SuffixFormatNone), QString("17851290000000"));
        EXPECT_EQ(FormatIntSuffices(17851690000000ll, SuffixFormatNone), QString("17851690000000"));
        EXPECT_EQ(FormatIntSuffices(17851690000000000ll, SuffixFormatNone), QString("17851690000000000"));
        EXPECT_EQ(FormatIntSuffices(-0, SuffixFormatNone), QString("0"));
        EXPECT_EQ(FormatIntSuffices(-5, SuffixFormatNone), QString("-5"));
        EXPECT_EQ(FormatIntSuffices(-123, SuffixFormatNone), QString("-123"));
        EXPECT_EQ(FormatIntSuffices(-500, SuffixFormatNone), QString("-500"));
        EXPECT_EQ(FormatIntSuffices(-12345, SuffixFormatNone), QString("-12345"));
        EXPECT_EQ(FormatIntSuffices(-12870, SuffixFormatNone), QString("-12870"));
        EXPECT_EQ(FormatIntSuffices(-128000000, SuffixFormatNone), QString("-128000000"));
        EXPECT_EQ(FormatIntSuffices(-17000000000000ll, SuffixFormatNone), QString("-17000000000000"));
        EXPECT_EQ(FormatIntSuffices(-17850000000000ll, SuffixFormatNone), QString("-17850000000000"));
        EXPECT_EQ(FormatIntSuffices(-17851000000000ll, SuffixFormatNone), QString("-17851000000000"));
        EXPECT_EQ(FormatIntSuffices(-17851290000000ll, SuffixFormatNone), QString("-17851290000000"));
        EXPECT_EQ(FormatIntSuffices(-17851690000000ll, SuffixFormatNone), QString("-17851690000000"));
        EXPECT_EQ(FormatIntSuffices(-17851690000000000ll, SuffixFormatNone), QString("-17851690000000000"));
    }

    {
        EXPECT_EQ(FormatFloatSuffices(0.0, SuffixFormatSIBase10), QString("0"));
        EXPECT_EQ(FormatFloatSuffices(5.0, SuffixFormatSIBase10), QString("5"));
        EXPECT_EQ(FormatFloatSuffices(123.0, SuffixFormatSIBase10), QString("123"));
        EXPECT_EQ(FormatFloatSuffices(500.0, SuffixFormatSIBase10), QString("500"));
        EXPECT_EQ(FormatFloatSuffices(12345.0, SuffixFormatSIBase10), QString("12.345k"));
        EXPECT_EQ(FormatFloatSuffices(17851690000000000.0, SuffixFormatSIBase10), QString("17.852P"));
        EXPECT_EQ(FormatFloatSuffices(2841.29, SuffixFormatSIBase10), QString("2.841k"));
        EXPECT_EQ(FormatFloatSuffices(2841.79, SuffixFormatSIBase10), QString("2.842k"));
        EXPECT_EQ(FormatFloatSuffices(0.19, SuffixFormatSIBase10), QString("190m"));
        EXPECT_EQ(FormatFloatSuffices(0.19518, SuffixFormatSIBase10), QString("195.18m"));
        EXPECT_EQ(FormatFloatSuffices(0.19518531, SuffixFormatSIBase10), QString("195.185m"));
        EXPECT_EQ(FormatFloatSuffices(0.19518571, SuffixFormatSIBase10), QString("195.186m"));
        EXPECT_EQ(FormatFloatSuffices(0.001, SuffixFormatSIBase10), QString("1m"));
        EXPECT_EQ(FormatFloatSuffices(0.0001, SuffixFormatSIBase10), QString("100u"));
        EXPECT_EQ(FormatFloatSuffices(0.000001, SuffixFormatSIBase10), QString("1u"));
        EXPECT_EQ(FormatFloatSuffices(0.000000001, SuffixFormatSIBase10), QString("1n"));
        EXPECT_EQ(FormatFloatSuffices(0.000000000001, SuffixFormatSIBase10), QString("1p"));
        EXPECT_EQ(FormatFloatSuffices(0.00000000006178, SuffixFormatSIBase10), QString("61.78p"));
        EXPECT_EQ(FormatFloatSuffices(0.00000000006178168, SuffixFormatSIBase10), QString("61.782p"));
        EXPECT_EQ(FormatFloatSuffices(0.000000000000001, SuffixFormatSIBase10), QString("1f"));
        EXPECT_EQ(FormatFloatSuffices(0.000000000000000001, SuffixFormatSIBase10), QString("0.001f"));
        EXPECT_EQ(FormatFloatSuffices(0.0000000000000000001, SuffixFormatSIBase10), QString("1e-19"));
        EXPECT_EQ(FormatFloatSuffices(0.00000000000000000001, SuffixFormatSIBase10), QString("1e-20"));
        EXPECT_EQ(FormatFloatSuffices(57384e-106, SuffixFormatSIBase10), QString("5.738e-102"));
        EXPECT_EQ(FormatFloatSuffices(57386e-106, SuffixFormatSIBase10), QString("5.739e-102"));
        EXPECT_EQ(FormatFloatSuffices(-0.0, SuffixFormatSIBase10), QString("0"));
        EXPECT_EQ(FormatFloatSuffices(-5.0, SuffixFormatSIBase10), QString("-5"));
        EXPECT_EQ(FormatFloatSuffices(-123.0, SuffixFormatSIBase10), QString("-123"));
        EXPECT_EQ(FormatFloatSuffices(-500.0, SuffixFormatSIBase10), QString("-500"));
        EXPECT_EQ(FormatFloatSuffices(-12345.0, SuffixFormatSIBase10), QString("-12.345k"));
        EXPECT_EQ(FormatFloatSuffices(-17851690000000000.0, SuffixFormatSIBase10), QString("-17.852P"));
        EXPECT_EQ(FormatFloatSuffices(-2841.29, SuffixFormatSIBase10), QString("-2.841k"));
        EXPECT_EQ(FormatFloatSuffices(-2841.79, SuffixFormatSIBase10), QString("-2.842k"));
        EXPECT_EQ(FormatFloatSuffices(-0.19, SuffixFormatSIBase10), QString("-190m"));
        EXPECT_EQ(FormatFloatSuffices(-0.19518, SuffixFormatSIBase10), QString("-195.18m"));
        EXPECT_EQ(FormatFloatSuffices(-0.19518531, SuffixFormatSIBase10), QString("-195.185m"));
        EXPECT_EQ(FormatFloatSuffices(-0.19518571, SuffixFormatSIBase10), QString("-195.186m"));
        EXPECT_EQ(FormatFloatSuffices(-0.001, SuffixFormatSIBase10), QString("-1m"));
        EXPECT_EQ(FormatFloatSuffices(-0.0001, SuffixFormatSIBase10), QString("-100u"));
        EXPECT_EQ(FormatFloatSuffices(-0.000001, SuffixFormatSIBase10), QString("-1u"));
        EXPECT_EQ(FormatFloatSuffices(-0.000000001, SuffixFormatSIBase10), QString("-1n"));
        EXPECT_EQ(FormatFloatSuffices(-0.000000000001, SuffixFormatSIBase10), QString("-1p"));
        EXPECT_EQ(FormatFloatSuffices(-0.00000000006178, SuffixFormatSIBase10), QString("-61.78p"));
        EXPECT_EQ(FormatFloatSuffices(-0.00000000006178168, SuffixFormatSIBase10), QString("-61.782p"));
        EXPECT_EQ(FormatFloatSuffices(-0.000000000000001, SuffixFormatSIBase10), QString("-1f"));
        EXPECT_EQ(FormatFloatSuffices(-0.000000000000000001, SuffixFormatSIBase10), QString("-0.001f"));
        EXPECT_EQ(FormatFloatSuffices(-0.0000000000000000001, SuffixFormatSIBase10), QString("-1e-19"));
        EXPECT_EQ(FormatFloatSuffices(-0.00000000000000000001, SuffixFormatSIBase10), QString("-1e-20"));
        EXPECT_EQ(FormatFloatSuffices(-57384e-106, SuffixFormatSIBase10), QString("-5.738e-102"));
        EXPECT_EQ(FormatFloatSuffices(-57386e-106, SuffixFormatSIBase10), QString("-5.739e-102"));

        EXPECT_EQ(FormatFloatSuffices(0.0, SuffixFormatSIBase2), QString("0"));
        EXPECT_EQ(FormatFloatSuffices(5.0, SuffixFormatSIBase2), QString("5"));
        EXPECT_EQ(FormatFloatSuffices(123.0, SuffixFormatSIBase2), QString("123"));
        EXPECT_EQ(FormatFloatSuffices(1023.0, SuffixFormatSIBase2), QString("1023"));
        EXPECT_EQ(FormatFloatSuffices(1024.0, SuffixFormatSIBase2), QString("1Ki"));
        EXPECT_EQ(FormatFloatSuffices(54400.0, SuffixFormatSIBase2), QString("53.125Ki"));
        EXPECT_EQ(FormatFloatSuffices(55040.0, SuffixFormatSIBase2), QString("53.75Ki"));
        EXPECT_EQ(FormatFloatSuffices(87665.0, SuffixFormatSIBase2), QString("85.61Ki"));
        EXPECT_EQ(FormatFloatSuffices(95668.0, SuffixFormatSIBase2), QString("93.426Ki"));
        EXPECT_EQ(FormatFloatSuffices(25165824.0, SuffixFormatSIBase2), QString("24Mi"));
        EXPECT_EQ(FormatFloatSuffices(7487674185154.0, SuffixFormatSIBase2), QString("6.81Ti"));
        EXPECT_EQ(FormatFloatSuffices(2841.29, SuffixFormatSIBase2), QString("2.775Ki"));
        EXPECT_EQ(FormatFloatSuffices(0.1, SuffixFormatSIBase2), QString("0.1"));
        EXPECT_EQ(FormatFloatSuffices(0.034, SuffixFormatSIBase2), QString("0.034"));
        EXPECT_EQ(FormatFloatSuffices(0.03412, SuffixFormatSIBase2), QString("0.034"));
        EXPECT_EQ(FormatFloatSuffices(0.03462, SuffixFormatSIBase2), QString("0.035"));
        EXPECT_EQ(FormatFloatSuffices(0.0000000001, SuffixFormatSIBase2), QString("1e-10"));
        EXPECT_EQ(FormatFloatSuffices(1.57e-103, SuffixFormatSIBase2), QString("1.57e-103"));
        EXPECT_EQ(FormatFloatSuffices(-0.0, SuffixFormatSIBase2), QString("0"));
        EXPECT_EQ(FormatFloatSuffices(-5.0, SuffixFormatSIBase2), QString("-5"));
        EXPECT_EQ(FormatFloatSuffices(-123.0, SuffixFormatSIBase2), QString("-123"));
        EXPECT_EQ(FormatFloatSuffices(-1023.0, SuffixFormatSIBase2), QString("-1023"));
        EXPECT_EQ(FormatFloatSuffices(-1024.0, SuffixFormatSIBase2), QString("-1Ki"));
        EXPECT_EQ(FormatFloatSuffices(-54400.0, SuffixFormatSIBase2), QString("-53.125Ki"));
        EXPECT_EQ(FormatFloatSuffices(-55040.0, SuffixFormatSIBase2), QString("-53.75Ki"));
        EXPECT_EQ(FormatFloatSuffices(-87665.0, SuffixFormatSIBase2), QString("-85.61Ki"));
        EXPECT_EQ(FormatFloatSuffices(-95668.0, SuffixFormatSIBase2), QString("-93.426Ki"));
        EXPECT_EQ(FormatFloatSuffices(-25165824.0, SuffixFormatSIBase2), QString("-24Mi"));
        EXPECT_EQ(FormatFloatSuffices(-7487674185154.0, SuffixFormatSIBase2), QString("-6.81Ti"));
        EXPECT_EQ(FormatFloatSuffices(-2841.29, SuffixFormatSIBase2), QString("-2.775Ki"));
        EXPECT_EQ(FormatFloatSuffices(-0.1, SuffixFormatSIBase2), QString("-0.1"));
        EXPECT_EQ(FormatFloatSuffices(-0.034, SuffixFormatSIBase2), QString("-0.034"));
        EXPECT_EQ(FormatFloatSuffices(-0.03412, SuffixFormatSIBase2), QString("-0.034"));
        EXPECT_EQ(FormatFloatSuffices(-0.03462, SuffixFormatSIBase2), QString("-0.035"));
        EXPECT_EQ(FormatFloatSuffices(-0.0000000001, SuffixFormatSIBase2), QString("-1e-10"));
        EXPECT_EQ(FormatFloatSuffices(-1.57e-103, SuffixFormatSIBase2), QString("-1.57e-103"));

        EXPECT_EQ(FormatFloatSuffices(0.0, SuffixFormatPercent), QString("0%"));
        EXPECT_EQ(FormatFloatSuffices(1.0, SuffixFormatPercent), QString("100%"));
        EXPECT_EQ(FormatFloatSuffices(6.0, SuffixFormatPercent), QString("600%"));
        EXPECT_EQ(FormatFloatSuffices(13.0, SuffixFormatPercent), QString("1300%"));
        EXPECT_EQ(FormatFloatSuffices(0.5, SuffixFormatPercent), QString("50%"));
        EXPECT_EQ(FormatFloatSuffices(0.4137, SuffixFormatPercent), QString("41.37%"));
        EXPECT_EQ(FormatFloatSuffices(0.4137143, SuffixFormatPercent), QString("41.371%"));
        EXPECT_EQ(FormatFloatSuffices(0.4137153, SuffixFormatPercent), QString("41.372%"));
        EXPECT_EQ(FormatFloatSuffices(0.01, SuffixFormatPercent), QString("1%"));
        EXPECT_EQ(FormatFloatSuffices(0.001, SuffixFormatPercent), QString("0.1%"));
        EXPECT_EQ(FormatFloatSuffices(0.00001, SuffixFormatPercent), QString("0.001%"));
        EXPECT_EQ(FormatFloatSuffices(0.000001, SuffixFormatPercent), QString("1e-06"));
        EXPECT_EQ(FormatFloatSuffices(-0.0, SuffixFormatPercent), QString("0%"));
        EXPECT_EQ(FormatFloatSuffices(-1.0, SuffixFormatPercent), QString("-100%"));
        EXPECT_EQ(FormatFloatSuffices(-6.0, SuffixFormatPercent), QString("-600%"));
        EXPECT_EQ(FormatFloatSuffices(-13.0, SuffixFormatPercent), QString("-1300%"));
        EXPECT_EQ(FormatFloatSuffices(-0.5, SuffixFormatPercent), QString("-50%"));
        EXPECT_EQ(FormatFloatSuffices(-0.4137, SuffixFormatPercent), QString("-41.37%"));
        EXPECT_EQ(FormatFloatSuffices(-0.4137143, SuffixFormatPercent), QString("-41.371%"));
        EXPECT_EQ(FormatFloatSuffices(-0.4137153, SuffixFormatPercent), QString("-41.372%"));
        EXPECT_EQ(FormatFloatSuffices(-0.01, SuffixFormatPercent), QString("-1%"));
        EXPECT_EQ(FormatFloatSuffices(-0.001, SuffixFormatPercent), QString("-0.1%"));
        EXPECT_EQ(FormatFloatSuffices(-0.00001, SuffixFormatPercent), QString("-0.001%"));
        EXPECT_EQ(FormatFloatSuffices(-0.000001, SuffixFormatPercent), QString("-1e-06"));

        EXPECT_EQ(FormatFloatSuffices(0.0, SuffixFormatPPM), QString("0ppm"));
        EXPECT_EQ(FormatFloatSuffices(1.0, SuffixFormatPPM), QString("1000000ppm"));
        EXPECT_EQ(FormatFloatSuffices(6.0, SuffixFormatPPM), QString("6000000ppm"));
        EXPECT_EQ(FormatFloatSuffices(13.0, SuffixFormatPPM), QString("13000000ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.000001, SuffixFormatPPM), QString("1ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.000006, SuffixFormatPPM), QString("6ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.000155, SuffixFormatPPM), QString("155ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.00000531, SuffixFormatPPM), QString("5.31ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.0000053174, SuffixFormatPPM), QString("5.317ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.0000053176, SuffixFormatPPM), QString("5.318ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.000000001, SuffixFormatPPM), QString("0.001ppm"));
        EXPECT_EQ(FormatFloatSuffices(0.0000000001, SuffixFormatPPM), QString("1e-10"));

        EXPECT_EQ(FormatFloatSuffices(0.0, SuffixFormatNone), QString("0"));
        EXPECT_EQ(FormatFloatSuffices(5.0, SuffixFormatNone), QString("5"));
        EXPECT_EQ(FormatFloatSuffices(123.0, SuffixFormatNone), QString("123"));
        EXPECT_EQ(FormatFloatSuffices(500.0, SuffixFormatNone), QString("500"));
        EXPECT_EQ(FormatFloatSuffices(12345.0, SuffixFormatNone), QString("12345"));
        EXPECT_EQ(FormatFloatSuffices(17851690000000000.0, SuffixFormatNone), QString("17851690000000000"));
        EXPECT_EQ(FormatFloatSuffices(2841.29, SuffixFormatNone), QString("2841.29"));
        EXPECT_EQ(FormatFloatSuffices(2841.79, SuffixFormatNone), QString("2841.79"));
        EXPECT_EQ(FormatFloatSuffices(0.19518531, SuffixFormatNone), QString("0.195"));
        EXPECT_EQ(FormatFloatSuffices(0.19568531, SuffixFormatNone), QString("0.196"));
        EXPECT_EQ(FormatFloatSuffices(0.00000000006178, SuffixFormatNone), QString("6.178e-11"));
        EXPECT_EQ(FormatFloatSuffices(-0.0, SuffixFormatNone), QString("0"));
        EXPECT_EQ(FormatFloatSuffices(-5.0, SuffixFormatNone), QString("-5"));
        EXPECT_EQ(FormatFloatSuffices(-123.0, SuffixFormatNone), QString("-123"));
        EXPECT_EQ(FormatFloatSuffices(-500.0, SuffixFormatNone), QString("-500"));
        EXPECT_EQ(FormatFloatSuffices(-12345.0, SuffixFormatNone), QString("-12345"));
        EXPECT_EQ(FormatFloatSuffices(-17851690000000000.0, SuffixFormatNone), QString("-17851690000000000"));
        EXPECT_EQ(FormatFloatSuffices(-2841.29, SuffixFormatNone), QString("-2841.29"));
        EXPECT_EQ(FormatFloatSuffices(-2841.79, SuffixFormatNone), QString("-2841.79"));
        EXPECT_EQ(FormatFloatSuffices(-0.19518531, SuffixFormatNone), QString("-0.195"));
        EXPECT_EQ(FormatFloatSuffices(-0.19568531, SuffixFormatNone), QString("-0.196"));
        EXPECT_EQ(FormatFloatSuffices(-0.00000000006178, SuffixFormatNone), QString("-6.178e-11"));
    }
}

TEST_F(UtilTest, ResolveSuffices)
{
    bool ok;

    {
        EXPECT_EQ(ResolveIntSuffices("0", &ok), 0); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1", &ok), 1); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1Ohm", &ok), 1); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("145", &ok), 145); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("85913315", &ok), 85913315); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1k", &ok), 1000); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1kV", &ok), 1000); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1kOhm", &ok), 1000); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1K", &ok), 1000); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1M", &ok), 1000000); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1G", &ok), 1000000000); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1T", &ok), 1000000000000ll); EXPECT_TRUE(ok);
        EXPECT_EQ(ResolveIntSuffices("1P", &ok), 1000000000000000ll); EXPECT_TRUE(ok);
    }

    {
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("0", &ok), 0.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1", &ok), 1.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1Ohm", &ok), 1.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("145", &ok), 145.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("85913315", &ok), 85913315.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1k", &ok), 1000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1kV", &ok), 1000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1kOhm", &ok), 1000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1K", &ok), 1000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1M", &ok), 1000000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1G", &ok), 1000000000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1T", &ok), 1000000000000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1P", &ok), 1000000000000000.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1.24k", &ok), 1240.0); EXPECT_TRUE(ok);
        EXPECT_DOUBLE_EQ(ResolveFloatSuffices("1.100k", &ok), 1100.0); EXPECT_TRUE(ok);
    }
}

TEST_F(UtilTest, FilterSQL)
{
    using FieldPair = std::pair<QString, QVariant>;

    QSqlQuery query(db);

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE autotest_filter1 (\n"
            "   %1,\n"
            "   s VARCHAR(64) NOT NULL,\n"
            "   i INTEGER NOT NULL,\n"
            "   b BOOLEAN NOT NULL\n"
            ")"
            ).arg(dbw->generateIntPrimaryKeyCode("an_id")));

    dbw->execAndCheckQuery(query, QString(
            "INSERT INTO autotest_filter1 (an_id, s, i, b) VALUES\n"
            "   (1, 'Lorem ipsum dolor sit amet',          -7,         TRUE),\n"
            "   (2, 'Wer anderen eine Grube graebt',       -1700,      FALSE),\n"
            "   (3, 'Morgenstund hat Gold im Mund',        0,          FALSE),\n"
            "   (4, 'The sum of all fears',                69,         TRUE),\n"
            "   (5, 'sakura sakura yayoi no sora wa',      1337,       TRUE)"
            ));

    auto searchWithFilter = [&](Filter& filter) {
        QString queryStr = QString(
                "SELECT * FROM autotest_filter1 %1 %2"
                ).arg(filter.getSQLWhereCode(),
                      filter.getSQLOrderCode());
        EXPECT_TRUE(query.prepare(queryStr));
        filter.fillSQLWhereCodeBindParams(query);
        dbw->execAndCheckPreparedQuery(query);
    };

    {
        Filter f;
        f.setSQLWhereCode("WHERE i < -10");

        searchWithFilter(f);
        {
            SCOPED_TRACE("");
            ExpectQueryResult(query, {{FieldPair("an_id", 2)}}, true, true);
        }
    }

    {
        Filter f;
        f.setSQLWhereCode("WHERE s LIKE ? OR i<?", {"%sum%", -100});
        f.setSQLOrderCode("ORDER BY i ASC");

        searchWithFilter(f);
        {
            SCOPED_TRACE("");
            ExpectQueryResult(query, {
                {FieldPair("an_id", 2)},
                {FieldPair("an_id", 1)},
                {FieldPair("an_id", 4)}
            }, true, true);
        }
    }
}

TEST_F(UtilTest, KeyVault)
{
    KeyVault& vault = KeyVault::getInstance();

    vault.clear(true);
    vault.setEnabled(false);

    // Check behavior when vault isn't setup yet
    {
        EXPECT_FALSE(vault.isEnabled());
        EXPECT_FALSE(vault.isVaultKeyed());
        EXPECT_FALSE(vault.isVaultDefaultKeyed());
        EXPECT_EQ(vault.getKeyCount(), 0);

        EXPECT_NO_THROW(vault.clear(false));
        EXPECT_NO_THROW(vault.clear(true));

        EXPECT_ANY_THROW(vault.setKey("test1", "value1"));
        EXPECT_ANY_THROW(vault.getKey("test1", "testpw"));
    }

    // Enable vault
    {
        vault.setEnabled(true);
        EXPECT_TRUE(vault.isEnabled());

        vault.clear(false);
        EXPECT_TRUE(vault.isEnabled());

        vault.clear(true);
        EXPECT_TRUE(vault.isEnabled());
    }

    // Check keying with user password
    {
        vault.setVaultPassword("secret1");

        EXPECT_TRUE(vault.isVaultKeyed());
        EXPECT_FALSE(vault.isVaultDefaultKeyed());
        EXPECT_EQ(vault.getKeyCount(), 0);
        EXPECT_FALSE(vault.containsKey("test1"));
        EXPECT_FALSE(vault.containsKey("test2"));

        vault.setKey("test1", "th1515!v3ry53cr3tpw");

        EXPECT_EQ(vault.getKeyCount(), 1);

        vault.setKey("test2", "hunter2");

        EXPECT_EQ(vault.getKeyCount(), 2);

        QString test2 = vault.getKey("test2", "secret1");
        EXPECT_EQ(test2, QString("hunter2"));
        vault.wipeKey(test2);
        EXPECT_NE(test2, QString("hunter2"));

        QString test1 = vault.getKey("test1", "secret1");
        EXPECT_EQ(test1, QString("th1515!v3ry53cr3tpw"));
        vault.wipeKey(test1);
        EXPECT_NE(test1, QString("th1515!v3ry53cr3tpw"));

        EXPECT_THROW(vault.getKey("test1", "secret2"), InvalidCredentialsException);
        EXPECT_THROW(vault.getKey("test2", QString()), InvalidCredentialsException);

        EXPECT_TRUE(vault.containsKey("test1"));
        EXPECT_TRUE(vault.containsKey("test2"));
    }

    // Check re-keying to another user password
    {
        EXPECT_THROW(vault.setVaultPassword("secret2", "secret0"), InvalidCredentialsException);
        EXPECT_THROW(vault.setVaultPassword("secret2"), InvalidCredentialsException);

        EXPECT_NO_THROW(vault.setVaultPassword("secret2", "secret1"));

        EXPECT_TRUE(vault.isVaultKeyed());
        EXPECT_FALSE(vault.isVaultDefaultKeyed());
        EXPECT_EQ(vault.getKeyCount(), 2);

        QString test2 = vault.getKey("test2", "secret2");
        EXPECT_EQ(test2, QString("hunter2"));
        vault.wipeKey(test2);

        QString test1 = vault.getKey("test1", "secret2");
        EXPECT_EQ(test1, QString("th1515!v3ry53cr3tpw"));
        vault.wipeKey(test1);

        EXPECT_THROW(vault.getKey("test1", "secret1"), InvalidCredentialsException);
    }

    // Check re-keying to default password
    {
        EXPECT_THROW(vault.setVaultPassword("secret2", "secret0"), InvalidCredentialsException);
        EXPECT_THROW(vault.setVaultPassword("secret2"), InvalidCredentialsException);

        EXPECT_NO_THROW(vault.setVaultPassword(QString(), "secret2"));

        EXPECT_TRUE(vault.isVaultKeyed());
        EXPECT_TRUE(vault.isVaultDefaultKeyed());
        EXPECT_EQ(vault.getKeyCount(), 2);

        QString test2 = vault.getKey("test2", QString());
        EXPECT_EQ(test2, QString("hunter2"));
        vault.wipeKey(test2);

        QString test1 = vault.getKey("test1", QString());
        EXPECT_EQ(test1, QString("th1515!v3ry53cr3tpw"));
        vault.wipeKey(test1);

        EXPECT_NO_THROW(vault.getKey("test1", "secret2"));
    }

    // Check re-keying from default to user password
    {
        EXPECT_NO_THROW(vault.setVaultPassword("secret3", "somethinginvalid"));

        EXPECT_TRUE(vault.isVaultKeyed());
        EXPECT_FALSE(vault.isVaultDefaultKeyed());
        EXPECT_EQ(vault.getKeyCount(), 2);

        QString test2 = vault.getKey("test2", "secret3");
        EXPECT_EQ(test2, QString("hunter2"));
        vault.wipeKey(test2);

        QString test1 = vault.getKey("test1", "secret3");
        EXPECT_EQ(test1, QString("th1515!v3ry53cr3tpw"));
        vault.wipeKey(test1);

        EXPECT_THROW(vault.getKey("test1", "secret2"), InvalidCredentialsException);
        EXPECT_THROW(vault.getKey("test1", QString()), InvalidCredentialsException);
    }

    EXPECT_TRUE(vault.isEnabled());
    vault.clear(true);
}


}
