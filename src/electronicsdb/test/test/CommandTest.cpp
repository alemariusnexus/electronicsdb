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
#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <gtest/gtest.h>
#include <nxcommon/log.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../db/DatabaseConnection.h"
#include "../../command/sql/SQLDeleteCommand.h"
#include "../../command/sql/SQLInsertCommand.h"
#include "../../command/sql/SQLUpdateCommand.h"
#include "../../command/SQLEditCommand.h"
#include "../../command/EditStack.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../TestManager.h"
#include "../testutil.h"

namespace electronicsdb
{


class CommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        System& sys = *System::getInstance();
        TestManager& testMgr = TestManager::getInstance();
        EditStack* editStack = sys.getEditStack();

        editStack->setUndoStackSize(100);
        editStack->setRedoStackSize(100);

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


TEST_F(CommandTest, InsertBasic)
{
    using FieldPair = std::pair<QString, QVariant>;

    QSqlQuery query(db);

    QList<dbid_t> firstIDs;

    SQLInsertCommand* c1 = nullptr;
    SQLInsertCommand* c2 = nullptr;

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE autotest_cmdins1 (\n"
            "   %1,\n"
            "   a VARCHAR(64) DEFAULT 'def a',\n"
            "   b INTEGER NOT NULL DEFAULT 420,\n"
            "   c BOOLEAN\n"
            ")"
            ).arg(dbw->generateIntPrimaryKeyCode("an_id")));

    // Insert some basic rows, reverting and re-committing them.
    {
        SQLInsertCommand::DataMapList data = {
                {FieldPair("a", "the quick"), FieldPair("b", 69), FieldPair("c", false)},
                {FieldPair("a", "brown fox jumps"), FieldPair("b", 0x0BADC0DE), FieldPair("c", true)},
                {FieldPair("a", "over the lazy dog"), FieldPair("b", -123456), FieldPair("c", true)}
        };
        c1 = new SQLInsertCommand("autotest_cmdins1", "an_id", data);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);

        SQLCommand* listenerCmd = nullptr;
        SQLCommand::Operation listenerOp = SQLCommand::OpInvalid;
        QList<dbid_t> listenerInsIDs;
        c1->setListener([&](SQLCommand* cmd, SQLCommand::Operation op) {
            listenerCmd = cmd;
            listenerOp = op;
            listenerInsIDs = c1->getInsertIDs();
        });

        c1->commit();

        firstIDs = listenerInsIDs;

        EXPECT_EQ(listenerCmd, c1);
        EXPECT_EQ(listenerOp, SQLCommand::OpCommit);
        EXPECT_EQ(listenerInsIDs.size(), data.size());

        EXPECT_EQ(listenerInsIDs, c1->getInsertIDs());
        {
            SCOPED_TRACE("");
            ExpectValidIDs(listenerInsIDs);
        }

        dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdins1 ORDER BY an_id ASC");
        {
            SCOPED_TRACE("");
            ExpectQueryResult(query, data, true, true);
        }

        c1->revert();

