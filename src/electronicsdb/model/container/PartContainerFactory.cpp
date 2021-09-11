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

#include "PartContainerFactory.h"

#include <memory>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <nxcommon/log.h>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../PartCategory.h"

namespace electronicsdb
{


PartContainerFactory& PartContainerFactory::getInstance()
{
    static PartContainerFactory inst;
    return inst;
}



PartContainerFactory::PartContainerFactory()
{
    connect(System::getInstance(), &System::appModelAboutToBeReset,
            this, &PartContainerFactory::appModelAboutToBeReset);
}

void PartContainerFactory::registerContainer(const PartContainer& cont)
{
    assert(cont  &&  cont.hasID());
    assert(contRegistry.find(cont.getID()) == contRegistry.end());
    contRegistry[cont.getID()] = PartContainer::WeakPtr(cont);
}

void PartContainerFactory::unregisterContainer(dbid_t cid)
{
    assert(cid >= 0);
    contRegistry.erase(cid);
}

PartContainer PartContainerFactory::findContainerByID(dbid_t cid)
{
    auto it = contRegistry.find(cid);
    if (it != contRegistry.end()) {
        PartContainer cont = it->second.lock();
        if (cont) {
            return cont;
        }
    }
    PartContainer cont(cid);
    contRegistry[cid] = PartContainer::WeakPtr(cont);
    return cont;
}

PartContainerFactory::ContainerList PartContainerFactory::findContainersByID(const QList<dbid_t>& cids)
{
    ContainerList conts;
    for (dbid_t cid : cids) {
        conts << findContainerByID(cid);
    }
    return conts;
}

PartContainerFactory::ContainerList PartContainerFactory::loadItems (
        const QString& whereClause,
        const QString& orderClause,
        int flags
) {
    ContainerList dummy;
    ContainerList conts;
    loadItems(whereClause,
              orderClause,
              flags,
              dummy.end(), dummy.end(),
              &conts);
    return conts;
}

void PartContainerFactory::processChangelog()
{
    checkDatabaseConnection();

    System* sys = System::getInstance();

    if (sys->isChangelogProcessingInhibited()) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, QString("SELECT cid, state FROM clog_container"),
                           "Error reading clog_container");

    ContainerList inserted;
    ContainerList updated;
    ContainerList deleted;

    bool emptyLog = true;

    while (query.next()) {
        PartContainer cont = findContainerByID(VariantToDBID(query.value(0)));
        QString state = query.value(1).toString();

        if (state == "ins") {
            inserted << cont;
        } else if (state == "upd") {
            updated << cont;
        } else if (state == "del") {
            deleted << cont;
        } else {
            assert(false);
        }

        emptyLog = false;
    }

    // NOTE: We only have to reload data for updated containers:
    // For inserted containers, the only references that could possibly exist in memory are the ones from where the
    // values were just inserted. Obviously, they are up-to-date.
    // For deleted containers, there's currently no need to mark them as deleted or anything. Instead, clients should
    // listen for delete events and drop their references to them manually. We don't need to reload associated parts
    // here either, because the log triggers will mark them as updated automatically, so the PartFactory update logic
    // takes care of them.

    // Reload data for updated containers
    if (!updated.empty()) {
        for (PartContainer& cont : updated) {
            cont.claimAssocs(false);
        }
        loadItems(updated.begin(), updated.end(), LoadReload);
    }

    if (!emptyLog) {
        truncateChangelog();

        emit containersChanged(inserted, updated, deleted);
    }
}

void PartContainerFactory::truncateChangelog()
{
    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    dbw->truncateTable("clog_container");
}

void PartContainerFactory::appModelAboutToBeReset()
{
    contRegistry.clear();
}


}
