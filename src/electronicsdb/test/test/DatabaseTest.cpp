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
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <gtest/gtest.h>
#include <nxcommon/log.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../db/DatabaseConnection.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../TestManager.h"

namespace electronicsdb
{


class DatabaseTest : public ::testing::Test
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


TEST_F(DatabaseTest, Settings)
{
    {
        QMap<QString, QString> s = dbw->queryDatabaseSettings();

        EXPECT_TRUE(s.contains("db_version"));
        EXPECT_EQ(SQLDatabaseWrapper::programDbVersion, s["db_version"].toInt());

        EXPECT_FALSE(s.contains("autotest_token"));
    }

    {
        QMap<QString, QString> s;
        s["autotest_token"] = "Hello World!";

        dbw->writeDatabaseSettings(s);
    }

    {
        QMap<QString, QString> s = dbw->queryDatabaseSettings();

        EXPECT_TRUE(s.contains("autotest_token"));
        EXPECT_EQ(s["autotest_token"], QString("Hello World!"));
    }
}

TEST_F(DatabaseTest, PrimaryKey)
{
    {
        QSqlQuery query(db);

        dbw->execAndCheckQuery(query, QString("CREATE TABLE autotest_primarykey (%1, v TEXT)")
                .arg(dbw->generateIntPrimaryKeyCode("id")));

        dbw->execAndCheckQuery(query, "INSERT INTO autotest_primarykey (v) VALUES ('one'), ('two'), ('three')");

        dbw->execAndCheckQuery(query, "SELECT DISTINCT id FROM autotest_primarykey");

        EXPECT_TRUE(query.next());

        dbid_t firstID = VariantToDBID(query.value(0));

        EXPECT_TRUE(query.next());
        EXPECT_TRUE(query.next());
        EXPECT_FALSE(query.next());

        EXPECT_THROW({
            dbw->execAndCheckQuery(query, QString("INSERT INTO autotest_primarykey (id, v) VALUES (%1, 'conflict')")
                    .arg(firstID));
        }, SQLException);
    }
}

TEST_F(DatabaseTest, Upsert)
{
    {
        QSqlQuery query(db);

        dbw->execAndCheckQuery(query, QString("CREATE TABLE autotest_upsert (k VARCHAR(64) PRIMARY KEY, v TEXT)"));

        dbw->execAndCheckQuery(query, QString(
                "INSERT INTO autotest_upsert (k, v) VALUES ('key1', 'value1') %1 v='value1'"
                ).arg(dbw->generateUpsertPrefixCode(QStringList{"k"})));
        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_upsert WHERE v='value1'");
        query.next();
        EXPECT_EQ(query.value(0).toUInt(), 1);

        dbw->execAndCheckQuery(query, QString(
                "INSERT INTO autotest_upsert (k, v) VALUES ('key2', 'value2') %1 v='value2'"
                ).arg(dbw->generateUpsertPrefixCode(QStringList{"k"})));
        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_upsert WHERE v='value2'");
        query.next();
        EXPECT_EQ(query.value(0).toUInt(), 1);

        EXPECT_THROW(dbw->execAndCheckQuery(query, QString(
                "INSERT INTO autotest_upsert (k, v) VALUES ('key1', 'value1_new')"
                )), SQLException);
        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_upsert WHERE v='value1'");
        query.next();
        EXPECT_EQ(query.value(0).toUInt(), 1);

        dbw->execAndCheckQuery(query, QString(
                "INSERT INTO autotest_upsert (k, v) VALUES ('key1', 'value1_new') %1 v='value1_new'"
                ).arg(dbw->generateUpsertPrefixCode(QStringList{"k"})));
        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_upsert WHERE v='value1_new'");
        query.next();
        EXPECT_EQ(query.value(0).toUInt(), 1);
    }
}

TEST_F(DatabaseTest, GroupOps)
{
    QSqlQuery query(db);

    {
        dbw->execAndCheckQuery(query, "CREATE TABLE autotest_grouptest1 (k INTEGER, count INTEGER)");

        dbw->execAndCheckQuery(query, QString(
                "INSERT INTO autotest_grouptest1 (k, count) "
                "VALUES (1, 3), (2, 2), (3, 7), (3, 1), (4, 9), (1, 1), (2, 4), (1, 2)"
                ));

        dbw->execAndCheckQuery(query, QString(
                "SELECT k, %1 FROM autotest_grouptest1 GROUP BY k ORDER BY k ASC"
                ).arg(dbw->groupConcatCode("count", "/")));

        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);
        EXPECT_EQ(query.value(1).toString(), QString("3/1/2"));

        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 2);
        EXPECT_EQ(query.value(1).toString(), QString("2/4"));

        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);
        EXPECT_EQ(query.value(1).toString(), QString("7/1"));

        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 4);
        EXPECT_EQ(query.value(1).toString(), QString("9"));

        EXPECT_FALSE(query.next());
    }

    {
        dbw->execAndCheckQuery(query, "CREATE TABLE autotest_grouptest2 (k INTEGER, value TEXT)");

        dbw->execAndCheckQuery(query, QString(
                "INSERT INTO autotest_grouptest2 (k, value) "
                "VALUES (1, 'one'), (2, 'two,three'), (1, 'some''stuff'), (3, 420), (2, '{\"pseudo\":\"json\"}')"
                ));

        dbw->execAndCheckQuery(query, QString(
                "SELECT k, %1 FROM autotest_grouptest2 GROUP BY k ORDER BY k ASC"
                ).arg(dbw->groupJsonArrayCode("value")));

        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);
        {
            QJsonDocument jdoc = QJsonDocument::fromJson(query.value(1).toString().toUtf8());
            EXPECT_FALSE(jdoc.isNull());
            EXPECT_TRUE(jdoc.isArray());
            QJsonArray jarr = jdoc.array();
            EXPECT_EQ(jarr.size(), 2);
            EXPECT_TRUE(jarr[0].isString());
            EXPECT_TRUE(jarr[1].isString());
            EXPECT_EQ(jarr[0].toString(), "one");
            EXPECT_EQ(jarr[1].toString(), "some'stuff");
        }

        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 2);
        {
            QJsonDocument jdoc = QJsonDocument::fromJson(query.value(1).toString().toUtf8());
            EXPECT_FALSE(jdoc.isNull());
            EXPECT_TRUE(jdoc.isArray());
            QJsonArray jarr = jdoc.array();
            EXPECT_EQ(jarr.size(), 2);
            EXPECT_TRUE(jarr[0].isString());
            EXPECT_TRUE(jarr[1].isString());
            EXPECT_EQ(jarr[0].toString(), "two,three");
            EXPECT_EQ(jarr[1].toString(), "{\"pseudo\":\"json\"}");
        }

        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);
        {
            QJsonDocument jdoc = QJsonDocument::fromJson(query.value(1).toString().toUtf8());
            EXPECT_FALSE(jdoc.isNull());
            EXPECT_TRUE(jdoc.isArray());
            QJsonArray jarr = jdoc.array();
            EXPECT_EQ(jarr.size(), 1);
            EXPECT_TRUE(jarr[0].isString());
            EXPECT_EQ(jarr[0].toString(), "420");
        }
    }
}

