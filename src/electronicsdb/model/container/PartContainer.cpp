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

#include "PartContainer.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/log.h>
#include "../part/Part.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../System.h"
#include "../PartCategory.h"
#include "PartContainerFactory.h"

namespace electronicsdb
{


PartContainer::Traits::Data::~Data()
{
    if (cid >= 0) {
        PartContainerFactory::getInstance().unregisterContainer(cid);
    }
}


PartContainer::PartContainer(const PartContainer& other, const CloneTag&)
        : Base(other, CloneTag())
{
    d->flags |= FlagDirty;
}

void PartContainer::setID(dbid_t id)
{
    if (hasID()) {
        throw InvalidStateException("PartContainer ID already set", __FILE__, __LINE__);
    }
    d->cid = id;
    d->flags |= FlagDirty;
    PartContainerFactory::getInstance().registerContainer(*this);
}

QString PartContainer::getName() const
{
    checkValid();
    if (!isNameLoaded()) {
        LogWarning("Loading container name just-in-time. This is slow. Did you forget to bulk-load?");
        PartContainerFactory::getInstance().loadItems(*const_cast<PartContainer*>(this));
    }
    assert(isNameLoaded());
    return d->name;
}

PartContainer PartContainer::clone() const
{
    return PartContainer(*this, CloneTag());
}

void PartContainer::checkDatabaseConnection() const
{
    if (!System::getInstance()->hasValidDatabaseConnection()) {
        throw NoDatabaseConnectionException("No database connection for PartContainer", __FILE__, __LINE__);
    }
}


EDB_ABSTRACTSHAREDITEM_DEFINE_ASSOCMETHODS(PartContainerAssocIdx, PartContainer, Part, PartContainerAssoc, Part, Parts)


}
