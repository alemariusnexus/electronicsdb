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

#include "AbstractSharedItemAssoc_Def.h"

#include <nxcommon/log.h>

namespace electronicsdb
{


template <typename DerivedT, class ItemA, typename ItemB>
void AbstractSharedItemAssoc<DerivedT, ItemA, ItemB>::markDirty()
{
    if ((d->flags & FlagInhibitMarkDirty) != 0) {
        return;
    }

    ItemA::foreachAssocTypeIndex([this](auto assocIdx) {
        if constexpr(std::is_same_v<typename ItemA::template AssocByIndex<assocIdx>, DerivedT>) {
            d->items.first.template markAssocDirty<assocIdx>();
        }
    });
    ItemB::foreachAssocTypeIndex([this](auto assocIdx) {
        if constexpr(std::is_same_v<typename ItemB::template AssocByIndex<assocIdx>, DerivedT>) {
            d->items.second.template markAssocDirty<assocIdx>();
        }
    });
}


}