TEST_F(DatabaseTest, List)
{
    QSqlQuery query(db);

    {
        dbw->execAndCheckQuery("CREATE TABLE autotest_listtest1 (col_a INTEGER, col_b TEXT DEFAULT 'some value')");
        dbw->execAndCheckQuery("CREATE TABLE autotest_listtest2 (col_c INTEGER NOT NULL DEFAULT 1337)");

        QStringList tables = dbw->listTables();
        EXPECT_TRUE(tables.contains("autotest_listtest1"));
        EXPECT_TRUE(tables.contains("autotest_listtest2"));
        EXPECT_FALSE(tables.contains("autotest_listtest3"));

        auto cols1 = dbw->listColumns("autotest_listtest1", true);
        EXPECT_EQ(cols1.size(), 2);
        {
            auto colA = cols1[0];
            EXPECT_EQ(colA["name"].toString(), QString("col_a"));
            // TODO: In the following line, the toString() is necessary before isNull() in Qt6, because QVariant does
            //       NOT report null if the contained value is null. That's fucked and might have consequences in other
            //       parts of the code. Find a way to work around this.
            // See: https://doc.qt.io/qt-6/qtcore-changes-qt6.html#the-qvariant-class
            EXPECT_TRUE(colA["default"].toString().isNull());

            auto colB = cols1[1];
            EXPECT_EQ(colB["name"].toString(), QString("col_b"));
            EXPECT_EQ(colB["default"].toString(), QString("some value"));
        }

        auto cols2 = dbw->listColumns("autotest_listtest2", true);
        EXPECT_EQ(cols2.size(), 1);
        {
            auto colC = cols2[0];
            EXPECT_EQ(colC["name"].toString(), QString("col_c"));
            EXPECT_EQ(colC["default"].toInt(), 1337);
        }
    }

    {
        dbw->createRowTrigger("autotest_listtest1_trigger", "autotest_listtest1", SQLDatabaseWrapper::SQLInsert,
                              "    DELETE FROM autotest_listtest2 WHERE 1=0;\n");

        auto triggers = dbw->listTriggers();

        int triggerIdx = -1;
        for (int i = 0 ; i < triggers.size() ; i++) {
            if (triggers[i]["name"] == "autotest_listtest1_trigger") {
                triggerIdx = i;
                break;
            }
        }

        EXPECT_GE(triggerIdx, 0);

        if (triggerIdx >= 0) {
            EXPECT_EQ(triggers[triggerIdx]["name"].toString(), QString("autotest_listtest1_trigger"));
            EXPECT_EQ(triggers[triggerIdx]["table"].toString(), QString("autotest_listtest1"));
        }
    }
}

