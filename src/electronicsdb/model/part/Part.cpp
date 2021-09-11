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

#include "Part.h"

#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <nxcommon/log.h>
#include "../../System.h"
#include "../container/PartContainer.h"
#include "../PartCategory.h"
#include "../PartLinkType.h"
#include "../PartProperty.h"
#include "PartFactory.h"

namespace electronicsdb
{


Part::Traits::Data::~Data()
{
    if (pid >= 0) {
        PartFactory::getInstance().unregisterPart(cat, pid);
    }
}


Part::QualifiedID::QualifiedID(const Part& part)
        : pcat(part ? const_cast<PartCategory*>(part.getPartCategory()) : nullptr), pid(part ? part.getID() : -1)
{
}

Part Part::QualifiedID::findPart() const
{
    return pcat ? PartFactory::getInstance().findPartByID(pcat, pid) : Part();
}


Part::Part(const Part& other, const CloneTag&)
        : Base(other, CloneTag())
{
    d->flags |= FlagDirty;
}

void Part::setID(dbid_t pid)
{
    checkValid();
    if (pid == d->pid) {
        return;
    }
    if (hasID()) {
        throw InvalidStateException("Part ID already set", __FILE__, __LINE__);
    }
    d->pid = pid;
    d->flags |= FlagDirty;
    PartFactory::getInstance().registerPart(*this);
}

bool Part::isDataLoaded(const PropertyList& props) const
{
    checkValid();
    if (d->data.size() < props.size()) {
        return false;
    }
    if (isDataFullyLoaded()) {
        return true;
    }
    for (PartProperty* prop : props) {
        assert(prop->getPartCategory() == d->cat);
        if (!d->data.contains(prop)) {
            return false;
        }
    }
    return true;
}

bool Part::isDataLoaded(PartProperty* prop) const
{
    checkValid();
    assert(prop->getPartCategory() == d->cat);
    return d->data.contains(prop);
}

bool Part::isDataFullyLoaded() const
{
    checkValid();
    return d->data.size() == d->cat->getPropertyCount();
}

const QVariant& Part::getData(PartProperty* prop) const
{
    checkValid();
    assert(prop);
    assert(prop->getPartCategory() == d->cat);
    assureDataLoaded(prop);
    return d->data[prop];
}

void Part::setData(PartProperty* prop, const QVariant& value)
{
    checkValid();
    assert(prop);
    assert(prop->getPartCategory() == d->cat);
    d->data[prop] = value;
    d->description = QString();
    d->flags |= FlagDirty;
}

QString Part::getDescription() const
{
    checkValid();
    if (!d->description.isNull()) {
        return d->description;
    }

    try {
        assureDataLoaded(d->cat->getDescriptionProperties());
    } catch (std::exception&) {
        return QCoreApplication::tr("(Invalid)");
    }

    QString desc = d->cat->getDescriptionPattern();

    if (!desc.isNull()) {
        QMap<QString, QString> vars;
        vars["id"] = QString::number(d->pid);

        for (PartProperty* prop : d->cat->getDescriptionProperties()) {
            QString descStr = prop->formatValueForDisplay(getData(prop));
            vars[prop->getFieldName()] = descStr;
        }

        desc = ReplaceStringVariables(desc, vars);
    } else {
        d->description = QString::number(d->pid);
    }

    d->description = desc;

    return desc;
}

PartLinkAssoc Part::linkPart(const Part& part, const QSet<PartLinkType*>& types)
{
    PartLinkAssoc assoc = addLinkedPart(part);
    for (PartLinkType* type : types) {
        assoc.addLinkType(type);
    }
    return assoc;
}

bool Part::unlinkPart(const Part& part)
{
    return removeLinkedPart(part);
}

void Part::unlinkPart(const Part& part, const QSet<PartLinkType*>& types)
{
    PartLinkAssoc assoc = getLinkedPartAssoc(part);
    if (assoc) {
        for (PartLinkType* type : types) {
            assoc.removeLinkType(type);
        }

        if (assoc.getLinkTypes().empty()) {
            unlinkPart(part);
        }
    }
}

void Part::unlinkPartLinkTypes(const QSet<PartLinkType*>& types)
{
    PartList lparts = getLinkedParts();
    for (const Part& lpart : lparts) {
        unlinkPart(lpart, types);
    }
}

Part Part::clone() const
{
    return Part(*this, CloneTag());
}

void Part::buildPersistData (
        PersistOperation op,
        QMap<QString, QList<QMap<QString, QVariant>>>& outInsData,
        QMap<QString, QList<QMap<QString, QVariant>>>& outUpdData,
        QMap<QString, QList<QMap<QString, QVariant>>>& outDelData,
        QMap<QString, QString>& outTableIDs
) const {
    checkValid();

    if (op == PersistInsert  ||  (op == PersistUpdate  &&  isDirty())) {
        // Build base data (the pcat_* table)
        QMap<QString, QVariant> bdata;
        if (hasID()) {
            bdata[d->cat->getIDField()] = d->pid;
        }
        for (auto it = d->data.begin() ; it != d->data.end() ; it++) {
            PartProperty* prop = it.key();
            if ((prop->getFlags() & PartProperty::MultiValue) == 0) {
                bdata[prop->getFieldName()] = it.value();
            }
        }
        if (op == PersistInsert) {
            outInsData[d->cat->getTableName()] << bdata;
        } else {
            outUpdData[d->cat->getTableName()] << bdata;
        }
        outTableIDs[d->cat->getTableName()] = d->cat->getIDField();

        // Build multi-value data
        for (auto it = d->data.begin() ; it != d->data.end() ; it++) {
            PartProperty* prop = it.key();
            if ((prop->getFlags() & PartProperty::MultiValue) != 0) {
                const QVariant& multiValue = it.value();
                auto& mvInsData = outInsData[prop->getMultiValueForeignTableName()];

                QMap<QString, QVariant> mvData;
                mvData[prop->getMultiValueIDFieldName()] = d->pid;

                for (const QVariant& value : multiValue.toList()) {
                    mvData[prop->getFieldName()] = value;
                    mvInsData << mvData;
                }
            }
        }
    }

    if (op == PersistDelete) {
        if (hasID()) {
            QMap<QString, QVariant> bdata;
            bdata[d->cat->getIDField()] = d->pid;
            outDelData[d->cat->getTableName()] << bdata;
        }
    }
}

void Part::loadData(const PropertyList& props) const
{
    if (!hasID()) {
        throw InvalidStateException("Attempted to load data just-in-time for part with invalid ID", __FILE__, __LINE__);
    }
    Part* ncThis = const_cast<Part*>(this);
    LogWarning("Part data (%s:%d) loaded just-in-time. This is slow. Did you forget to bulk-load?",
            d->cat->getID().toUtf8().constData(), d->pid);
    PartFactory::getInstance().loadItemsSingleCategory(ncThis, ncThis+1, 0, props);
    if (!isDataLoaded(props)) {
        throw InvalidValueException("Failed to load part data. Maybe the part doesn't exist?");
    }
}


EDB_ABSTRACTSHAREDITEM_DEFINE_ASSOCMETHODS(PartContainerAssocIdx, Part, PartContainer, PartContainerAssoc, Container, Containers)
EDB_ABSTRACTSHAREDITEM_DEFINE_ASSOCMETHODS(PartLinkAssocIdx, Part, Part, PartLinkAssoc, LinkedPart, LinkedParts)


}
