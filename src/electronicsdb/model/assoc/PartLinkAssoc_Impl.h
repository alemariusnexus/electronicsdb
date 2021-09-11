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

#include "PartLinkAssoc_Def.h"

#include "../PartCategory.h"

namespace electronicsdb
{


template <typename ItemT>
void PartLinkAssoc::getAssocTableIDData(const ItemT& item, QList<QMap<QString, QVariant>>& outData)
{
    const PartCategory* pcat = item.getPartCategory();
    QList<PartLinkType*> types = pcat->getPartLinkTypes();

    for (PartLinkType* type : types) {
        QMap<QString, QVariant> d;
        d["type"] = type->getID();

        if (type->isPartA(item)) {
            d["ptype_a"] = pcat->getID();
            d["pid_a"] = item.getID();
        } else {
            d["ptype_b"] = pcat->getID();
            d["pid_b"] = item.getID();
        }

        outData << d;
    }
}


}


Q_DECLARE_METATYPE(electronicsdb::PartLinkAssoc)
