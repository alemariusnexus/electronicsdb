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

#include <QList>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include "../abstract/AbstractSharedItemAssoc.h"

namespace electronicsdb
{

class Part;
class PartContainer;
class PartContainerAssoc;

namespace detail
{

template <>
struct AbstractSharedItemAssocTraits<class electronicsdb::PartContainerAssoc>
{
    struct Data
    {
    };
};

}


class PartContainerAssoc : public AbstractSharedItemAssoc<PartContainerAssoc, Part, PartContainer>
{
private:
    using Base = AbstractSharedItemAssoc<PartContainerAssoc, Part, PartContainer>;

public:
    static QString getAssocTableName() { return "container_part"; }

    template <typename ItemT>
    static void getAssocTableIDData(const ItemT& item, QList<QMap<QString, QVariant>>& outData);

public:
    PartContainerAssoc() : Base() {}
    PartContainerAssoc(const PartContainerAssoc& o) : Base(o) {}

    template <typename T, typename U>
    PartContainerAssoc(const T& item1, const U& item2) : Base(item1, item2) {}

    const Part& getPart() const;
    const PartContainer& getContainer() const;

    void copyDataFrom(const PartContainerAssoc& other) {}

    void buildPersistData(QList<QMap<QString, QVariant>>& outData) const;
};


}
