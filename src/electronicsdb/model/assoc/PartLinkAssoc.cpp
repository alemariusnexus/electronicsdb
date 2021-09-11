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

#include "PartLinkAssoc.h"

#include "../part/Part.h"
#include "../PartCategory.h"

namespace electronicsdb
{


void PartLinkAssoc::addLinkType(PartLinkType* type)
{
    checkValid();
    markDirty();
    d->linkTypes.insert(type);
}

bool PartLinkAssoc::removeLinkType(PartLinkType* type)
{
    checkValid();
    markDirty();
    return d->linkTypes.remove(type);
}

void PartLinkAssoc::setLinkTypes(const QSet<PartLinkType*>& types)
{
    checkValid();
    markDirty();
    d->linkTypes = types;
}

bool PartLinkAssoc::hasLinkType(PartLinkType* type) const
{
    checkValid();
    return d->linkTypes.contains(type);
}

const QSet<PartLinkType*>& PartLinkAssoc::getLinkTypes() const
{
    checkValid();
    return d->linkTypes;
}

void PartLinkAssoc::copyDataFrom(const PartLinkAssoc& other)
{
    checkValid();
    d->linkTypes = other.d->linkTypes;
    markDirty();
}

void PartLinkAssoc::buildPersistData(QList<QMap<QString, QVariant>>& outData) const
{
    const Part& partA = getItemA();
    const Part& partB = getItemB();

    for (PartLinkType* type : d->linkTypes) {
        QMap<QString, QVariant> data;
        data["type"] = type->getID();

        if (type->isPartA(partA)) {
            data["ptype_a"] = partA.getPartCategory()->getID();
            data["pid_a"] = partA.getID();
            data["ptype_b"] = partB.getPartCategory()->getID();
            data["pid_b"] = partB.getID();
        } else {
            data["ptype_a"] = partB.getPartCategory()->getID();
            data["pid_a"] = partB.getID();
            data["ptype_b"] = partA.getPartCategory()->getID();
            data["pid_b"] = partA.getID();
        }

        outData << data;
    }
}


}