        EXPECT_EQ(listenerOp, SQLCommand::OpRevert);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);

        listenerInsIDs.clear();

        c1->commit();

        EXPECT_EQ(listenerOp, SQLCommand::OpCommit);
        EXPECT_EQ(listenerInsIDs, firstIDs);

        dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdins1 ORDER BY an_id ASC");
        {
            SCOPED_TRACE("");
            ExpectQueryResult(query, data, true, true);
        }

        c1->setListener(SQLCommand::Listener());
    }

    // Insert more rows
    {
        SQLInsertCommand::DataMapList data = {
                {FieldPair("a", "lorem ipsum"), FieldPair("b", 98765), FieldPair("c", true)},
                {FieldPair("a", "dolor sit amet"), FieldPair("b", 0x1DA15BAD), FieldPair("c", false)}
        };
        c2 = new SQLInsertCommand("autotest_cmdins1", "an_id", data);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        QList<dbid_t> listenerInsIDs;
        c2->setListener([&](SQLCommand* cmd, SQLCommand::Operation op) {
            if (op != SQLCommand::OpCommit) {
                return;
            }
            listenerInsIDs = c2->getInsertIDs();
        });

        c2->commit();

        EXPECT_EQ(listenerInsIDs.size(), data.size());

        EXPECT_EQ(listenerInsIDs, c2->getInsertIDs());
        {
            SCOPED_TRACE("");
            ExpectValidIDs(listenerInsIDs);
        }

        auto allIDs = firstIDs;
        allIDs << listenerInsIDs;

        {
            SCOPED_TRACE("");
            ExpectValidIDs(allIDs);
        }

        c1->revert();

        QList<QMap<QString, QVariant>> dataAfterC1Revert = {
                {FieldPair("an_id", listenerInsIDs[0])},
                {FieldPair("an_id", listenerInsIDs[1])}
        };
        dbw->execAndCheckQuery(query, "SELECT an_id FROM autotest_cmdins1 ORDER BY an_id ASC");
        {
            SCOPED_TRACE("");
            ExpectQueryResult(query, dataAfterC1Revert);
        }
    }

    // Insert rows with fixed IDs
    {
        SQLInsertCommand::DataMapList data = {
                {FieldPair("an_id", 1000), FieldPair("a", "this has a"), FieldPair("b", 1), FieldPair("c", true)},
                {FieldPair("an_id", 980), FieldPair("a", "fixed ID"), FieldPair("b", 0), FieldPair("c", false)}
        };
        SQLInsertCommand c3("autotest_cmdins1", "an_id", data);

        for (int i = 0 ; i < 3 ; i++) {
            c3.commit();

            auto c3IDs = c3.getInsertIDs();
            EXPECT_EQ(c3IDs.size(), data.size());

            EXPECT_EQ(c3IDs[0], 1000);
            EXPECT_EQ(c3IDs[1], 980);

            dbw->execAndCheckQuery(query, "SELECT an_id FROM autotest_cmdins1 WHERE an_id > 900 ORDER BY an_id ASC");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 980);
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 1000);
            EXPECT_FALSE(query.next());

            c3.revert();
        }
    }

    // Empty insert
    {
        SQLInsertCommand::DataMapList data = {};
        SQLInsertCommand c3("autotest_cmdins1", "an_id", data);

        for (int i = 0 ; i < 3 ; i++) {
            c3.commit();

            auto c3IDs = c3.getInsertIDs();
            EXPECT_EQ(c3IDs.size(), 0);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            c3.revert();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);
        }
    }

    delete c1;
    delete c2;

    dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins1");
    EXPECT_TRUE(query.next());
    EXPECT_EQ(query.value(0).toInt(), 2);
}

TEST_F(CommandTest, InsertWithHoles)
{
    using FieldPair = std::pair<QString, QVariant>;

    QSqlQuery query(db);

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE autotest_cmdins2 (\n"
            "   %1,\n"
            "   a VARCHAR(64) DEFAULT 'def a',\n"
            "   b INTEGER DEFAULT 420,\n"
            "   c TEXT\n"
            ")"
            ).arg(dbw->generateIntPrimaryKeyCode("an_id")));

    // Insert some rows without values for field 'b'
    {
        SQLInsertCommand::DataMapList data = {
                {FieldPair("a", "What the fuck did"), FieldPair("c", "wayne")},
                {FieldPair("a", "you just fucking"), FieldPair("c", "interessierts")},
                {FieldPair("a", "say about me"), FieldPair("c", "zug")}
        };
        SQLInsertCommand c1("autotest_cmdins2", "an_id", data);

        c1.commit();

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM autotest_cmdins2 WHERE "
                "a='What the fuck did' AND b=420 AND c='wayne'"
                ));
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM autotest_cmdins2 WHERE "
                "a='you just fucking' AND b=420 AND c='interessierts'"
                ));
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM autotest_cmdins2 WHERE "
                "a='say about me' AND b=420 AND c='zug'"
                ));
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);
    }

    // Insert some rows where not all of them have the same set of fields
    {
        SQLInsertCommand::DataMapList data = {
                {FieldPair("a", "What the fuck did"), FieldPair("b", 0x5ADD0D0)},
                {FieldPair("a", "you just fucking"), FieldPair("c", "interessierts")},
        };
        SQLInsertCommand c2("autotest_cmdins2", "an_id", data);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        c2.commit();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 5);
    }

    // Insert rows with mixed fields, and mixed presence of IDs
    {
        SQLInsertCommand::DataMapList data = {
                {FieldPair("a", "mixed one"), FieldPair("c", "apple")},
                {FieldPair("b", -157), FieldPair("c", "apple")},
                {FieldPair("an_id", 2000), FieldPair("a", "mixed three"), FieldPair("c", "pen")},
                {FieldPair("a", "mixed four"), FieldPair("b", -5498)},
                {FieldPair("an_id", 2010), FieldPair("a", "mixed five"), FieldPair("c", "apple-pen")},
                {FieldPair("an_id", 2020), FieldPair("b", 1221), FieldPair("c", "apple-pen")}
        };
        SQLInsertCommand c3("autotest_cmdins2", "an_id", data);

        c3.commit();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 11);

        auto c3IDs = c3.getInsertIDs();

        {
            SCOPED_TRACE("");
            ExpectValidIDs(c3IDs);
        }
        EXPECT_EQ(c3IDs.size(), data.size());
        EXPECT_EQ(c3IDs[2], 2000);
        EXPECT_EQ(c3IDs[4], 2010);
        EXPECT_EQ(c3IDs[5], 2020);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2 WHERE an_id IN (2000, 2010, 2020)");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        for (int i = 0 ; i < 3 ; i++) {
            c3.revert();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2 WHERE an_id IN (2000, 2010, 2020)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 0);

            c3.commit();

            EXPECT_EQ(c3.getInsertIDs(), c3IDs);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 11);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdins2 WHERE an_id IN (2000, 2010, 2020)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 3);
        }
    }
}

