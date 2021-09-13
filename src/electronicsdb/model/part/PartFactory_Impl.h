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

#include "PartFactory_Def.h"

#include <functional>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include "../../command/sql/SQLDeleteCommand.h"
#include "../../command/sql/SQLInsertCommand.h"
#include "../../command/sql/SQLUpdateCommand.h"
#include "../../command/SQLEditCommand.h"
#include "../../util/elutil.h"
#include "../assoc/PartContainerAssoc.h"
#include "../assoc/PartLinkAssoc.h"
#include "../container/PartContainerFactory.h"

namespace electronicsdb
{


template <typename Iterator>
void PartFactory::loadItemsSingleCategory(Iterator beg, Iterator end, int flags, const PropertyList& props)
{
    if (beg == end) {
        return;
    }

    QSqlDatabase db = getSQLDatabase();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    Part& firstPart = *beg;
    PartCategory* pcat = firstPart.getPartCategory();

    QString whereInStr;
    for (auto it = beg ; it != end ; ++it) {
        const Part& part = *it;
        if (!whereInStr.isEmpty()) {
            whereInStr += ",";
        }
        whereInStr += QString::number(part.getID());
    }

    loadItemsSingleCategory (
            pcat,
            QString("WHERE _p.%1 IN (%2)").arg(dbw->escapeIdentifier(pcat->getIDField()), whereInStr),
            QString(),
            QString(),
            flags,
            props,
            beg, end,
            nullptr
            );
}

template <typename Iterator>
void PartFactory::loadItemsSingleCategory (
        PartCategory* pcat,
        const QString& whereClause,
        const QString& orderClause,
        const QString& limitClause,
        int flags,
        const QList<PartProperty*>& argProps,
        Iterator inBeg, Iterator inEnd,
        PartList* outList
) {
    checkDatabaseConnection();

    // Build part index
    QMap<dbid_t, Part*> partsByID;
    for (auto it = inBeg ; it != inEnd ; ++it) {
        Part& part = *it;
        assert(part);
        assert(part.hasID());
        assert(part.getPartCategory() == pcat);
        partsByID[part.getID()] = &part;
    }

    bool haveInputParts = !partsByID.empty();

    QList<PartProperty*> props = argProps;
    if (props.empty()) {
        props = pcat->getProperties();
    }

    bool mustLoadData = true;

    if ((flags & LoadReload) != 0) {
        if (haveInputParts) {
            for (auto it = inBeg ; it != inEnd ; ++it) {
                Part& part = *it;

                if (part.isContainerAssocsLoaded()) {
                    flags |= LoadContainers;
                }
                if (part.isLinkedPartAssocsLoaded()) {
                    flags |= LoadPartLinks;
                }
            }
        } else {
            flags |= LoadAll;
        }
    } else {
        if (haveInputParts) {
            // Check what data must actually be loaded. Unless LoadReload is set, we can skip already loaded stuff

            mustLoadData = false;
            int newFlags = (flags & ~(LoadContainers | LoadPartLinks));

            for (auto it = inBeg ; it != inEnd ; ++it) {
                Part& part = *it;

                if (!mustLoadData  &&  !part.isDataLoaded(props)) {
                    mustLoadData = true;
                }

                if (!part.isContainerAssocsLoaded()) {
                    newFlags |= LoadContainers;
                }
                if (!part.isLinkedPartAssocsLoaded()) {
                    newFlags |= LoadPartLinks;
                }
            }

            newFlags &= flags;
            flags = newFlags & flags;
        }
    }

    if (outList  &&  !orderClause.isEmpty()) {
        // We can't entirely skip loading, because we need the database to provide us with the correct ordering to fill
        // the out list, even if no data actually has to be loaded.
        mustLoadData = true;
    }

    if (!mustLoadData  &&  (flags & LoadContainers) == 0  &&  (flags & LoadPartLinks) == 0) {
        // Nothing to load!

        if (outList) {
            assert(orderClause.isEmpty());

            for (auto it = inBeg ; it != inEnd ; ++it) {
                *outList << *it;
            }
        }

        return;
    }

    System* sys = System::getInstance();
    PartContainerFactory& contFactory = PartContainerFactory::getInstance();

    QSqlDatabase db = getSQLDatabase();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);


