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

#include "../../global.h"

#include <QObject>
#include "../../command/sql/SQLInsertCommand.h"
#include "../../command/EditCommand.h"
#include "../../command/SQLEditCommand.h"

namespace electronicsdb
{


template <class DerivedT, class ItemT>
class AbstractSharedItemFactory
{
public:
    using Derived = DerivedT;
    using ItemType = ItemT;

protected:
    enum ApplyAssocType
    {
        ApplyAssocTypeInsert,
        ApplyAssocTypeUpdate,
        ApplyAssocTypeDelete
    };

public:
    template <typename Iterator>
    void insertItems(Iterator beg, Iterator end)
            { applyCommand(static_cast<Derived*>(this)->insertItemsCmd(beg, end)); }
    EditCommand* insertItemCmd(ItemType& item)
            { return static_cast<Derived*>(this)->insertItemsCmd(&item, &item + 1); }
    void insertItem(ItemType& item) { insertItems(&item, &item + 1); }

    template <typename Iterator>
    void updateItems(Iterator beg, Iterator end)
            { applyCommand(static_cast<Derived*>(this)->updateItemsCmd(beg, end)); }
    EditCommand* updateItemCmd(const ItemType& item)
            { return static_cast<Derived*>(this)->updateItemsCmd(&item, &item + 1); }
    void updateItem(const ItemType& item) { updateItems(&item, &item + 1); }

    template <typename Iterator>
    void deleteItems(Iterator beg, Iterator end)
            { applyCommand(static_cast<Derived*>(this)->deleteItemsCmd(beg, end)); }
    EditCommand* deleteItemCmd(const ItemType& item)
            { return static_cast<Derived*>(this)->deleteItemsCmd(&item, &item + 1); }
    void deleteItem(const ItemType& item) { deleteItems(&item, &item + 1); }


protected:
    /*template <size_t assocIdx, typename Iterator>
    void loadAssocs(Iterator beg, Iterator end);*/

    template <typename Iterator>
    bool applyAssocChanges (
            Iterator beg, Iterator end,
            SQLEditCommand* editCmd,
            ApplyAssocType type
            );

    void checkDatabaseConnection() const;
    void applyCommand(EditCommand* cmd);

    QString buildWhereExpressionFromList (
            const QList<QMap<QString, QVariant>>& data,
            const QString& op,
            QList<QVariant>& whereBindParamValues
            ) const;

    template <typename Iterator, typename FuncT>
    void addIDInsertListener(SQLInsertCommand* cmd, Iterator beg, Iterator end, const FuncT& listener) const;
};


}