TEST_F(CommandTest, InsertEmpty)
{
    using FieldPair = std::pair<QString, QVariant>;

    QSqlQuery query(db);

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE autotest_cmdins3 (\n"
            "   %1,\n"
            "   a VARCHAR(64) DEFAULT 'def a',\n"
            "   b INTEGER DEFAULT 420\n"
            ")"
    ).arg(dbw->generateIntPrimaryKeyCode("an_id")));

    // Insert zero rows
    {
        SQLInsertCommand::DataMapList data = {};
        SQLInsertCommand c1("autotest_cmdins3", "an_id", data);

        c1.commit();

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM autotest_cmdins3"
        ));
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);
    }

    // Insert some rows with no field values
    {
        SQLInsertCommand::DataMapList data = {
                {}, {}
        };
        SQLInsertCommand c1("autotest_cmdins3", "an_id", data);

        c1.commit();

        dbw->execAndCheckQuery(query, QString(
                "SELECT COUNT(*) FROM autotest_cmdins3 WHERE "
                "a='def a' AND b=420"
        ));
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 2);

        ExpectValidIDs(c1.getInsertIDs());
    }
}

TEST_F(CommandTest, UpdateBasic)
{
    QSqlQuery query(db);

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE autotest_cmdupd1 (\n"
            "   %1,\n"
            "   a VARCHAR(64),\n"
            "   b INTEGER,\n"
            "   c BOOLEAN\n"
            ")"
            ).arg(dbw->generateIntPrimaryKeyCode("an_id")));

    {
        dbw->execAndCheckQuery(
                "INSERT INTO autotest_cmdupd1 (an_id, a, b, c) VALUES "
                "(1, 'Hello World', 1337, TRUE), "
                "(2, 'same procedure', 420, FALSE), "
                "(3, 'as every year', 69, FALSE), "
                "(4, 'James', 13, TRUE)"
                );

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdupd1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 4);

        {
            SQLUpdateCommand c1("autotest_cmdupd1", "an_id", 3);
            c1.addFieldValue("a", "as last year");
            c1.addFieldValue("b", 96);

            for (int i = 0 ; i < 3 ; i++) {
                c1.commit();
                dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdupd1 WHERE an_id=3");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value("a").toString(), QString("as last year"));
                EXPECT_EQ(query.value("b").toInt(), 96);
                EXPECT_EQ(query.value("c").toBool(), false);
                EXPECT_FALSE(query.next());

                dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdupd1");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value(0).toInt(), 4);

                c1.revert();
                dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdupd1 WHERE an_id=3");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value("a").toString(), QString("as every year"));
                EXPECT_EQ(query.value("b").toInt(), 69);
                EXPECT_EQ(query.value("c").toBool(), false);
                EXPECT_FALSE(query.next());

                dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdupd1");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value(0).toInt(), 4);
            }

            c1.commit();
        }

        {
            SQLUpdateCommand c1("autotest_cmdupd1", "an_id", 4);
            c1.addFieldValue("a", "Miss Sophie");
            c1.addFieldValue("c", false);

            for (int i = 0 ; i < 3 ; i++) {
                c1.commit();
                dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdupd1 WHERE an_id=4");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value("a").toString(), QString("Miss Sophie"));
                EXPECT_EQ(query.value("b").toInt(), 13);
                EXPECT_EQ(query.value("c").toBool(), false);
                EXPECT_FALSE(query.next());

                dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdupd1");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value(0).toInt(), 4);

                c1.revert();
                dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdupd1 WHERE an_id=4");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value("a").toString(), QString("James"));
                EXPECT_EQ(query.value("b").toInt(), 13);
                EXPECT_EQ(query.value("c").toBool(), true);
                EXPECT_FALSE(query.next());

                dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdupd1");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value(0).toInt(), 4);
            }

            c1.commit();
        }

        {
            SQLUpdateCommand c1("autotest_cmdupd1", "an_id", 2);
            c1.addFieldValue("c", true);

            for (int i = 0 ; i < 3 ; i++) {
                c1.commit();
                dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdupd1 WHERE an_id=2");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value("a").toString(), QString("same procedure"));
                EXPECT_EQ(query.value("b").toInt(), 420);
                EXPECT_EQ(query.value("c").toBool(), true);
                EXPECT_FALSE(query.next());

                dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdupd1");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value(0).toInt(), 4);

                c1.revert();
                dbw->execAndCheckQuery(query, "SELECT * FROM autotest_cmdupd1 WHERE an_id=2");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value("a").toString(), QString("same procedure"));
                EXPECT_EQ(query.value("b").toInt(), 420);
                EXPECT_EQ(query.value("c").toBool(), false);
                EXPECT_FALSE(query.next());

                dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmdupd1");
                EXPECT_TRUE(query.next());
                EXPECT_EQ(query.value(0).toInt(), 4);
            }

            c1.commit();
        }
    }
}

