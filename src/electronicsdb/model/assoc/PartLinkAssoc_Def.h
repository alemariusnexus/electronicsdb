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

#include <QMetaType>
#include <QSet>
#include "../abstract/AbstractSharedItemAssoc.h"
#include "../PartLinkType.h"

namespace electronicsdb
{


class Part;
class PartLinkAssoc;

namespace detail
{

template <>
struct AbstractSharedItemAssocTraits<class electronicsdb::PartLinkAssoc>
{
    struct Data
    {
    public:
        QSet<PartLinkType*> linkTypes;
    };
};

}


class PartLinkAssoc : public AbstractSharedItemAssoc<PartLinkAssoc, Part, Part>
{
private:
    using Base = AbstractSharedItemAssoc<PartLinkAssoc, Part, Part>;

public:
    static QString getAssocTableName() { return "partlink"; }

    template <typename ItemT>
    static void getAssocTableIDData(const ItemT& item, QList<QMap<QString, QVariant>>& outData);

public:
    PartLinkAssoc() : Base() {}
    PartLinkAssoc(const PartLinkAssoc& other) : Base(other) {}

    template <typename T, typename U>
    PartLinkAssoc(const T& item1, const U& item2) : Base(item1, item2) {}

    void addLinkType(PartLinkType* type);
    bool removeLinkType(PartLinkType* type);
    void setLinkTypes(const QSet<PartLinkType*>& types);
    bool hasLinkType(PartLinkType* type) const;
    const QSet<PartLinkType*>& getLinkTypes() const;

    void copyDataFrom(const PartLinkAssoc& other);

    void buildPersistData(QList<QMap<QString, QVariant>>& outData) const;
};


}
