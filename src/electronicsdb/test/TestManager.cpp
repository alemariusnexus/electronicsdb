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

#include "TestManager.h"

#include <cstdio>
#include <memory>
#include <QSqlDatabase>
#include <gtest/gtest.h>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/log.h>
#include "../db/SQLDatabaseWrapperFactory.h"
#include "../exception/SQLException.h"
#include "../System.h"

namespace electronicsdb
{


TestManager& TestManager::getInstance()
{
    static TestManager inst;
    return inst;
}



TestManager::TestManager()
{
}

void TestManager::setTestConfigPath(const QString& cfgPath)
{
    testCfgPath = cfgPath;
}

int TestManager::runTests()
{
    int res = 1;

    try {
        testCfg = new QSettings(testCfgPath, QSettings::IniFormat);

        res = RUN_ALL_TESTS();

        closeTestDatabase();

        delete testCfg;
    } catch (std::exception& ex) {
        fprintf(stderr, "ERROR: Exception caught at test top-level: %s\n", ex.what());
    }

    return res;
}

void TestManager::openTestDatabase(const QString& suffix)
{
    openTestDatabase(suffix, false);
}

void TestManager::openFreshTestDatabase(const QString& suffix)
{
    openTestDatabase(suffix, true);
}

void TestManager::openTestDatabase(const QString& suffix, bool forceNew)
{
    System& sys = *System::getInstance();

    if (!forceNew  &&  QSqlDatabase::database().isValid()  &&  curDBSuffix == suffix) {
        // Keep using the current connection
        return;
    }

    closeTestDatabase();

    bool alreadyKnown = knownDBSuffices.contains(suffix);

    testCfg->beginGroup("db");

    try {
        DatabaseConnection* conn = sys.loadDatabaseConnectionFromSettings(*testCfg, suffix);
        if (!conn) {
            throw InvalidStateException("Error creating database connection", __FILE__, __LINE__);
        }

        QString pw;

        if (conn->needsPassword()) {
            pw = testCfg->value("pw").toString();
        }

        std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(conn->getDriverName()));

        if (!alreadyKnown  ||  forceNew) {
            // Drop old database only in force-mode, or when it's opened the first time in a test run.
            dbw->dropDatabaseIfExists(conn, pw);
        }

        dbw->createDatabaseIfNotExists(conn, pw);

        auto sniffToken = evtSniffer.startSniffing();

        if (!sys.connectDatabase(conn, pw, false)) {
            throw SQLException("Error connecting to database", __FILE__, __LINE__);
        }

        ASSERT_EQ(evtSniffer.getConnectionAboutToChangeData().size(), 1);
        ASSERT_EQ(evtSniffer.getConnectionChangedData().size(), 1);
        EXPECT_FALSE(evtSniffer.getConnectionAboutToChangeData()[0].oldConn);
        EXPECT_TRUE(evtSniffer.getConnectionAboutToChangeData()[0].newConn);
        EXPECT_FALSE(evtSniffer.getConnectionChangedData()[0].oldConn);
        EXPECT_TRUE(evtSniffer.getConnectionChangedData()[0].newConn);
        sniffToken.reset();

        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isValid()) {
            throw SQLException("Error connecting to database: is invalid", __FILE__, __LINE__);
        }

        delete conn;
    } catch (std::exception&) {
        testCfg->endGroup();
        throw;
    }

    testCfg->endGroup();

    knownDBSuffices.insert(suffix);
    curDBSuffix = suffix;
}

void TestManager::closeTestDatabase()
{
    System& sys = *System::getInstance();

    if (QSqlDatabase::database().isValid()) {
        // Close old connection

        auto sniffToken = evtSniffer.startSniffing();

        sys.disconnect();

        ASSERT_EQ(evtSniffer.getConnectionAboutToChangeData().size(), 1);
        ASSERT_EQ(evtSniffer.getConnectionChangedData().size(), 1);
        EXPECT_TRUE(evtSniffer.getConnectionAboutToChangeData()[0].oldConn);
        EXPECT_FALSE(evtSniffer.getConnectionAboutToChangeData()[0].newConn);
        EXPECT_TRUE(evtSniffer.getConnectionChangedData()[0].oldConn);
        EXPECT_FALSE(evtSniffer.getConnectionChangedData()[0].newConn);
    }
}


}