    // Unfortunately, the following is best done in two queries, and the first one can be extremely inefficient when
    // multi-value properties with many values and many parts are involved. It's not trivial for the following reasons:
    //
    //      * We must support WHERE expressions like `WHERE peripheral='GPIO'`, with `peripheral` being a multi-value
    //        property. This should return all part that have 'GPIO' as one of the values in the property. To do this
    //        in a single WHERE expression, we must do a full join of pcat_* and pcatmv_*.
    //      * The first query can be rather slow and I'm not sure that including it as a subquery (or CTE) in the second
    //        query would guarantee that it's executed only once, across all supported DB systems. So we'll be safe and
    //        execute them separately

    // ********** STEP 1: Find matching IDs **********

    QString joinCode;

    // Build property query code
    for (PartProperty* prop : props) {
        if ((prop->getFlags() & PartProperty::MultiValue) != 0) {
            joinCode += QString("\n    LEFT JOIN %1 ON %1.%2=_p.%3")
                    .arg(dbw->escapeIdentifier(prop->getMultiValueForeignTableName()),
                         dbw->escapeIdentifier(prop->getMultiValueIDFieldName()),
                         pcat->getIDField());
        }
    }

    dbw->execAndCheckQuery(query, QString(
            "SELECT _p.%1\n"
            "FROM %2 _p"
            "%3"
            "%4\n"
            "GROUP BY _p.%1"
            "%5"
            "%6"
            ).arg(dbw->escapeIdentifier(pcat->getIDField()),
                  dbw->escapeIdentifier(pcat->getTableName()),
                  joinCode,
                  whereClause.isEmpty() ? "" : ("\n" + whereClause),
                  orderClause.isEmpty() ? "" : ("\n" + orderClause),
                  limitClause.isEmpty() ? "" : ("\n" + limitClause)
                  ));

    QList<dbid_t> pids;
    QStringList pidStrs;

    while (query.next()) {
        dbid_t pid = VariantToDBID(query.value(0));
        pids << pid;
        pidStrs << QString::number(pid);
    }

    if (pids.empty()) {
        return;
    }


    // ********** STEP 2: Collect all relevant data by ID **********

    QStringList selFields;

    // Add ID field
    selFields << QString("_p.%1").arg(dbw->escapeIdentifier(pcat->getIDField()));

    // Build property query code
    for (PartProperty* prop : props) {
        if ((prop->getFlags() & PartProperty::MultiValue) == 0) {
            selFields << QString("_p.%1").arg(dbw->escapeIdentifier(prop->getFieldName()));
        } else {
            QString jsonGroupCode = dbw->groupJsonArrayCode(dbw->jsonArrayCode({
                           QString("%1.idx").arg(dbw->escapeIdentifier(prop->getMultiValueForeignTableName())),
                           QString("%1.%2").arg(dbw->escapeIdentifier(prop->getMultiValueForeignTableName()),
                                                dbw->escapeIdentifier(prop->getFieldName()))
                   }));
            selFields << QString("(SELECT %1 %2 FROM %3 WHERE %3.%4=_p.%5) %2")
                    .arg(jsonGroupCode,
                         dbw->escapeIdentifier(prop->getFieldName()),
                         dbw->escapeIdentifier(prop->getMultiValueForeignTableName()),
                         dbw->escapeIdentifier(prop->getMultiValueIDFieldName()),
                         dbw->escapeIdentifier(pcat->getIDField()));
        }
    }

    // Build container assoc query code
    if ((flags & LoadContainers) != 0) {
        selFields << QString("(SELECT %1 _cid FROM container_part _c WHERE _c.ptype=%2 AND _c.pid=_p.%3)")
                .arg(dbw->groupConcatCode("_c.cid"),
                     dbw->escapeString(pcat->getID()),
                     dbw->escapeIdentifier(pcat->getIDField()));
    }

    // Build part link query code
    if ((flags & LoadPartLinks) != 0  &&  !pcat->getPartLinkTypes().empty()) {
        for (PartLinkType* ltype : pcat->getPartLinkTypes()) {
            if (ltype->isPartCategoryA(pcat)) {
                QString jsonGroupCode = dbw->groupJsonArrayCode(QString("_pl_%1.ptype_b||':'||_pl_%1.pid_b")
                        .arg(ltype->getID()));
                selFields << QString(
                        "(SELECT %1 _pld_%2 FROM partlink _pl_%2 "
                        "WHERE _pl_%2.type=%3 AND _pl_%2.ptype_a=%4 AND _pl_%2.pid_a=_p.%5)"
                        ).arg(jsonGroupCode,
                              ltype->getID(),
                              dbw->escapeString(ltype->getID()),
                              dbw->escapeString(pcat->getID()),
                              dbw->escapeIdentifier(pcat->getIDField()));
            } else {
                QString jsonGroupCode = dbw->groupJsonArrayCode(QString("_pl_%1.ptype_a||':'||_pl_%1.pid_a")
                                                                        .arg(ltype->getID()));
                selFields << QString(
                        "(SELECT %1 _pld_%2 FROM partlink _pl_%2 "
                        "WHERE _pl_%2.type=%3 AND _pl_%2.ptype_b=%4 AND _pl_%2.pid_b=_p.%5)"
                ).arg(jsonGroupCode,
                      ltype->getID(),
                      dbw->escapeString(ltype->getID()),
                      dbw->escapeString(pcat->getID()),
                      dbw->escapeIdentifier(pcat->getIDField()));
            }
        }
    }

