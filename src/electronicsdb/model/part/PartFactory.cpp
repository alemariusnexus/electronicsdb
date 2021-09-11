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

#include "PartFactory.h"

#include <memory>
#include <unordered_map>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../container/PartContainerFactory.h"
#include "../PartCategory.h"

namespace electronicsdb
{


PartFactory& PartFactory::getInstance()
{
    static PartFactory inst;
    return inst;
}

PartFactory::PartFactory()
{
    connect(System::getInstance(), &System::appModelAboutToBeReset, this, &PartFactory::appModelAboutToBeReset);
}

void PartFactory::registerPart(const Part& part)
{
    PartCategory* pcat = const_cast<PartCategory*>(part.getPartCategory());
    assert(part  &&  part.hasID());
    assert(partRegistry[pcat].find(part.getID()) == partRegistry[pcat].end());
    partRegistry[pcat][part.getID()] = Part::WeakPtr(part);
}

void PartFactory::unregisterPart(PartCategory* pcat, dbid_t pid)
{
    assert(pcat  &&  pid >= 0);
    partRegistry[pcat].erase(pid);
}

Part PartFactory::findPartByID(PartCategory* pcat, dbid_t pid)
{
    auto& preg = partRegistry[pcat];
    auto it = preg.find(pid);
    if (it != preg.end()) {
        Part part = it->second.lock();
        if (part) {
            return part;
        }
    }
    Part newPart(pcat, pid);
    preg[pid] = Part::WeakPtr(newPart);
    return newPart;
}

PartList PartFactory::findPartsByID(PartCategory* pcat, const QList<dbid_t>& pids)
{
    PartList parts;
    auto& preg = partRegistry[pcat];
    for (dbid_t pid : pids) {
        auto it = preg.find(pid);
        if (it != preg.end()) {
            Part part = it->second.lock();
            if (part) {
                parts << part;
                continue;
            }
        }

        Part newPart(pcat, pid);
        preg[pid] = newPart;
        parts << newPart;
    }
    return parts;
}

PartList PartFactory::loadItemsSingleCategory (
        PartCategory* pcat,
        const QString& whereClause,
        const QString& orderClause,
        const QString& limitClause,
        int flags,
        const PropertyList& props
) {
    PartList results;
    PartList dummy;
    loadItemsSingleCategory(pcat, whereClause, orderClause, limitClause, flags, props, dummy.end(), dummy.end(), &results);
    return results;
}

void PartFactory::processChangelog()
{
    checkDatabaseConnection();

    System* sys = System::getInstance();

    if (sys->isChangelogProcessingInhibited()) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, QString("SELECT ptype, pid, state FROM clog_pcat"),
                           "Error reading clog_pcat");

    PartList inserted;
    PartList updated;
    PartList deleted;

    bool emptyLog = true;

    while (query.next()) {
        Part part = findPartByID(sys->getPartCategory(query.value(0).toString()), VariantToDBID(query.value(1)));
        QString state = query.value(2).toString();

        if (state == "ins") {
            inserted << part;
        } else if (state == "upd") {
            updated << part;
        } else if (state == "del") {
            deleted << part;
        } else {
            assert(false);
        }

        emptyLog = false;
    }

    if (!updated.empty()) {
        for (Part& part : updated) {
            part.claimAssocs(false);
        }
        loadItems(updated.begin(), updated.end(), LoadReload);
    }

    if (!emptyLog) {
        truncateChangelog();

        emit partsChanged(inserted, updated, deleted);
    }
}

void PartFactory::truncateChangelog()
{
    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    dbw->truncateTable("clog_pcat");
}

QMimeData* PartFactory::encodePartListToMimeData(const PartList& parts, const QUuid& uuid)
{
    QMap<PartCategory*, PartList> partsByCat;

    for (const Part& part : parts) {
        partsByCat[const_cast<PartCategory*>(part.getPartCategory())] << part;
    }

    QJsonObject jroot;
    jroot.insert("uuid", uuid.toString(QUuid::WithBraces));

    QJsonArray jpcats;

    for (auto it = partsByCat.begin() ; it != partsByCat.end() ; it++) {
        PartCategory* pcat = it.key();

        QJsonObject jpcat;
        jpcat.insert("partCategory", pcat->getID());

        QJsonArray jparts;
        for (const Part& part : it.value()) {
            jparts << part.getID();
        }
        jpcat.insert("parts", jparts);

        jpcats << jpcat;
    }

    jroot.insert("partCategories", jpcats);

    QJsonDocument jdoc(jroot);
    QByteArray jsonData = jdoc.toJson(QJsonDocument::Compact);

    QMimeData* mimeData = new QMimeData;
    mimeData->setData(getPartListMimeDataType(), jsonData);

    return mimeData;
}

PartList PartFactory::decodePartListFromMimeData(const QMimeData* mimeData, bool* valid, QUuid* uuid) {
    if (!mimeData->hasFormat(getPartListMimeDataType())) {
        if (valid) {
            *valid = false;
        }
        return PartList();
    }

    QByteArray jsonData = mimeData->data(getPartListMimeDataType());
    QJsonDocument jdoc = QJsonDocument::fromJson(jsonData);

    if (jdoc.isNull()) {
        if (valid) {
            *valid = false;
        }
        return PartList();
    }

    PartList parts;

    QJsonObject jroot = jdoc.object();

    if (uuid) {
        if (jroot.contains("uuid")) {
            *uuid = QUuid::fromString(jroot.value("uuid").toString());
        }
    }

    QJsonArray jpcats = jroot.value("partCategories").toArray();

    for (const auto& jpcatVal : jpcats) {
        QJsonObject jpcat = jpcatVal.toObject();
        QString pcatID = jpcat.value("partCategory").toString();
        PartCategory* pcat = System::getInstance()->getPartCategory(pcatID);
        if (!pcat) {
            continue;
        }

        QJsonArray jparts = jpcat.value("parts").toArray();

        for (const auto& jpartVal : jparts) {
            dbid_t pid = static_cast<dbid_t>(jpartVal.toDouble());
            parts << PartFactory::getInstance().findPartByID(pcat, pid);
        }
    }

    if (valid) {
        *valid = true;
    }
    return parts;
}

void PartFactory::appModelAboutToBeReset()
{
    // TODO: This is dangerous, because other components listening to the appModelAboutToBeReset() signal might assume
    //       that the caches are still valid. This really needs to be executed after all other such components have
    //       finished. Doing it in appModelReset() leads to the opposite problem: Some components might get the signal
    //       sent earlier and start fetching cache entries, in which case we'll destroy their entries here.
    //       Same goes for the similar code in PartContainerFactory.
    partRegistry.clear();
}


}