TEST_F(DatabaseTest, Escape)
{
    QSqlQuery query(db);

    {
        dbw->execAndCheckQuery(query, "CREATE TABLE autotest_esctest (id INTEGER, v TEXT)");

        dbw->execAndCheckQuery(query, QString(
                "INSERT INTO autotest_esctest (id, v) VALUES (0, %1), (1, %2), (2, %3), (3, %4)"
                ).arg(dbw->escapeString("Hello World"),
                      dbw->escapeString("'String with quotes'"),
                      dbw->escapeString("String with ' one quote"),
                      dbw->escapeString("String with '' two quotes"))
                );

        dbw->execAndCheckQuery(query, "SELECT v FROM autotest_esctest WHERE id=0");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toString(), QString("Hello World"));

        dbw->execAndCheckQuery(query, "SELECT v FROM autotest_esctest WHERE id=1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toString(), QString("'String with quotes'"));

        dbw->execAndCheckQuery(query, "SELECT v FROM autotest_esctest WHERE id=2");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toString(), QString("String with ' one quote"));

        dbw->execAndCheckQuery(query, "SELECT v FROM autotest_esctest WHERE id=3");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toString(), QString("String with '' two quotes"));

        EXPECT_NE(dbw->escapeString("4"), QString("4"));
    }
}

TEST_F(DatabaseTest, InsertMulti)
{
    QSqlQuery query(db);

    {
        dbw->execAndCheckQuery(query, QString("CREATE TABLE autotest_insmulti (%1, v TEXT)")
                .arg(dbw->generateIntPrimaryKeyCode("id")));

        dbw->execAndCheckQuery(query, "INSERT INTO autotest_insmulti (id, v) VALUES (10, 'fixed ID 3'), (13, 'fixed ID 5')");

        QList<dbid_t> ids;
        dbw->insertMultipleWithReturnID(QString(
                "INSERT INTO autotest_insmulti (v) VALUES ('auto ID A'), ('auto ID B'), ('auto ID C'), ('auto ID D')"
                ),
                "id", ids);

        EXPECT_EQ(ids.size(), 4);

        dbw->execAndCheckQuery(query, "SELECT COUNT(DISTINCT(id)) FROM autotest_insmulti");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 6);

        QMap<dbid_t, QString> expValues;
        expValues[10] = "fixed ID 3";
        expValues[13] = "fixed ID 5";
        expValues[ids[0]] = "auto ID A";
        expValues[ids[1]] = "auto ID B";
        expValues[ids[2]] = "auto ID C";
        expValues[ids[3]] = "auto ID D";

        for (auto it = expValues.begin() ; it != expValues.end() ; it++) {
            dbw->execAndCheckQuery(query, QString("SELECT v FROM autotest_insmulti WHERE id=%1").arg(it.key()));
            EXPECT_TRUE(query.next());
            EXPECT_EQ(query.value(0).toString(), it.value());
            EXPECT_FALSE(query.next());
        }
    }
}

TEST_F(DatabaseTest, TruncateTable)
{
    QSqlQuery query(db);

    {
        dbw->execAndCheckQuery(query, "CREATE TABLE autotest_trunc1 (v TEXT)");
        dbw->execAndCheckQuery(query, "CREATE TABLE autotest_trunc2 (v TEXT)");

        dbw->execAndCheckQuery(query, "INSERT INTO autotest_trunc1 (v) VALUES ('one'), ('two'), ('three')");
        dbw->execAndCheckQuery(query, "INSERT INTO autotest_trunc2 (v) VALUES ('four')");

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_trunc1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 3);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_trunc2");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);

        dbw->truncateTable("autotest_trunc1");

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_trunc1");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 0);

        dbw->execAndCheckQuery(query, "SELECT COUNT(*) FROM autotest_trunc2");
        EXPECT_TRUE(query.next());
        EXPECT_EQ(query.value(0).toInt(), 1);
    }
}


}
