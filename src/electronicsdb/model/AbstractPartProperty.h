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

#include "../global.h"

#include <QDataStream>
#include <QObject>

namespace electronicsdb
{

class PartCategory;


class AbstractPartProperty : public QObject
{
public:
    AbstractPartProperty();
    AbstractPartProperty(const AbstractPartProperty& other);
    virtual ~AbstractPartProperty();

    virtual int getSortIndexInCategory(PartCategory* pcat) const = 0;
    virtual void setSortIndexInCategory(PartCategory* pcat, int sortIdx) = 0;
};


}

Q_DECLARE_METATYPE(electronicsdb::AbstractPartProperty*)
Q_DECLARE_OPAQUE_POINTER(electronicsdb::AbstractPartProperty*)

// Required for moving pointers around in QAbstractItemView using drag and drop
QDataStream& operator<<(QDataStream& s, const electronicsdb::AbstractPartProperty* prop);
QDataStream& operator>>(QDataStream& s, electronicsdb::AbstractPartProperty*& prop);
