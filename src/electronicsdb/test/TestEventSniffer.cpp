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

#include "TestEventSniffer.h"

#include <nxcommon/log.h>
#include "../model/container/PartContainerFactory.h"
#include "../model/part/PartFactory.h"

namespace electronicsdb
{


TestEventSniffer::TestEventSniffer()
        : sniffing(false)
{
    System* sys = System::getInstance();
    PartContainerFactory* contFact = &PartContainerFactory::getInstance();
    PartFactory* partFact = &PartFactory::getInstance();

    connect(sys, &System::databaseConnectionAboutToChange, this, &TestEventSniffer::databaseConnectionAboutToChange);
    connect(sys, &System::databaseConnectionChanged, this, &TestEventSniffer::databaseConnectionChanged);
    connect(sys, &System::appModelAboutToBeReset, this, &TestEventSniffer::appModelAboutToBeReset);
    connect(sys, &System::appModelReset, this, &TestEventSniffer::appModelReset);

    connect(contFact, &PartContainerFactory::containersChanged, this, &TestEventSniffer::containersChanged);
    connect(partFact, &PartFactory::partsChanged, this, &TestEventSniffer::partsChanged);
}

TestEventSniffer::SniffToken TestEventSniffer::startSniffing()
{
    SniffToken token = weakSniffToken.lock();
    if (!token) {
        token = std::make_shared<SniffTokenData>(this);
        weakSniffToken = token;
    }
    return token;
}

void TestEventSniffer::doStartSniffing()
{
    clear();
    sniffing = true;
}

void TestEventSniffer::doStopSniffing()
{
    sniffing = false;
    clear();
}

void TestEventSniffer::clear()
{
    dbConnATCData.clear();
    dbConnCData.clear();

    appModelATBRData.clear();
    appModelRData.clear();

    contsChangedData.clear();
    partsChangedData.clear();
}

void TestEventSniffer::expectContainersChangedData (
        const PartContainerList& ins,
        const PartContainerList& upd,
        const PartContainerList& del,
        int index
) {
    ASSERT_GT(contsChangedData.size(), index) << "Missing containers changed event with index " << index;

    ContainersChangedData& d = contsChangedData[index];

    auto buildDescription = [&]() {
        QString insStr;
        QString updStr;
        QString delStr;

        for (const PartContainer& dc : d.inserted) {
            insStr += (!insStr.isEmpty() ? ", " : QString()) + QString::number(dc.getID());
        }
        for (const PartContainer& dc : d.updated) {
            updStr += (!updStr.isEmpty() ? ", " : QString()) + QString::number(dc.getID());
        }
        for (const PartContainer& dc : d.deleted) {
            delStr += (!delStr.isEmpty() ? ", " : QString()) + QString::number(dc.getID());
        }

        return QString("ins: %1,   upd: %2,   del: %3,   #cd: %4").arg(insStr, updStr, delStr)
                .arg(contsChangedData.size());
    };

    ASSERT_EQ(d.inserted.size(), ins.size())
            << "Wrong number of container insert events (" << buildDescription() << ")";
    ASSERT_EQ(d.updated.size(), upd.size())
            << "Wrong number of container update events (" << buildDescription() << ")";
    ASSERT_EQ(d.deleted.size(), del.size())
            << "Wrong number of container delete events (" << buildDescription() << ")";;

    auto expectContains = [&](const PartContainerList& l, const PartContainer& c) {
        QString evtStr;
        if (&l == &d.inserted) {
            evtStr = "insert";
        } else if (&l == &d.updated) {
            evtStr = "update";
        } else {
            evtStr = "delete";
        }

        EXPECT_TRUE(l.contains(c)) << "Missing " << evtStr << " event for container " << c.getID()
                << " (" << buildDescription() << ")";
    };

    for (const PartContainer& c : ins) {
        expectContains(d.inserted, c);
    }
    for (const PartContainer& c : upd) {
        expectContains(d.updated, c);
    }
    for (const PartContainer& c : del) {
        expectContains(d.deleted, c);
    }
}

void TestEventSniffer::expectNoContainersChangedData()
{
    ASSERT_EQ(contsChangedData.size(), 0);
}

void TestEventSniffer::expectPartsChangedData (
        const PartList& ins,
        const PartList& upd,
        const PartList& del,
        int index
) {
    ASSERT_GT(partsChangedData.size(), index) << "Missing parts changed event with index " << index;

    PartsChangedData& d = partsChangedData[index];

    auto buildDescription = [&]() {
        QString insStr;
        QString updStr;
        QString delStr;

        for (const Part& dp : d.inserted) {
            insStr += (!insStr.isEmpty() ? ", " : QString()) + QString("%1.%2")
                    .arg(dp.getPartCategory()->getID()).arg(dp.getID());
        }
        for (const Part& dp : d.updated) {
            updStr += (!updStr.isEmpty() ? ", " : QString()) + QString("%1.%2")
                    .arg(dp.getPartCategory()->getID()).arg(dp.getID());
        }
        for (const Part& dp : d.deleted) {
            delStr += (!delStr.isEmpty() ? ", " : QString()) + QString("%1.%2")
                    .arg(dp.getPartCategory()->getID()).arg(dp.getID());
        }

        return QString("ins: %1,   upd: %2,   del: %3,   #cd: %4").arg(insStr, updStr, delStr)
                .arg(partsChangedData.size());
    };

    ASSERT_EQ(d.inserted.size(), ins.size())
            << "Wrong number of part insert events (" << buildDescription() << ")";
    ASSERT_EQ(d.updated.size(), upd.size())
            << "Wrong number of part update events (" << buildDescription() << ")";
    ASSERT_EQ(d.deleted.size(), del.size())
            << "Wrong number of part delete events (" << buildDescription() << ")";

    auto expectContains = [&](const PartList& l, const Part& p) {
        QString evtStr;
        if (&l == &d.inserted) {
            evtStr = "insert";
        } else if (&l == &d.updated) {
            evtStr = "update";
        } else {
            evtStr = "delete";
        }

        EXPECT_TRUE(l.contains(p)) << "Missing " << evtStr << " event for part "
                        << p.getPartCategory()->getID() << "." << p.getID()
                        << " (" << buildDescription() << ")";
    };

    for (const Part& p : ins) {
        expectContains(d.inserted, p);
    }
    for (const Part& p : upd) {
        expectContains(d.updated, p);
    }
    for (const Part& p : del) {
        expectContains(d.deleted, p);
    }
}

void TestEventSniffer::expectNoPartsChangedData()
{
    ASSERT_EQ(partsChangedData.size(), 0);
}

void TestEventSniffer::databaseConnectionAboutToChange(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
    if (sniffing) {
        DatabaseConnectionAboutToChangeData d;
        d.oldConn = oldConn;
        d.newConn = newConn;
        dbConnATCData << d;
    }
}

void TestEventSniffer::databaseConnectionChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
    if (sniffing) {
        DatabaseConnectionChangedData d;
        d.oldConn = oldConn;
        d.newConn = newConn;
        dbConnCData << d;
    }
}


void TestEventSniffer::appModelAboutToBeReset()
{
    if (sniffing) {
        AppModelAboutToBeResetData d;
        appModelATBRData << d;
    }
}

void TestEventSniffer::appModelReset()
{
    if (sniffing) {
        AppModelResetData d;
        appModelRData << d;
    }
}

void TestEventSniffer::containersChanged (
        const PartContainerList& inserted,
        const PartContainerList& updated,
        const PartContainerList& deleted
)
{
    if (sniffing) {
        ContainersChangedData d;
        d.inserted = inserted;
        d.updated = updated;
        d.deleted = deleted;
        contsChangedData << d;
    }
}

void TestEventSniffer::partsChanged (
        const PartList& inserted,
        const PartList& updated,
        const PartList& deleted
)
{
    if (sniffing) {
        PartsChangedData d;
        d.inserted = inserted;
        d.updated = updated;
        d.deleted = deleted;
        partsChangedData << d;
    }
}


}