    int outListStartIdx = -1;
    if (outList) {
        outListStartIdx = outList->size();
        int numPIDs = pids.size();
        for (int i = 0 ; i < numPIDs ; i++) {
            *outList << Part();
        }
    }

    dbw->execAndCheckQuery(query, QString(
            "SELECT %1\n"
            "FROM   %2 _p\n"
            "WHERE _p.%3 IN (%4)"
            ).arg(selFields.join(",\n       "),
                  dbw->escapeIdentifier(pcat->getTableName()),
                  dbw->escapeIdentifier(pcat->getIDField()),
                  pidStrs.join(',')
                  ),
            "Error loading parts");

    // There's exactly one result row for each matched part (multi-values and assocs are flattened)
    while (query.next()) {
        dbid_t pid = VariantToDBID(query.value(0));

        Part regPart = haveInputParts ? Part() : findPartByID(pcat, pid);
        Part& part = haveInputParts ? *partsByID[pid] : regPart;

        int fieldIdx = 1;

        // Build property data (single- & multi-value)
        Part::DataMap propData;
        for (PartProperty* prop : props) {
            if ((prop->getFlags() & PartProperty::MultiValue) == 0) {
                propData[prop] = query.value(fieldIdx);
            } else {
                QList<QVariant> mvValues;

                if (!query.isNull(fieldIdx)) {
                    QString mvJsonCode = query.value(fieldIdx).toString();

                    QJsonDocument jdoc = QJsonDocument::fromJson(mvJsonCode.toUtf8());
                    assert(jdoc.isArray());

                    QJsonArray jarr = jdoc.array();

                    std::vector<std::pair<QVariant, int>> mvValuesWithIdx;
                    mvValuesWithIdx.reserve(jarr.size());
                    for (const QJsonValueRef& jval : jarr) {
                        QJsonArray jentry = jval.toArray();
                        mvValuesWithIdx.emplace_back(jentry[1].toVariant(), jentry[0].toInt());
                    }

                    std::sort(mvValuesWithIdx.begin(), mvValuesWithIdx.end(), [](const auto& a, const auto& b) {
                        return a.second < b.second;
                    });

                    for (const auto& p : mvValuesWithIdx) {
                        mvValues << p.first;
                    }
                }

                propData[prop] = mvValues;
            }

            fieldIdx++;
        }

        part.setData(propData);
        part.clearDirty();

        // Build container assocs
        if ((flags & LoadContainers) != 0) {
            QList<PartContainer> conts;

            if (!query.isNull(fieldIdx)) {
                QString splitStr = query.value(fieldIdx).toString();
                for (QString cidStr : splitStr.split(',')) {
                    dbid_t cid = static_cast<dbid_t>(cidStr.toLongLong());
                    conts << contFactory.findContainerByID(cid);
                }
            }

            part.loadContainers(conts.begin(), conts.end());
            part.template clearAssocDirty<Part::PartContainerAssocIdx>();
            fieldIdx++;
        }

        // Build part link assocs
        if ((flags & LoadPartLinks) != 0  &&  !pcat->getPartLinkTypes().empty()) {
            QMap<Part, QSet<PartLinkType*>> linkedParts;

            for (PartLinkType* ltype : pcat->getPartLinkTypes()) {
                if (!query.isNull(fieldIdx)) {
                    QString jsonCode = query.value(fieldIdx).toString();

                    QJsonDocument jdoc = QJsonDocument::fromJson(jsonCode.toUtf8());
                    assert(jdoc.isArray());

                    for (const QJsonValueRef& jval : jdoc.array()) {
                        QString splitStr = jval.toString();
                        const auto splitParts = splitStr.split(':');
                        PartCategory* linkPcat = sys->getPartCategory(splitParts[0]);
                        dbid_t linkPID = static_cast<dbid_t>(splitParts[1].toLongLong());
                        Part linkedPart = findPartByID(linkPcat, linkPID);
                        linkedParts[linkedPart].insert(ltype);
                    }
                }

                fieldIdx++;
            }

            QList<Part> linkedPartsRaw = linkedParts.keys();
            part.loadLinkedParts(linkedPartsRaw.begin(), linkedPartsRaw.end());

            for (auto lit = linkedParts.begin() ; lit != linkedParts.end() ; ++lit) {
                const Part& linkedPart = lit.key();
                const QSet<PartLinkType*>& ltypes = lit.value();

                PartLinkAssoc linkAssoc = part.getLinkedPartAssoc(linkedPart);
                linkAssoc.setInhibitMarkDirty(true);
                linkAssoc.setLinkTypes(ltypes);
                linkAssoc.setInhibitMarkDirty(false);
            }

            part.template clearAssocDirty<Part::PartLinkAssocIdx>();
        }

        if (outList) {
            int partIdx = pids.indexOf(pid);
            assert(partIdx >= 0);

            (*outList)[outListStartIdx + partIdx] = part;
        }
    }
}

