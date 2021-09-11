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
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <nxcommon/exception/InvalidStateException.h>
#include "../abstract/AbstractSharedItem.h"
#include "../assoc/PartContainerAssoc.h"

namespace electronicsdb
{

class PartContainer;

namespace detail
{

template <>
struct AbstractSharedItemTraits<class electronicsdb::PartContainer>
{
    struct Data
    {
    public:
        Data() : cid(-1), flags(0) {}
        Data(const Data& other) : cid(-1), flags(other.flags), name(other.name) {}
        ~Data();

        void init(dbid_t cid) { this->cid = cid; }
        void init(dbid_t cid, const QString& name) { this->cid = cid; this->name = name; }

        dbid_t cid;
        uint8_t flags;

        QString name;
    };
};

}


class PartContainer : public AbstractSharedItem<PartContainer, PartContainerAssoc>
{
    friend class PartContainerFactory;

public:
    using Base = AbstractSharedItem<PartContainer, PartContainerAssoc>;

    static constexpr size_t PartContainerAssocIdx = 0;

    struct CreateBlankTag {};

private:
    enum Flags
    {
        FlagDirty = 0x01        // This only signifies if the base data is dirty, i.e. ID and/or name.
                                // The assocs each have their own dirty flag.
    };

public:
    PartContainer() {}
    PartContainer(CreateBlankTag) : Base(std::make_shared<Data>()) {}
    PartContainer(const PartContainer& other) : Base(other) {}
    PartContainer(const std::shared_ptr<Data>& d) : Base(d) {}

    dbid_t getID() const { checkValid(); return d->cid; }
    void setID(dbid_t id);
    bool hasID() const { return getID() > 0; }

    bool isNameLoaded() const { checkValid(); return !d->name.isNull(); }
    QString getName() const;
    void setName(const QString& name) { checkValid(); d->name = name; d->flags |= FlagDirty; }

    void clearDirty() const { checkValid(); const_cast<PartContainer*>(this)->d->flags &= ~FlagDirty; }
    bool isDirty() const { checkValid(); return (d->flags & FlagDirty) != 0; }

    EDB_ABSTRACTSHAREDITEM_DECLARE_ASSOCMETHODS(PartContainerAssocIdx, PartContainer, Part, PartContainerAssoc, Part, Parts)

    PartContainer clone() const;

private:
    // These shouldn't be called by user code. Instead, use PartContainerFactory::findContainerByID().
    PartContainer(dbid_t cid) : Base(std::make_shared<Data>()) { d->init(cid); }
    PartContainer(dbid_t cid, const QString& name) : Base(std::make_shared<Data>()) { d->init(cid, name); }

    PartContainer(const PartContainer& other, const CloneTag&);

    void checkDatabaseConnection() const;
};

using PartContainerList = QList<PartContainer>;
using PartContainerIterator = PartContainerList::iterator;
using CPartContainerIterator = PartContainerList::const_iterator;


}

Q_DECLARE_METATYPE(electronicsdb::PartContainer)
