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

#include "PartContainerAssoc.h"

#include "../../System.h"
#include "../container/PartContainer.h"
#include "../part/Part.h"
#include "../PartCategory.h"

namespace electronicsdb
{


const Part& PartContainerAssoc::getPart() const
{
    return getItemA();
}

const PartContainer& PartContainerAssoc::getContainer() const
{
    return getItemB();
}

void PartContainerAssoc::buildPersistData(QList<QMap<QString, QVariant>>& outData) const
{
    const Part& part = getPart();
    const PartContainer& cont = getContainer();

    QMap<QString, QVariant> data;
    data["ptype"] = part.getPartCategory()->getID();
    data["pid"] = part.getID();
    data["cid"] = cont.getID();
    outData << data;
}


}