template <typename Iterator, typename PropFunc>
void PartFactory::loadItems (
        Iterator beg, Iterator end,
        int flags,
        const PropFunc& propFunc
) {
    QMap<PartCategory*, QList<std::reference_wrapper<Part>>> partsByCat;

    for (auto it = beg ; it != end ; ++it) {
        Part& part = *it;
        partsByCat[part.getPartCategory()] << part;
    }

    for (auto it = partsByCat.begin() ; it != partsByCat.end() ; it++) {
        PartCategory* pcat = it.key();
        auto& parts = it.value();

        PropertyList props = propFunc(pcat);
        loadItemsSingleCategory(parts.begin(), parts.end(), flags, props);
    }
}

template <typename Iterator>
EditCommand* PartFactory::insertItemsCmd(Iterator beg, Iterator end)
{
    if (beg == end) {
        return nullptr;
    }
    checkDatabaseConnection();

    QSqlDatabase db = getSQLDatabase();

    SQLEditCommand* editCmd = new SQLEditCommand;

    QMap<PartCategory*, QList<std::reference_wrapper<Part>>> partsByCat;
    QMap<PartCategory*, QList<QMap<QString, QVariant>>> baseDataByCat;

    for (auto it = beg ; it != end ; ++it) {
        Part& part = *it;
        assert(part);

        PartCategory* pcat = const_cast<PartCategory*>(part.getPartCategory());

        part.clearDirty();

        partsByCat[pcat] << part;

        QMap<QString, QVariant> d;
        if (part.hasID()) {
            d[pcat->getIDField()] = part.getID();
        }

        for (PartProperty* prop : pcat->getProperties()) {
            if ((prop->getFlags() & PartProperty::MultiValue) == 0) {
                if (part.isDataLoaded(prop)) {
                    d[prop->getFieldName()] = part.getData(prop);
                }
            }
        }

        baseDataByCat[pcat] << d;
    }

    for (auto it = baseDataByCat.begin() ; it != baseDataByCat.end() ; ++it) {
        PartCategory* pcat = it.key();
        auto& data = it.value();
        auto& parts = partsByCat[pcat];

        SQLInsertCommand* baseCmd = new SQLInsertCommand(pcat->getTableName(), pcat->getIDField(), data,
                                                         db.connectionName());

        addIDInsertListener(baseCmd, parts.begin(), parts.end(), [this, editCmd, pcat](auto beg, auto end) {
            applyMultiValueProps(editCmd, pcat, beg, end);
            applyAssocChanges(beg, end, editCmd, ApplyAssocTypeInsert);
        });

        editCmd->addSQLCommand(baseCmd);
    }

    return editCmd;
}

