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

#include <memory>
#include <type_traits>
#include <utility>
#include <nxcommon/exception/InvalidStateException.h>

namespace electronicsdb
{

namespace detail
{

template <typename T>
struct AbstractSharedItemAssocTraits;

}


template <typename DerivedT, class ItemA, typename ItemB>
class AbstractSharedItemAssoc
{
    friend ItemA;
    friend ItemB;

public:
    using Traits = typename detail::AbstractSharedItemAssocTraits<DerivedT>;

    template <typename T>
    using Other = std::conditional_t<std::is_same_v<ItemA, T>, ItemB, ItemA>;

private:
    enum Flags
    {
        FlagInhibitMarkDirty = 0x01
    };

    struct Data : public Traits::Data
    {
        template <typename T, typename U>
        Data(const T& item1, const U& item2)
                : flags(0)
        {
            if constexpr (std::is_same_v<T, ItemA>) {
                items = std::pair<ItemA, ItemB>(item1, item2);
            } else {
                items = std::pair<ItemA, ItemB>(item2, item1);
            }
            items.first.dropAssocs();
            items.second.dropAssocs();
        }

        std::pair<ItemA, ItemB> items;
        int flags;
    };

public:
    bool isValid() const { return (bool) d; }
    bool isNull() const { return !isValid(); }
    operator bool() const { return isValid(); }

    DerivedT& operator=(const DerivedT& o)
    {
        if (this != &o) {
            d = o.d;
        }
        return *static_cast<DerivedT*>(this);
    }

    template <typename Item>
    const Other<Item>& getOther(const Item& item) const
    {
        checkValid();
        if constexpr(std::is_same_v<ItemA, ItemB>) {
            if (d->items.first == item) {
                return d->items.second;
            } else {
                return d->items.first;
            }
        } else {
            return std::get<Other<Item>>(d->items);
        }
    }

    bool operator==(const DerivedT& other) const { return d == other.d; }
    bool operator!=(const DerivedT& other) const { return d != other.d; }

    void setInhibitMarkDirty(bool inhibit)
    {
        checkValid();
        if (inhibit) {
            d->flags |= FlagInhibitMarkDirty;
        } else {
            d->flags &= ~FlagInhibitMarkDirty;
        }
    }

protected:
    AbstractSharedItemAssoc() {}
    AbstractSharedItemAssoc(const DerivedT& other) : d(other.d) {}

    template <typename T, typename U>
    AbstractSharedItemAssoc(const T& item1, const U& item2) : d(std::make_shared<Data>(item1, item2)) {}

    // NOTE: If ItemA and ItemB are the same type, the order is arbitrary. In that case, use getOther() instead.
    const ItemA& getItemA() const { return d->items.first; }
    const ItemB& getItemB() const { return d->items.second; }

    void markDirty();

    void checkValid() const
    {
        if (!isValid()) {
            throw InvalidStateException("Assoc is null", __FILE__, __LINE__);
        }
    }

protected:
    std::shared_ptr<Data> d;
};


}