TEST_F(CommandTest, DeleteBasic)
{
    using FieldPair = std::pair<QString, QVariant>;

    QSqlQuery query(db);

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE autotest_cmddel1 (\n"
            "   %1,\n"
            "   a VARCHAR(64),\n"
            "   b INTEGER,\n"
            "   c BOOLEAN\n"
            ")"
            ).arg(dbw->generateIntPrimaryKeyCode("an_id")));

    QList<QMap<QString, QVariant>> data = {
            {FieldPair("an_id", 1),     FieldPair("a", "one"),      FieldPair("b", 13),     FieldPair("c", true)},
            {FieldPair("an_id", 2),     FieldPair("a", "two"),      FieldPair("b", -15),    FieldPair("c", false)},
            {FieldPair("an_id", 3),     FieldPair("a", "three"),    FieldPair("b", 7531),   FieldPair("c", true)},
            {FieldPair("an_id", 4),     FieldPair("a", "four"),     FieldPair("b", 48),     FieldPair("c", true)},
            {FieldPair("an_id", 5),     FieldPair("a", "five"),     FieldPair("b", -1),     FieldPair("c", false)}
    };

    SQLInsertCommand insCmd("autotest_cmddel1", "an_id", data);
    insCmd.commit();

    dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
    EXPECT_TRUE(query.next());
    EXPECT_EQ(query.value(0).toInt(), data.size());

    {
        SQLDeleteCommand c("autotest_cmddel1", "an_id", QList<dbid_t>{2, 4, 5});

        for (int i = 0 ; i < 3 ; i++) {
            c.commit();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (2,4,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 0);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,3)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            c.revert();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,2,3,4,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);
        }
    }

    {
        SQLDeleteCommand c("autotest_cmddel1", "c=TRUE");

        for (int i = 0 ; i < 3 ; i++) {
            c.commit();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,3,4)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 0);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (2,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            c.revert();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,2,3,4,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);
        }
    }

    {
        SQLDeleteCommand c("autotest_cmddel1", "b<0");

        for (int i = 0 ; i < 3 ; i++) {
            c.commit();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 3);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (2,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 0);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,3,4)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 3);

            c.revert();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,2,3,4,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);
        }
    }

    {
        SQLDeleteCommand c("autotest_cmddel1", "b>? OR b<?", QList<QVariant>{100, -10});

        for (int i = 0 ; i < 3 ; i++) {
            c.commit();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 3);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (2,3)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 0);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,4,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 3);

            c.revert();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1 WHERE an_id IN (1,2,3,4,5)");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);
        }
    }

    {
        SQLDeleteCommand c("autotest_cmddel1", "an_id", QList<dbid_t>{});

        for (int i = 0 ; i < 3 ; i++) {
            c.commit();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);

            c.revert();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_cmddel1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 5);
        }
    }
}

