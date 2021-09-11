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

#include "AbstractSharedItem_Def.h"

#include <nxcommon/log.h>

namespace electronicsdb
{


template <typename DerivedT, typename... AssocsT>
AbstractSharedItem<DerivedT, AssocsT...>::AbstractSharedItem(const DerivedT& other, CloneTag)
        : d(other ? std::make_shared<Data>(*other.d.get()) : std::shared_ptr<Data>())
{
    if (other.a) {
        claimAssocs();
    }

    foreachAssocTypeIndex([this, other](auto assocIdx) {
        if (other.template isAssocLoaded<assocIdx>()) {
            auto assocItems = other.template getAssocItems<assocIdx>();
            loadAssocItems<assocIdx>(assocItems.begin(), assocItems.end());

            auto assocs = getAssocs<assocIdx>();
            auto otherAssocs = other.template getAssocs<assocIdx>();

            assert(assocs.size() == otherAssocs.size());

            for (auto it = assocs.begin(), oit = otherAssocs.begin() ; it != assocs.end() ; ++it, ++oit) {
                it->copyDataFrom(*oit);
            }
        }
    });
}

template <typename DerivedT, typename... AssocsT>
template <size_t assocIdx, typename ItemIterator>
QList<typename AbstractSharedItem<DerivedT, AssocsT...>::template AssocByIndex<assocIdx>>
AbstractSharedItem<DerivedT, AssocsT...>::loadAssocItems(ItemIterator beg, ItemIterator end)
{
    using Assoc = AssocByIndex<assocIdx>;

    DerivedT* derivedThis = static_cast<DerivedT*>(this);

    claimAssocs();

    QList<Assoc> assocs;
    for (auto it = beg ; it != end ; ++it) {
        assocs << Assoc(*derivedThis, *it);
    }

    a->template set<assocIdx>(assocs);

    return assocs;
}

template <typename DerivedT, typename... AssocsT>
template <size_t assocIdx, typename ItemIterator>
QList<typename AbstractSharedItem<DerivedT, AssocsT...>::template AssocByIndex<assocIdx>>
AbstractSharedItem<DerivedT, AssocsT...>::addAssocItems(ItemIterator beg, ItemIterator end)
{
    using Assoc = AssocByIndex<assocIdx>;

    claimAssocs();

    checkAssocLoaded<assocIdx>();

    DerivedT* derivedThis = static_cast<DerivedT*>(this);

    QList<Assoc> resAssocs;

    for (auto it = beg ; it != end ; ++it) {
        const auto& other = *it;

        bool existsInOld = false;
        for (const auto& assoc : getAssocs<assocIdx>()) {
            if (assoc.getOther(*derivedThis) == other) {
                // Already associated -> simply return the existing assoc
                resAssocs << assoc;
                existsInOld = true;
                break;
            }
        }

        if (!existsInOld) {
            // Not associated yet -> create new assoc
            Assoc assoc(*derivedThis, other);
            a->template addSingle<assocIdx>(assoc);
            resAssocs << assoc;
        }
    }

    return resAssocs;
}

template <typename DerivedT, typename... AssocsT>
template <size_t assocIdx, typename ItemIterator>
int AbstractSharedItem<DerivedT, AssocsT...>::removeAssocItems(ItemIterator beg, ItemIterator end)
{
    claimAssocs();

    DerivedT* derivedThis = static_cast<DerivedT*>(this);

    int numRemoved = 0;

    for (ItemIterator it = beg ; it != end ; ++it) {
        const auto& other = *it;
        for (const auto& assoc : getAssocs<assocIdx>()) {
            if (assoc.getOther(*derivedThis) == other) {
                // Assoc exists -> remove it
                numRemoved++;
                a->template removeSingle<assocIdx>(assoc);
                break;
            }
        }
    }

    return numRemoved;
}

template <typename DerivedT, typename... AssocsT>
template <size_t assocIdx>
int AbstractSharedItem<DerivedT, AssocsT...>::clearAssocs()
{
    claimAssocs();

    auto assocs = getAssocs<assocIdx>();
    for (const auto& assoc : assocs) {
        a->template removeSingle<assocIdx>(assoc);
    }

    return assocs.size();
}

template <typename DerivedT, typename... AssocsT>
template <size_t assocIdx, typename ItemIterator>
QList<typename AbstractSharedItem<DerivedT, AssocsT...>::template AssocByIndex<assocIdx>>
AbstractSharedItem<DerivedT, AssocsT...>::setAssocItems(ItemIterator beg, ItemIterator end)
{
    using Assoc = AssocByIndex<assocIdx>;
    using Other = typename Assoc::template Other<DerivedT>;

    claimAssocs();

    checkAssocLoaded<assocIdx>();

    DerivedT* derivedThis = static_cast<DerivedT*>(this);

    const QList<Assoc>& oldAssocs = a->template get<assocIdx>();

    QList<Other> removed;
    QList<Other> added;

    // Check for removed associations
    for (const Assoc& assoc : oldAssocs) {
        const Other& oldOther = assoc.getOther(*derivedThis);

        bool foundInNew = false;
        for (auto it = beg ; it != end ; ++it) {
            const Other& o = *it;
            if (o == oldOther) {
                foundInNew = true;
                break;
            }
        }

        if (!foundInNew) {
            removed << oldOther;
        }
    }

    for (auto it = beg ; it != end ; ++it) {
        const Other& other = *it;

        // Check if the association already exists on this side
        bool foundInOld = false;
        for (const Assoc& assoc : oldAssocs) {
            if (assoc.getOther(*derivedThis) == other) {
                foundInOld = true;
                break;
            }
        }

        if (!foundInOld) {
            added << other;
        }
    }

    addAssocItems<assocIdx>(added.begin(), added.end());
    removeAssocItems<assocIdx>(removed.begin(), removed.end());

    QList<Assoc> resAssocs;
    for (auto it = beg ; it != end ; ++it) {
        resAssocs << getAssoc<assocIdx>(*it);
    }

    return resAssocs;
}


}
