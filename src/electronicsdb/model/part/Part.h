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
#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVariant>
#include <nxcommon/exception/InvalidStateException.h>
#include "../abstract/AbstractSharedItem.h"
#include "../assoc/PartContainerAssoc_Def.h"
#include "../assoc/PartLinkAssoc_Def.h"

namespace electronicsdb
{

class Part;
class PartCategory;
class PartContainer;
class PartLinkType;
class PartProperty;

namespace detail
{

template <>
struct AbstractSharedItemTraits<class electronicsdb::Part>
{
    struct Data
    {
    public:
        Data() : cat(nullptr), pid(-1), flags(0) {}
        Data(const Data& o) : cat(o.cat), pid(-1), flags(o.flags), data(o.data) {}
        ~Data();

        void init(PartCategory* cat, dbid_t pid) { this->cat = cat; this->pid = pid; }
        void init(PartCategory* cat) { this->cat = cat; }

        PartCategory* cat;
        dbid_t pid;
        uint8_t flags;

        QMap<PartProperty*, QVariant> data;
        mutable QString description;
    };
};

}


class Part : public AbstractSharedItem<Part, PartContainerAssoc, PartLinkAssoc>
{
    friend class PartFactory;

public:
    using Base = AbstractSharedItem<Part, PartContainerAssoc, PartLinkAssoc>;

    static constexpr size_t PartContainerAssocIdx = 0;
    static constexpr size_t PartLinkAssocIdx = 1;

    using DataMap = QMap<PartProperty*, QVariant>;
    using PropertyList = QList<PartProperty*>;

    struct CreateBlankTag {};

    class QualifiedID
    {
    public:
        QualifiedID() : pcat(nullptr), pid(-1) {}
        QualifiedID(PartCategory* pcat, dbid_t pid) : pcat(pcat), pid(pid) {}
        QualifiedID(const QualifiedID& other) : pcat(other.pcat), pid(other.pid) {}
        QualifiedID(const Part& part);

        bool isNull() const { return pcat == nullptr; }
        bool isValid() const { return !isNull(); }
        operator bool() const { return isValid(); }

        Part findPart() const;

        bool operator==(const QualifiedID& other)
                { return (!pcat  &&  !other.pcat)  ||  (pcat == other.pcat  &&  pid == other.pid); }
        bool operator!=(const QualifiedID& other) { return !(*this == other); }

    private:
        PartCategory* pcat;
        dbid_t pid;
    };

private:
    enum Flags
    {
        FlagDirty = 0x01        // This only signifies if the base data is dirty, i.e. the property data.
                                // The assocs each have their own dirty flag.
    };

public:
    Part() : Base() {}
    Part(PartCategory* pcat, CreateBlankTag) : Base(std::make_shared<Data>()) { d->init(pcat); }
    Part(const Part& other) : Base(other) {}
    Part(const std::shared_ptr<Data>& d) : Base(d) {}

    PartCategory* getPartCategory() { checkValid(); return d->cat; }
    const PartCategory* getPartCategory() const { checkValid(); return d->cat; }

    dbid_t getID() const { checkValid(); return d->pid; }
    void setID(dbid_t pid);
    bool hasID() const { return getID() > 0; }

    QualifiedID getQualifiedID() const { return QualifiedID(*this); }

    bool isDataLoaded(const PropertyList& props) const;
    bool isDataLoaded(PartProperty* prop) const;
    bool isDataFullyLoaded() const;

    const QVariant& getData(PartProperty* prop) const;
    const DataMap& getData() const { checkValid(); return d->data; }
    void setData(PartProperty* prop, const QVariant& value);
    void setData(const DataMap& data)
            { checkValid(); d->data = data; d->description = QString(); d->flags |= FlagDirty; }

    void clearDirty() const { checkValid(); const_cast<Part*>(this)->d->flags &= ~FlagDirty; }
    bool isDirty() const { checkValid(); return (d->flags & FlagDirty) != 0; }

    QString getDescription() const;

    EDB_ABSTRACTSHAREDITEM_DECLARE_ASSOCMETHODS(PartContainerAssocIdx, Part, PartContainer, PartContainerAssoc, Container, Containers)
    EDB_ABSTRACTSHAREDITEM_DECLARE_ASSOCMETHODS(PartLinkAssocIdx, Part, Part, PartLinkAssoc, LinkedPart, LinkedParts)

    PartLinkAssoc linkPart(const Part& part, const QSet<PartLinkType*>& types);

    bool unlinkPart(const Part& part);
    void unlinkPart(const Part& part, const QSet<PartLinkType*>& types);
    void unlinkPartLinkTypes(const QSet<PartLinkType*>& types);

    Part clone() const;

    void buildPersistData (
            PersistOperation op,
            QMap<QString, QList<QMap<QString, QVariant>>>& outInsData,
            QMap<QString, QList<QMap<QString, QVariant>>>& outUpdData,
            QMap<QString, QList<QMap<QString, QVariant>>>& outDelData,
            QMap<QString, QString>& outTableIDs
            ) const;

private:
    // This shouldn't be called by user code. Instead, use PartFactory::findPartByID().
    Part(PartCategory* cat, dbid_t pid) : Base(std::make_shared<Data>()) { d->init(cat, pid); }

    Part(const Part& other, const CloneTag&);

    void assureDataLoaded(const PropertyList& props) const
    {
        if (!isDataLoaded(props)) {
            loadData(props);
        }
    }
    void assureDataLoaded(PartProperty* prop) const
    {
        if (!isDataLoaded(prop)) {
            loadData(prop);
        }
    }

    void loadData(const PropertyList& props) const;
    void loadData(PartProperty* prop) const { PropertyList l; l << prop; loadData(l); }
};


using PartList = QList<Part>;
using PartIterator = PartList::iterator;
using CPartIterator = PartList::const_iterator;


}

Q_DECLARE_METATYPE(electronicsdb::Part)
Q_DECLARE_METATYPE(electronicsdb::Part::QualifiedID)
