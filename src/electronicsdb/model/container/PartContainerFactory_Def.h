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

#include <unordered_map>
#include <QList>
#include <QObject>
#include "../abstract/AbstractSharedItemFactory.h"
#include "PartContainer.h"

namespace electronicsdb
{


class PartContainerFactory : public QObject, public AbstractSharedItemFactory<PartContainerFactory, PartContainer>
{
    Q_OBJECT

public:
    using Base = AbstractSharedItemFactory<PartContainerFactory, PartContainer>;

    using ContainerList = QList<PartContainer>;
    using ContainerIterator = ContainerList::iterator;
    using CContainerIterator = ContainerList::const_iterator;

    enum LoadFlags
    {
        LoadContainerParts = 0x01,

        LoadReload = 0x10000,

        LoadAll = LoadContainerParts
    };

public:
    static PartContainerFactory& getInstance();

public:
    PartContainer findContainerByID(dbid_t cid);
    ContainerList findContainersByID(const QList<dbid_t>& cids);

    template <typename Iterator>
    void loadItems(Iterator beg, Iterator end, int flags = 0);

    void loadItems(PartContainer& cont, int flags = 0) { loadItems(&cont, &cont + 1, flags); }

    ContainerList loadItems(const QString& whereClause, const QString& orderClause, int flags = 0);


    template <typename Iterator>
    EditCommand* insertItemsCmd(Iterator beg, Iterator end);

    template <typename Iterator>
    EditCommand* updateItemsCmd(Iterator beg, Iterator end);

    template <typename Iterator>
    EditCommand* deleteItemsCmd(Iterator beg, Iterator end);

    void processChangelog();
    void truncateChangelog();

public:
    void registerContainer(const PartContainer& cont);
    void unregisterContainer(dbid_t cid);

private:
    PartContainerFactory();

    template <typename Iterator>
    void loadItems (
            const QString& whereClause,
            const QString& orderClause,
            int flags,
            Iterator inBeg, Iterator inEnd,
            ContainerList* outList
            );

private slots:
    void appModelAboutToBeReset();

signals:
    void containersChanged (
            const PartContainerList& inserted,
            const PartContainerList& updated,
            const PartContainerList& deleted
            );

private:
    std::unordered_map<dbid_t, PartContainer::WeakPtr> contRegistry;
};


}
