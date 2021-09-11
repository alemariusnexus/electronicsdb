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

#include "test_global.h"

#include <memory>
#include <QList>
#include <QObject>
#include "../model/container/PartContainer.h"
#include "../model/part/Part.h"
#include "../System.h"

namespace electronicsdb
{


class TestEventSniffer : public QObject
{
    Q_OBJECT

public:
    struct DatabaseConnectionAboutToChangeData
    {
        DatabaseConnection* oldConn;
        DatabaseConnection* newConn;
    };
    struct DatabaseConnectionChangedData
    {
        DatabaseConnection* oldConn;
        DatabaseConnection* newConn;
    };

    struct AppModelAboutToBeResetData {};
    struct AppModelResetData {};

    struct ContainersChangedData
    {
        PartContainerList inserted;
        PartContainerList updated;
        PartContainerList deleted;
    };
    struct PartsChangedData
    {
        PartList inserted;
        PartList updated;
        PartList deleted;
    };

public:
    struct SniffTokenData
    {
        SniffTokenData(TestEventSniffer* sniffer) : sniffer(sniffer) { sniffer->doStartSniffing(); }
        ~SniffTokenData() { sniffer->doStopSniffing(); }

        TestEventSniffer* sniffer;
    };

    using SniffToken = std::shared_ptr<SniffTokenData>;

public:
    TestEventSniffer();

    SniffToken startSniffing();

    void clear();

    QList<DatabaseConnectionAboutToChangeData> getConnectionAboutToChangeData() const { return dbConnATCData; }
    QList<DatabaseConnectionChangedData> getConnectionChangedData() const { return dbConnCData; }

    QList<AppModelAboutToBeResetData> getAppModelAboutToBeResetData() const { return appModelATBRData; }
    QList<AppModelResetData> getAppModelResetData() const { return appModelRData; }

    QList<ContainersChangedData> getContainersChangedData() const { return contsChangedData; }
    QList<PartsChangedData> getPartsChangedData() const { return partsChangedData; }

    void expectContainersChangedData (
            const PartContainerList& ins,
            const PartContainerList& upd,
            const PartContainerList& del,
            int index = 0
            );
    void expectNoContainersChangedData();

    void expectPartsChangedData (
            const PartList& ins,
            const PartList& upd,
            const PartList& del,
            int index = 0
            );
    void expectNoPartsChangedData();

private:
    void doStartSniffing();
    void doStopSniffing();

private slots:
    void databaseConnectionAboutToChange(DatabaseConnection* oldConn, DatabaseConnection* newConn);
    void databaseConnectionChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);

    void appModelAboutToBeReset();
    void appModelReset();

    void containersChanged (
            const PartContainerList& inserted,
            const PartContainerList& updated,
            const PartContainerList& deleted
            );
    void partsChanged (
            const PartList& inserted,
            const PartList& updated,
            const PartList& deleted
            );

private:
    std::weak_ptr<SniffTokenData> weakSniffToken;
    bool sniffing;

    QList<DatabaseConnectionAboutToChangeData> dbConnATCData;
    QList<DatabaseConnectionChangedData> dbConnCData;

    QList<AppModelAboutToBeResetData> appModelATBRData;
    QList<AppModelResetData> appModelRData;

    QList<ContainersChangedData> contsChangedData;
    QList<PartsChangedData> partsChangedData;
};


}
