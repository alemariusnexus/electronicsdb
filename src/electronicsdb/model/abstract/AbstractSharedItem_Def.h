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

#include <algorithm>
#include <bitset>
#include <memory>
#include <utility>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/log.h>
#include "../../util/tplutil.h"

#define EDB_ABSTRACTSHAREDITEM_DECLARE_ASSOCMETHODS(assocIdx,ThisCls,OtherCls,AssocCls,nameSingular,namePlural)     \
        bool is##nameSingular##AssocsLoaded() const;                                                                \
                                                                                                                    \
        QList<AssocCls> get##nameSingular##Assocs() const;                                                          \
                                                                                                                    \
        AssocCls get##nameSingular##Assoc(const OtherCls& item) const;                                              \
                                                                                                                    \
        QList<OtherCls> get##namePlural() const;                                                                    \
                                                                                                                    \
        void drop##namePlural();                                                                                    \
                                                                                                                    \
        template <typename ItemIterator>                                                                            \
        QList<AssocCls> load##namePlural(ItemIterator beg, ItemIterator end)                                        \
                { return loadAssocItems<assocIdx>(beg, end); }                                                      \
                                                                                                                    \
        QList<AssocCls> load##namePlural(const QList<OtherCls>& items);                                             \
                                                                                                                    \
        template <typename ItemIterator>                                                                            \
        QList<AssocCls> add##namePlural(ItemIterator beg, ItemIterator end)                                         \
                { return addAssocItems<assocIdx>(beg, end); }                                                       \
                                                                                                                    \
        QList<AssocCls> add##namePlural(const QList<OtherCls>& items);                                              \
                                                                                                                    \
        AssocCls add##nameSingular(const OtherCls& item);                                                           \
                                                                                                                    \
        template <typename ItemIterator>                                                                            \
        int remove##namePlural(ItemIterator beg, ItemIterator end)                                                  \
                { return removeAssocItems<assocIdx>(beg, end); }                                                    \
                                                                                                                    \
        int remove##namePlural(const QList<OtherCls>& items);                                                       \
                                                                                                                    \
        bool remove##nameSingular(const OtherCls& item);                                                            \
                                                                                                                    \
        int clear##namePlural();                                                                                    \
                                                                                                                    \
        template <typename ItemIterator>                                                                            \
        QList<AssocCls> set##namePlural(ItemIterator beg, ItemIterator end)                                         \
                { return setAssocItems<assocIdx>(beg, end); }                                                       \
                                                                                                                    \
        QList<AssocCls> set##namePlural(const QList<OtherCls>& items);

