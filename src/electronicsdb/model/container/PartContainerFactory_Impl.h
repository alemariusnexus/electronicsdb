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

#include "PartContainerFactory_Def.h"

#include <memory>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <nxcommon/log.h>
#include "../../command/sql/SQLDeleteCommand.h"
#include "../../command/sql/SQLInsertCommand.h"
#include "../../command/sql/SQLUpdateCommand.h"
#include "../../command/SQLEditCommand.h"
#include "../../exception/SQLException.h"
#include "../../util/elutil.h"
#include "../../System.h"
#include "../assoc/PartContainerAssoc.h"
#include "../part/Part.h"
#include "../part/PartFactory.h"

namespace electronicsdb
{


template <typename Iterator>
void PartContainerFactory::loadItems(Iterator beg, Iterator end, int flags)
{
    if (beg == end) {
        return;
    }

    QString idListStr;
    for (auto it = beg ; it != end ; ++it) {
        if (!idListStr.isEmpty()) {
            idListStr += ",";
        }
        idListStr += QString::number(it->getID());
    }

    loadItems(QString("WHERE c.id IN (%1)").arg(idListStr),
              QString(),
              flags,
              beg, end,
              nullptr);
}

template <typename Iterator>
void PartContainerFactory::loadItems (
        const QString& whereClause,
        const QString& orderClause,
        int flags,
        Iterator inBeg, Iterator inEnd,
        ContainerList* outList
) {
    checkDatabaseConnection();

    QMap<dbid_t, PartContainer*> contsByID;
    for (auto it = inBeg ; it != inEnd ; ++it) {
        PartContainer& cont = *it;
        assert(cont);
        assert(cont.hasID());
        contsByID[it->getID()] = &cont;
    }

    bool haveInputConts = !contsByID.empty();

    if ((flags & LoadReload) != 0) {
        // Reload assocs only if any of the input container has them loaded
        if (haveInputConts) {
            for (auto it = inBeg ; it != inEnd ; ++it) {
                const PartContainer& cont = *it;
                if (cont.isPartAssocsLoaded()) {
                    flags |= LoadContainerParts;
                    break;
                }
            }
        } else {
            flags |= LoadAll;
        }
    }

    System* sys = System::getInstance();
    PartFactory& partFactory = PartFactory::getInstance();

    QSqlDatabase db = getSQLDatabase();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QString moreFields;
    QString joinCode;
    QString groupCode;

    if ((flags & LoadContainerParts) != 0) {
        moreFields += QString(", %1 parts").arg(dbw->groupConcatCode("ptype||':'||pid"));
        joinCode = QString("LEFT JOIN container_part p ON c.id=p.cid");
        groupCode = QString("GROUP BY c.id");
    }

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, QString(
            "SELECT c.id, c.name%1\n"
            "FROM   container c\n"
            "%2\n"
            "%3\n"
            "%4\n"
            "%5"
            ).arg(moreFields,
                  joinCode,
                  whereClause,
                  groupCode,
                  orderClause),
            "Error loading containers");

    while (query.next()) {
        dbid_t cid = VariantToDBID(query.value(0));
        QString cname = query.value(1).toString();

        PartContainer regCont = haveInputConts ? PartContainer() : findContainerByID(cid);
        PartContainer& cont = haveInputConts ? *contsByID[cid] : regCont;

        cont.setName(cname);
        cont.clearDirty();

        if ((flags & LoadContainerParts) != 0) {
            PartList parts;
            if (!query.isNull(2)) {
                QString partsStr = query.value(2).toString();
                for (const QString partStr : partsStr.split(',')) {
                    auto splitPart = partStr.split(':');
                    assert(splitPart.size() == 2);
                    PartCategory* pcat = sys->getPartCategory(splitPart[0]);
                    dbid_t pid = static_cast<dbid_t>(splitPart[1].toLongLong());
                    parts << partFactory.findPartByID(pcat, pid);
                }
            }
            cont.loadParts(parts.begin(), parts.end());
            cont.template clearAssocDirty<PartContainer::PartContainerAssocIdx>();
        }

        if (outList) {
            *outList << cont;
        }
    }
}

template <typename Iterator>
EditCommand* PartContainerFactory::insertItemsCmd(Iterator beg, Iterator end)
{
    if (beg == end) {
        return nullptr;
    }
    checkDatabaseConnection();

    QSqlDatabase db = getSQLDatabase();

    SQLEditCommand* editCmd = new SQLEditCommand;

    QList<QMap<QString, QVariant>> insData;

    for (auto it = beg ; it != end ; ++it) {
        const PartContainer& cont = *it;

        assert(cont);

        cont.clearDirty();

        QMap<QString, QVariant> d;
        if (cont.hasID()) {
            d["id"] = cont.getID();
        }
        d["name"] = cont.getName();
        insData << d;
    }

    SQLInsertCommand* insCmd = new SQLInsertCommand("container", "id", insData, db.connectionName());
    addIDInsertListener(insCmd, beg, end, [this, editCmd](auto beg, auto end) {
        // Insert assocs
        applyAssocChanges(beg, end, editCmd, ApplyAssocTypeInsert);
    });
    editCmd->addSQLCommand(insCmd);

    return editCmd;
}

template <typename Iterator>
EditCommand* PartContainerFactory::updateItemsCmd(Iterator beg, Iterator end)
{
    if (beg == end) {
        return nullptr;
    }
    checkDatabaseConnection();

    QSqlDatabase db = getSQLDatabase();

    SQLEditCommand* editCmd = new SQLEditCommand;

    for (auto it = beg ; it != end ; ++it) {
        const PartContainer& cont = *it;

        assert(cont);
        assert(cont.hasID());

        if (cont.isDirty()) {
            SQLUpdateCommand* updCmd = new SQLUpdateCommand("container", "id", cont.getID(),
                                                            db.connectionName());

            if (cont.isNameLoaded()) {
                updCmd->addFieldValue("name", cont.getName());
            }

            editCmd->addSQLCommand(updCmd);
            cont.clearDirty();
        }
    }

    applyAssocChanges(beg, end, editCmd, ApplyAssocTypeUpdate);

    return editCmd;
}

template <typename Iterator>
EditCommand* PartContainerFactory::deleteItemsCmd(Iterator beg, Iterator end)
{
    if (beg == end) {
        return nullptr;
    }
    checkDatabaseConnection();

    QSqlDatabase db = getSQLDatabase();

    SQLEditCommand* editCmd = new SQLEditCommand;

    QList<dbid_t> ids;

    for (auto it = beg ; it != end ; ++it) {
        const PartContainer& cont = *it;
        assert(cont);
        cont.clearDirty();
        if (cont.hasID()) {
            ids << cont.getID();
        }
    }

    applyAssocChanges(beg, end, editCmd, ApplyAssocTypeDelete);

    SQLDeleteCommand* delCmd = new SQLDeleteCommand("container", "id", ids, db.connectionName());
    editCmd->addSQLCommand(delCmd);

    return editCmd;
}


}
