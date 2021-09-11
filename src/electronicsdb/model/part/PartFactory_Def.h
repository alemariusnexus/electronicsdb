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
#include <QMimeData>
#include <QObject>
#include <QUuid>
#include "Part.h"
#include "../abstract/AbstractSharedItemFactory.h"
#include "../PartProperty.h"

namespace electronicsdb
{


class PartFactory : public QObject, public AbstractSharedItemFactory<PartFactory, Part>
{
    Q_OBJECT

public:
    using Base = AbstractSharedItemFactory<PartFactory, Part>;

    using PropertyList = QList<PartProperty*>;

    enum LoadFlags
    {
        LoadContainers = 0x01,
        LoadPartLinks = 0x02,

        LoadReload = 0x10000,

        LoadAll = LoadContainers | LoadPartLinks
    };

public:
    static PartFactory& getInstance();

    static PropertyList loadItemsFuncAllProperties(PartCategory* pcat) { return pcat->getProperties(); }

public:
    Part findPartByID(PartCategory* pcat, dbid_t pid);
    PartList findPartsByID(PartCategory* pcat, const QList<dbid_t>& pids);

    template <typename Iterator>
    void loadItemsSingleCategory (
            Iterator beg, Iterator end,
            int flags = 0,
            const PropertyList& props = PropertyList());
    PartList loadItemsSingleCategory (
            PartCategory* pcat,
            const QString& whereClause,
            const QString& orderClause,
            const QString& limitClause,
            int flags = 0,
            const PropertyList& props = PropertyList());
    template <typename Iterator, typename PropFunc = decltype(&PartFactory::loadItemsFuncAllProperties)>
    void loadItems (
            Iterator beg, Iterator end,
            int flags = 0,
            const PropFunc& propFunc = &PartFactory::loadItemsFuncAllProperties);

    template <typename Iterator>
    EditCommand* insertItemsCmd(Iterator beg, Iterator end);

    template <typename Iterator>
    EditCommand* updateItemsCmd(Iterator beg, Iterator end);

    template <typename Iterator>
    EditCommand* deleteItemsCmd(Iterator beg, Iterator end);

    void processChangelog();
    void truncateChangelog();

    QString getPartListMimeDataType() const { return "application/vnd.nx-electronicsdb.partrecords"; }
    QMimeData* encodePartListToMimeData(const PartList& parts, const QUuid& uuid = QUuid());
    PartList decodePartListFromMimeData(const QMimeData* mimeData, bool* valid = nullptr, QUuid* uuid = nullptr);

public:
    void registerPart(const Part& part);
    void unregisterPart(PartCategory* pcat, dbid_t pid);

signals:
    void partsChanged (
            const PartList& inserted,
            const PartList& updated,
            const PartList& deleted
            );

private:
    PartFactory();

    template <typename Iterator>
    void loadItemsSingleCategory (
            PartCategory* pcat,
            const QString& whereClause,
            const QString& orderClause,
            const QString& limitClause,
            int flags,
            const QList<PartProperty*>& props,
            Iterator inBeg, Iterator inEnd,
            PartList* outList
            );

    template <typename Iterator>
    void applyMultiValueProps (
            SQLEditCommand* editCmd,
            PartCategory* pcat,
            Iterator beg, Iterator end
            );

private slots:
    void appModelAboutToBeReset();

private:
    std::unordered_map<PartCategory*, std::unordered_map<dbid_t, Part::WeakPtr>> partRegistry;
};


}