TEST_F(CommandTest, ReservedKeywords)
{
    using FieldPair = std::pair<QString, QVariant>;

    QSqlQuery query(db);

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE %2 (\n"
            "   %1,\n"
            "   %3 VARCHAR(64) NOT NULL,\n"
            "   %4 INTEGER NOT NULL"
            ")"
            ).arg(dbw->generateIntPrimaryKeyCode("UNIQUE"),
                  dbw->escapeIdentifier("IF"),
                  dbw->escapeIdentifier("NULL"),
                  dbw->escapeIdentifier("INTEGER")));

    {
        QList<QMap<QString, QVariant>> data = {
                {FieldPair("UNIQUE", 1),     FieldPair("NULL", "value 1"),   FieldPair("INTEGER", 101)},
                {FieldPair("UNIQUE", 2),     FieldPair("NULL", "value 2"),   FieldPair("INTEGER", 201)}
        };

        SQLInsertCommand insCmd("IF", "UNIQUE", data);

        for (int i = 0 ; i < 3 ; i++) {
            insCmd.commit();

            dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM %1").arg(dbw->escapeIdentifier("IF")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            insCmd.revert();

            dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM %1").arg(dbw->escapeIdentifier("IF")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 0);
        }

        insCmd.commit();
    }

    {
        SQLUpdateCommand updCmd("IF", "UNIQUE", 2);
        updCmd.addFieldValue("NULL", "new value 2");
        updCmd.addFieldValue("INTEGER", 210);

        for (int i = 0 ; i < 3 ; i++) {
            updCmd.commit();

            dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM %1").arg(dbw->escapeIdentifier("IF")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            dbw->execAndCheckQuery(query, QString("SELECT * FROM %1 WHERE %2=2")
                    .arg(dbw->escapeIdentifier("IF"), dbw->escapeIdentifier("UNIQUE")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value("NULL"), "new value 2");
            EXPECT_EQ(query.value("INTEGER"), 210);

            updCmd.revert();

            dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM %1").arg(dbw->escapeIdentifier("IF")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);

            dbw->execAndCheckQuery(query, QString("SELECT * FROM %1 WHERE %2=2")
                    .arg(dbw->escapeIdentifier("IF"), dbw->escapeIdentifier("UNIQUE")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value("NULL"), "value 2");
            EXPECT_EQ(query.value("INTEGER"), 201);
        }
    }

    {
        SQLDeleteCommand delCmd("IF", "UNIQUE", QList<dbid_t>{2});

        for (int i = 0 ; i < 3 ; i++) {
            delCmd.commit();

            dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM %1").arg(dbw->escapeIdentifier("IF")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 1);

            delCmd.revert();

            dbw->execAndCheckQuery(query, QString("SELECT COUNT(*) FROM %1").arg(dbw->escapeIdentifier("IF")));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);
        }
    }
}