template <typename Iterator>
EditCommand* PartFactory::updateItemsCmd(Iterator beg, Iterator end)
{
    if (beg == end) {
        return nullptr;
    }
    checkDatabaseConnection();

    QSqlDatabase db = getSQLDatabase();

    SQLEditCommand* editCmd = new SQLEditCommand;

    QMap<PartCategory*, PartList> dirtyPartsByCat;

    for (auto it = beg ; it != end ; ++it) {
        const Part& part = *it;

        assert(part);
        assert(part.hasID());

        PartCategory* pcat = const_cast<PartCategory*>(part.getPartCategory());

        if (part.isDirty()) {
            SQLUpdateCommand* updCmd = new SQLUpdateCommand(pcat->getTableName(), pcat->getIDField(), part.getID(),
                                                            db.connectionName());

            for (PartProperty* prop : pcat->getProperties()) {
                if (!part.isDataLoaded(prop)) {
                    continue;
                }
                if ((prop->getFlags() & PartProperty::MultiValue) == 0) {
                    if (part.isDataLoaded(prop)) {
                        updCmd->addFieldValue(prop->getFieldName(), part.getData(prop));
                    }
                }
            }

            editCmd->addSQLCommand(updCmd);
        }

        dirtyPartsByCat[pcat] << part;

        part.clearDirty();
    }

    for (auto it = dirtyPartsByCat.begin() ; it != dirtyPartsByCat.end() ; ++it) {
        PartCategory* pcat = it.key();
        auto& parts = it.value();

        applyMultiValueProps(editCmd, pcat, parts.begin(), parts.end());
    }

    applyAssocChanges(beg, end, editCmd, ApplyAssocTypeUpdate);

    return editCmd;
}

template <typename Iterator>
EditCommand* PartFactory::deleteItemsCmd(Iterator beg, Iterator end)
{
    if (beg == end) {
        return nullptr;
    }
    checkDatabaseConnection();

    QSqlDatabase db = getSQLDatabase();

    SQLEditCommand* editCmd = new SQLEditCommand;

    QMap<PartCategory*, QList<dbid_t>> partIDsByCat;

    for (auto it = beg ; it != end ; ++it) {
        const Part& part = *it;
        PartCategory* pcat = const_cast<PartCategory*>(part.getPartCategory());

        if (!part.hasID()) {
            continue;
        }

        partIDsByCat[pcat] << part.getID();

        part.clearDirty();
    }

    applyAssocChanges(beg, end, editCmd, ApplyAssocTypeDelete);

    for (auto it = partIDsByCat.begin() ; it != partIDsByCat.end() ; ++it) {
        PartCategory* pcat = it.key();
        const auto& ids = it.value();

        for (PartProperty* prop : pcat->getProperties()) {
            if ((prop->getFlags() & PartProperty::MultiValue) != 0) {
                SQLDeleteCommand* mvDelCmd = new SQLDeleteCommand(prop->getMultiValueForeignTableName(),
                                                                  prop->getMultiValueIDFieldName(),
                                                                  ids,
                                                                  db.connectionName());
                editCmd->addSQLCommand(mvDelCmd);
            }
        }

        SQLDeleteCommand* delCmd = new SQLDeleteCommand(pcat->getTableName(), pcat->getIDField(), ids,
                                                        db.connectionName());
        editCmd->addSQLCommand(delCmd);
    }

    return editCmd;
}

template <typename Iterator>
void PartFactory::applyMultiValueProps (
        SQLEditCommand* editCmd,
        PartCategory* pcat,
        Iterator beg, Iterator end
) {
    QSqlDatabase db = getSQLDatabase();

    for (PartProperty* prop : pcat->getProperties()) {
        if ((prop->getFlags() & PartProperty::MultiValue) == 0) {
            continue;
        }

        QList<dbid_t> mvIDs;
        QList<QMap<QString, QVariant>> mvData;

        for (auto it = beg ; it != end ; ++it) {
            const Part& part = *it;
            assert(part.getPartCategory() == pcat);

            if (!part.isDataLoaded(prop)) {
                continue;
            }

            mvIDs << part.getID();

            QMap<QString, QVariant> d;
            d[prop->getMultiValueIDFieldName()] = part.getID();
            int idx = 0;
            for (const QVariant& singleVal : part.getData(prop).toList()) {
                d[prop->getFieldName()] = singleVal;
                d["idx"] = idx;
                mvData << d;
                idx++;
            }
        }

        SQLDeleteCommand* delCmd = new SQLDeleteCommand(prop->getMultiValueForeignTableName(),
                                                        prop->getMultiValueIDFieldName(),
                                                        mvIDs,
                                                        db.connectionName());
        editCmd->addSQLCommand(delCmd);

        SQLInsertCommand* insCmd = new SQLInsertCommand(prop->getMultiValueForeignTableName(),
                                                        "id",
                                                        mvData,
                                                        db.connectionName());
        editCmd->addSQLCommand(insCmd);
    }
}


}