#define EDB_ABSTRACTSHAREDITEM_DEFINE_ASSOCMETHODS(assocIdx,ThisCls,OtherCls,AssocCls,nameSingular,namePlural)      \
        bool ThisCls::is##nameSingular##AssocsLoaded() const                                                        \
                { return isAssocLoaded<assocIdx>(); }                                                               \
                                                                                                                    \
        QList<AssocCls> ThisCls::get##nameSingular##Assocs() const                                                  \
                { return getAssocs<assocIdx>(); }                                                                   \
                                                                                                                    \
        AssocCls ThisCls::get##nameSingular##Assoc(const OtherCls& item) const                                      \
                { return getAssoc<assocIdx>(item); }                                                                \
                                                                                                                    \
        QList<OtherCls> ThisCls::get##namePlural() const                                                            \
                { return getAssocItems<assocIdx>(); }                                                               \
                                                                                                                    \
        void ThisCls::drop##namePlural()                                                                            \
                { dropAssocs<assocIdx>(); }                                                                         \
                                                                                                                    \
        QList<AssocCls> ThisCls::load##namePlural(const QList<OtherCls>& items)                                     \
                { return load##namePlural(items.begin(), items.end()); }                                            \
                                                                                                                    \
        QList<AssocCls> ThisCls::add##namePlural(const QList<OtherCls>& items)                                      \
                { return add##namePlural(items.begin(), items.end()); }                                             \
                                                                                                                    \
        AssocCls ThisCls::add##nameSingular(const OtherCls& item)                                                   \
                { return addAssocItem<assocIdx>(item); }                                                            \
                                                                                                                    \
        int ThisCls::remove##namePlural(const QList<OtherCls>& items)                                               \
                { return remove##namePlural(items.begin(), items.end()); }                                          \
                                                                                                                    \
        bool ThisCls::remove##nameSingular(const OtherCls& item)                                                    \
                { return removeAssocItem<assocIdx>(item); }                                                         \
                                                                                                                    \
        int ThisCls::clear##namePlural()                                                                            \
                { return clearAssocs<assocIdx>(); }                                                                 \
                                                                                                                    \
        QList<AssocCls> ThisCls::set##namePlural(const QList<OtherCls>& items)                                      \
                { return set##namePlural(items.begin(), items.end()); }

namespace electronicsdb
{

namespace detail
{

template <typename T>
struct AbstractSharedItemTraits;

}


template <typename DerivedT, typename... AssocsT>
class AbstractSharedItem
{
    EDB_UTIL_DECLARE_PACK_FRIENDS(AssocsT)

    template <typename T, typename... U>
    friend class AbstractSharedItem;

    // TODO: Keep the order of associations. Currently, they are stored into and loaded from the database unordered. The
    // problem is that the order must be specified from BOTH sides separately, but only one of these sides might be
    // loaded, so we don't know the other side's order when writing. There are already index fields for this in the
    // tables (pidx/cidx in container_part, and idx_a/idx_b in partlink), but they are currently ignored and not filled
    // in either.

public:
    using Traits = typename detail::AbstractSharedItemTraits<DerivedT>;
    using AssocsTuple = std::tuple<AssocsT...>;

    template <size_t idx>
    using AssocByIndex = std::tuple_element_t<idx, AssocsTuple>;

    //static constexpr size_t getAssocTypeCount() { return std::tuple_size_v<AssocsTuple>; }

public:
    struct CloneTag {};

    enum PersistOperation
    {
        PersistInsert,
        PersistUpdate,
        PersistDelete
    };

protected:
    class AssocData
    {
    private:
        enum Flags
        {
            FlagLoaded = 0,
            FlagDirty = 1,

            Flag_COUNT = FlagDirty+1
        };

    public:
        template <size_t idx>
        bool isLoaded() const
        {
            return assocFlags.test(idx*Flag_COUNT + FlagLoaded);
        }

        template <size_t idx>
        bool isDirty() const
        {
            return assocFlags.test(idx*Flag_COUNT + FlagDirty);
        }

        template <size_t idx>
        void setDirty(bool dirty) const
        {
            const_cast<AssocData*>(this)->assocFlags.set(idx*Flag_COUNT + FlagDirty, dirty);
        }

        template <size_t idx>
        const QList<AssocByIndex<idx>>& get() const
        {
            return std::get<idx>(assocs);
        }

        template <size_t idx>
        void set(const QList<AssocByIndex<idx>>& assocs)
        {
            std::get<idx>(this->assocs) = assocs;
            assocFlags.set(idx*Flag_COUNT + FlagLoaded);
            setDirty<idx>(true);
        }

        template <size_t idx>
        bool drop()
        {
            std::get<idx>(assocs).clear();
            assocFlags.reset(idx*Flag_COUNT + FlagLoaded);
            setDirty<idx>(false);
            return assocFlags.any();
        }

        template <size_t idx>
        void addSingle(const AssocByIndex<idx>& assoc)
        {
            assert(isLoaded<idx>());
            std::get<idx>(assocs) << assoc;
            setDirty<idx>(true);
        }

        template <size_t idx>
        bool removeSingle(const AssocByIndex<idx>& assoc)
        {
            assert(isLoaded<idx>());
            bool removed = std::get<idx>(assocs).removeOne(assoc);
            if (removed) {
                setDirty<idx>(true);
            }
            return removed;
        }

    private:
        std::tuple<QList<AssocsT>...> assocs;
        std::bitset<sizeof...(AssocsT) * Flag_COUNT> assocFlags;
    };

    struct Data : public Traits::Data
    {
        Data() {}
        Data(const Data& o) : Traits::Data(dynamic_cast<const typename Traits::Data&>(o)) {}

        std::weak_ptr<AssocData> assocDataWeakPtr;
    };

public:
    class WeakPtr
    {
    public:
        WeakPtr() {}
        WeakPtr(const DerivedT& item) : d(item.d) {}
        WeakPtr(const WeakPtr& o) : d(o.d) {}

        bool expired() const { return d.expired(); }
        DerivedT lock() const { return DerivedT(d.lock()); }

    private:
        std::weak_ptr<Data> d;
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

    bool operator==(const DerivedT& other) const { return d == other.d; }
    bool operator!=(const DerivedT& other) const { return d != other.d; }

    bool operator<(const DerivedT& other) const { return d < other.d; }
    bool operator<=(const DerivedT& other) const { return d <= other.d; }
    bool operator>(const DerivedT& other) const { return d > other.d; }
    bool operator>=(const DerivedT& other) const { return d >= other.d; }

    template <size_t assocIdx>
    bool isAssocLoaded() const { return a  &&  a->template isLoaded<assocIdx>(); }

    template <size_t assocIdx>
    bool isAssocDirty() const { return isAssocLoaded<assocIdx>()  &&  a->template isDirty<assocIdx>(); }

    template <size_t assocIdx>
    void clearAssocDirty() const
    {
        if (a) {
            a->template setDirty<assocIdx>(false);
        }
    }

    template <size_t assocIdx>
    void markAssocDirty() const
    {
        if (a) {
            a->template setDirty<assocIdx>(true);
        } else {
            auto tmpA = d->assocDataWeakPtr.lock();
            if (tmpA) {
                tmpA->template setDirty<assocIdx>(true);
            }
        }
    }

    void claimAssocs(bool createOnDemand = true)
    {
        if (!a) {
            a = d->assocDataWeakPtr.lock();
            if (!a  &&  createOnDemand) {
                a = std::make_shared<AssocData>();
                d->assocDataWeakPtr = a;
            }
        }
    }

    void dropAssocs() { a.reset(); }

    template <size_t idx>
    void dropAssocs()
    {
        if (a) {
            if (!a->template drop<idx>()) {
                dropAssocs();
            }
        }
    }

    template <size_t assocIdx>
    QList<AssocByIndex<assocIdx>> getAssocs() const
    {
        if (!a) {
            return QList<AssocByIndex<assocIdx>>();
        }
        return a->template get<assocIdx>();
    }

    template <size_t assocIdx>
    QList<typename AssocByIndex<assocIdx>::template Other<DerivedT>> getAssocItems() const
    {
        decltype(getAssocItems<assocIdx>()) conts;
        for (const auto& assoc : getAssocs<assocIdx>()) {
            conts << assoc.getOther(*static_cast<const DerivedT*>(this));
        }
        return conts;
    }

    template <size_t assocIdx>
    AssocByIndex<assocIdx> getAssoc(const typename AssocByIndex<assocIdx>::template Other<DerivedT>& item) const
    {
        const DerivedT* derivedThis = static_cast<const DerivedT*>(this);
        for (const auto& assoc : getAssocs<assocIdx>()) {
            if (assoc.getOther(*derivedThis) == item) {
                return assoc;
            }
        }
        return AssocByIndex<assocIdx>();
    }

    template <size_t assocIdx, typename ItemIterator>
    QList<AssocByIndex<assocIdx>> loadAssocItems(ItemIterator beg, ItemIterator end);

    template <size_t assocIdx, typename ItemIterator>
    QList<AssocByIndex<assocIdx>> addAssocItems(ItemIterator beg, ItemIterator end);

    template <size_t assocIdx>
    AssocByIndex<assocIdx> addAssocItem(const typename AssocByIndex<assocIdx>::template Other<DerivedT>& item)
    {
        return addAssocItems<assocIdx>(&item, &item + 1)[0];
    }

    template <size_t assocIdx, typename ItemIterator>
    int removeAssocItems(ItemIterator beg, ItemIterator end);

    template <size_t assocIdx>
    bool removeAssocItem(const typename AssocByIndex<assocIdx>::template Other<DerivedT>& item)
    {
        return removeAssocItems<assocIdx>(&item, &item + 1) != 0;
    }

    template <size_t assocIdx>
    int clearAssocs();

    template <size_t assocIdx, typename ItemIterator>
    QList<AssocByIndex<assocIdx>> setAssocItems(ItemIterator beg, ItemIterator end);

    template <typename FuncT>
    static void foreachAssocTypeIndex(const FuncT& func)
    {
        foreachAssocTypeIndexImpl(func);
    }

protected:
    AbstractSharedItem() {}
    AbstractSharedItem(const DerivedT& other) : d(other.d), a(other.a) {}
    AbstractSharedItem(const std::shared_ptr<Data>& d) : d(d) {}
    AbstractSharedItem(const DerivedT& other, CloneTag);

    void checkValid() const
    {
        if (!isValid()) {
            throw InvalidStateException("Item is null", __FILE__, __LINE__);
        }
    }

    template <size_t assocIdx>
    void checkAssocLoaded() const
    {
        if (!isAssocLoaded<assocIdx>()) {
            throw InvalidStateException("Assocs must be loaded before they can be modified", __FILE__, __LINE__);
        }
    }

    // Based on https://stackoverflow.com/a/56938140
    // I have no idea what most of this means :-)))
    template <typename FuncT, size_t... indices>
    static void foreachAssocTypeIndexImpl(const FuncT& func, std::index_sequence<indices...>)
    {
        (func(std::integral_constant<size_t, indices>()), ...);
    }
    template <typename FuncT, typename Indices = std::make_index_sequence<std::tuple_size_v<AssocsTuple>>>
    static void foreachAssocTypeIndexImpl(const FuncT& func)
    {
        foreachAssocTypeIndexImpl(func, Indices{});
    }

protected:
    std::shared_ptr<Data> d;
    std::shared_ptr<AssocData> a;
};


}