TEST_F(CommandTest, EditStack)
{
    using FieldPair = std::pair<QString, QVariant>;

    QSqlQuery query(db);

    dbw->execAndCheckQuery(query, QString(
            "CREATE TABLE autotest_editstack1 (\n"
            "   %1,\n"
            "   a VARCHAR(64) NOT NULL\n"
            ")"
            ).arg(dbw->generateIntPrimaryKeyCode("an_id")));

    //EditStack* editStack = System::getInstance()->getEditStack();
    std::unique_ptr<EditStack> editStack = std::make_unique<EditStack>(2, 2);

    ASSERT_FALSE(editStack->canUndo());
    ASSERT_FALSE(editStack->canRedo());

    {
        QList<QMap<QString, QVariant>> data = {
                {FieldPair("an_id", 1),     FieldPair("a", "value 1")},
                {FieldPair("an_id", 2),     FieldPair("a", "value 2")},
                {FieldPair("an_id", 3),     FieldPair("a", "value 3")}
        };

        SQLInsertCommand* insCmd = new SQLInsertCommand("autotest_editstack1", "an_id", data);

        SQLUpdateCommand* updCmd = new SQLUpdateCommand("autotest_editstack1", "an_id", 2);
        updCmd->addFieldValue("a", "new value 2");

        SQLEditCommand* editCmd = new SQLEditCommand;
        editCmd->addSQLCommand(insCmd);
        editCmd->addSQLCommand(updCmd);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);

        editStack->push(editCmd);

        EXPECT_TRUE(editStack->canUndo());
        EXPECT_FALSE(editStack->canRedo());

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1 WHERE a LIKE 'new %'");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);

        for (int i = 0 ; i < 2 ; i++) {
            editStack->undo();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 0);

            EXPECT_FALSE(editStack->canUndo());
            EXPECT_TRUE(editStack->canRedo());

            editStack->redo();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 3);

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1 WHERE a LIKE 'new %'");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 1);

            EXPECT_TRUE(editStack->canUndo());
            EXPECT_FALSE(editStack->canRedo());
        }
    }

    {
        SQLDeleteCommand* delCmd = new SQLDeleteCommand("autotest_editstack1", "an_id", QList<dbid_t>{1});

        SQLEditCommand* editCmd = new SQLEditCommand;
        editCmd->addSQLCommand(delCmd);

        editStack->push(editCmd);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 2);

        for (int i = 0 ; i < 3 ; i++) {
            editStack->undo();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 3);

            editStack->redo();

            dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toInt(), 2);
        }
    }

    {
        EXPECT_TRUE(editStack->canUndo());
        EXPECT_FALSE(editStack->canRedo());

        editStack->undo();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        EXPECT_TRUE(editStack->canUndo());
        EXPECT_TRUE(editStack->canRedo());

        editStack->undo();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);

        EXPECT_FALSE(editStack->canUndo());
        EXPECT_TRUE(editStack->canRedo());

        editStack->redo();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        EXPECT_TRUE(editStack->canUndo());
        EXPECT_TRUE(editStack->canRedo());

        editStack->undo();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);

        EXPECT_FALSE(editStack->canUndo());
        EXPECT_TRUE(editStack->canRedo());

        editStack->redo();
        editStack->redo();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 2);

        EXPECT_TRUE(editStack->canUndo());
        EXPECT_FALSE(editStack->canRedo());
    }

    {
        QList<QMap<QString, QVariant>> data = {
                {FieldPair("an_id", 10),     FieldPair("a", "value 10")},
                {FieldPair("an_id", 11),     FieldPair("a", "value 11")}
        };

        SQLInsertCommand* insCmd = new SQLInsertCommand("autotest_editstack1", "an_id", data);

        SQLEditCommand* editCmd = new SQLEditCommand;
        editCmd->addSQLCommand(insCmd);

        editStack->push(editCmd);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 4);

        EXPECT_TRUE(editStack->canUndo());
        EXPECT_FALSE(editStack->canRedo());

        editStack->undo();
        editStack->undo();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        EXPECT_FALSE(editStack->canUndo());
        EXPECT_TRUE(editStack->canRedo());

        editStack->redo();

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 2);
    }

    {
        EXPECT_TRUE(editStack->canUndo());
        EXPECT_TRUE(editStack->canRedo());

        SQLDeleteCommand* delCmd = new SQLDeleteCommand("autotest_editstack1", "an_id", QList<dbid_t>{3});

        SQLEditCommand* editCmd = new SQLEditCommand;
        editCmd->addSQLCommand(delCmd);

        editStack->push(editCmd);

        EXPECT_TRUE(editStack->canUndo());
        EXPECT_FALSE(editStack->canRedo());

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_editstack1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);
    }
}


}
