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

#include "PartContainerAssoc_Def.h"

#include "../PartCategory.h"

namespace electronicsdb
{


template <typename ItemT>
void PartContainerAssoc::getAssocTableIDData(const ItemT& item, QList<QMap<QString, QVariant>>& outData)
{
    QMap<QString, QVariant> idData;
    if constexpr(std::is_same_v<ItemT, Part>) {
        idData["ptype"] = item.getPartCategory()->getID();
        idData["pid"] = item.getID();
    } else {
        idData["cid"] = item.getID();
    }
    outData << idData;
}


}


Q_DECLARE_METATYPE(electronicsdb::PartContainerAssoc)
