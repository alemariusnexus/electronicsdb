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

#include "metatypes.h"

#include <QMetaType>
#include "../model/assoc/PartContainerAssoc.h"
#include "../model/assoc/PartLinkAssoc.h"
#include "../model/container/PartContainer.h"
#include "../model/part/Part.h"
#include "../model/AbstractPartProperty.h"
#include "../model/PartCategory.h"
#include "../model/PartLinkType.h"
#include "../model/PartProperty.h"

namespace electronicsdb
{


void RegisterQtMetaTypes()
{
    qRegisterMetaType<AbstractPartProperty*>();
    qRegisterMetaType<PartCategory*>();
    qRegisterMetaType<PartLinkType*>();
    qRegisterMetaType<PartProperty*>();

    qRegisterMetaType<Part>();
    qRegisterMetaType<Part::QualifiedID>();
    qRegisterMetaType<PartContainer>();
    qRegisterMetaType<PartContainerAssoc>();
    qRegisterMetaType<PartLinkAssoc>();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<AbstractPartProperty*>("AbstractPartProperty*");
    qRegisterMetaTypeStreamOperators<PartCategory*>("PartCategory*");
    qRegisterMetaTypeStreamOperators<PartLinkType*>("PartLinkType*");
    qRegisterMetaTypeStreamOperators<PartProperty*>("PartProperty*");
#endif
}


}
