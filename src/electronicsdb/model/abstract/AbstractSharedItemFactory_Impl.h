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

#include "AbstractSharedItemFactory_Def.h"

#include <functional>
#include <memory>
#include <QList>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QVariant>
#include <nxcommon/log.h>
#include "../../command/sql/SQLDeleteCommand.h"
#include "../../command/sql/SQLInsertCommand.h"
#include "../../command/sql/SQLUpdateCommand.h"
#include "../../command/EditStack.h"
#include "../../command/SQLEditCommand.h"
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../System.h"

namespace electronicsdb
{


template <class DerivedT, typename ItemT>
template <typename Iterator>
bool AbstractSharedItemFactory<DerivedT, ItemT>::applyAssocChanges (
        Iterator beg, Iterator end,
        SQLEditCommand* editCmd,
        ApplyAssocType type
) {
    if (beg == end) {
        return false;
    }

    bool anyChanges = false;

    QMap<QString, QList<QMap<QString, QVariant>>> assocIDDataByTable;
    QMap<QString, QList<QMap<QString, QVariant>>> assocDataByTable;

    const bool deleteOld = (type == ApplyAssocTypeUpdate  ||  type == ApplyAssocTypeDelete);
    const bool insertNew = (type == ApplyAssocTypeInsert  ||  type == ApplyAssocTypeUpdate);
    const bool skipNonDirty = (type == ApplyAssocTypeUpdate);

    // Build assoc data maps
    for (auto it = beg ; it != end ; ++it) {
        const ItemType& item = *it;

        ItemType::foreachAssocTypeIndex([&](auto assocIdx) {
            using Assoc = typename ItemType::template AssocByIndex<assocIdx>;

            if (type != ApplyAssocTypeDelete  &&  !item.template isAssocLoaded<assocIdx>()) {
                // Unless this is an actual delete operation, we'll only touch assocs that are loaded. For the delete
                // operation, we MUST delete even the ones that aren't loaded, to avoid dangling references when an item
                // is deleted.
                return;
            }
            if (skipNonDirty  &&  !item.template isAssocDirty<assocIdx>()) {
                return;
            }

            QString tableName = Assoc::getAssocTableName();

            if (deleteOld) {
                auto& assocIDData = assocIDDataByTable[tableName];
                Assoc::getAssocTableIDData(item, assocIDData);
            }

            if (insertNew) {
                auto& assocData = assocDataByTable[tableName];
                for (const Assoc& assoc : item.template getAssocs<assocIdx>()) {
                    assoc.buildPersistData(assocData);
                }
            }

            if (item.template isAssocLoaded<assocIdx>()) {
                item.template clearAssocDirty<assocIdx>();
            }
        });
    }

    // Build assoc delete commands (so we can insert with a clean slate later)
    if (deleteOld) {
        for (auto tit = assocIDDataByTable.begin() ; tit != assocIDDataByTable.end() ; ++tit) {
            QString tableName = tit.key();
            auto& assocIDData = tit.value();

            if (!assocIDData.empty()) {
                QList<QVariant> whereBindParamValues;
                QString delWhereExpr = buildWhereExpressionFromList(assocIDData, "OR", whereBindParamValues);

                SQLDeleteCommand* delCmd = new SQLDeleteCommand(tableName, delWhereExpr, whereBindParamValues,
                                                                getSQLDatabase().connectionName());
                editCmd->addSQLCommand(delCmd);

                anyChanges = true;
            }
        }
    }

    // Build assoc insert commands
    if (insertNew) {
        for (auto tit = assocDataByTable.begin() ; tit != assocDataByTable.end() ; ++tit) {
            QString tableName = tit.key();
            auto& assocData = tit.value();

            if (!assocData.empty()) {
                SQLInsertCommand* insCmd = new SQLInsertCommand(tableName, "id",
                                                                assocData, getSQLDatabase().connectionName());
                editCmd->addSQLCommand(insCmd);

                anyChanges = true;
            }
        }
    }

    return anyChanges;
}

template <class DerivedT, typename ItemT>
void AbstractSharedItemFactory<DerivedT, ItemT>::checkDatabaseConnection() const
{
    if (!getSQLDatabase().isValid()) {
        throw NoDatabaseConnectionException("No database connection for items", __FILE__, __LINE__);
    }
}

template <class DerivedT, typename ItemT>
void AbstractSharedItemFactory<DerivedT, ItemT>::applyCommand(EditCommand* cmd)
{
    if (cmd) {
        System::getInstance()->getEditStack()->push(cmd);
    }
}

template <class DerivedT, typename ItemT>
QString AbstractSharedItemFactory<DerivedT, ItemT>::buildWhereExpressionFromList (
        const QList<QMap<QString, QVariant>>& data,
        const QString& op,
        QList<QVariant>& whereBindParamValues
) const {
    QString expr;

    QSqlDatabase db = getSQLDatabase();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    for (const auto& d : data) {
        if (!expr.isEmpty()) {
            expr += QString(" %1 ").arg(op);
        }

        expr += "(";

        for (auto it = d.begin() ; it != d.end() ; it++) {
            if (it != d.begin()) {
                expr += " AND ";
            }
            whereBindParamValues << it.value();
            expr += QString("%1=?").arg(dbw->escapeIdentifier(it.key()));
        }

        expr += ")";
    }

    return expr;
}

template <class DerivedT, typename ItemT>
template <typename Iterator, typename FuncT>
void AbstractSharedItemFactory<DerivedT, ItemT>::addIDInsertListener(SQLInsertCommand* cmd, Iterator beg, Iterator end, const FuncT& listener) const
{
    // We can NOT just pass the iterators to the lambda, because they might become invalid before the listener is
    // invoked. We also need to pass references/pointers to the ORIGINAL objects, because the listener might load
    // assocs, which would otherwise be deleted again shortly after the listener finishes. For that reason, we pass
    // a list by value to the listener, but the values are actually references to the original objects.
    QList<std::reference_wrapper<ItemType>> items;
    for (auto it = beg ; it != end ; ++it) {
        items << *it;
    }
    cmd->setListener([cmd, listener, items](SQLCommand*, SQLCommand::Operation op) {
        if (op != SQLCommand::OpCommit  ||  cmd->hasBeenReverted()) {
            return;
        }
        auto it = items.begin();
        for (dbid_t id : cmd->getInsertIDs()) {
            ItemType& item = *it;
            if (!item.hasID()) {
                item.setID(id);
            }
            it++;
        }
        listener(items.begin(), items.end());
    });
}


}
