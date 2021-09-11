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

#pragma once

#include "../global.h"

#include <QObject>
#include <QSet>
#include <QSettings>
#include <QString>
#include "TestEventSniffer.h"

namespace electronicsdb
{


class TestManager : public QObject
{
    Q_OBJECT

public:
    static TestManager& getInstance();

public:
    void setTestConfigPath(const QString& cfgPath);

    int runTests();

    QSettings& getSettings() { return *testCfg; }

    void openTestDatabase(const QString& suffix);
    void openFreshTestDatabase(const QString& suffix);
    void closeTestDatabase();

    TestEventSniffer& getEventSniffer() { return evtSniffer; }

private:
    TestManager();

    void openTestDatabase(const QString& suffix, bool forceNew);
    //void initTestDB();

private:
    QString testCfgPath;
    QSettings* testCfg;

    QSet<QString> knownDBSuffices;
    QString curDBSuffix;

    TestEventSniffer evtSniffer;
};


}
