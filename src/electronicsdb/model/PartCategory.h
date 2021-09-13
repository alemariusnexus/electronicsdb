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
#include <QIODevice>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include "../db/DatabaseConnection.h"
#include "../util/Filter.h"
#include "part/Part.h"
#include "PartProperty.h"

using Xapian::WritableDatabase;

namespace electronicsdb
{

class PartLinkType;


class PartCategory : public QObject
{
    Q_OBJECT

    friend class PartProperty;

public:
    typedef QList<PartProperty*> PropertyList;
    typedef PropertyList::iterator PropertyIterator;
    typedef PropertyList::const_iterator ConstPropertyIterator;

public:
    static bool isValidID(const QString& id);

public:
    PartCategory(const QString& id, const QString& userReadableName, const QString& idField = "id");
    ~PartCategory();

    PartCategory* clone(const QMap<PartPropertyMetaType*, PartPropertyMetaType*>& mtypeMap = {}) const;

    void addProperty(PartProperty* prop);
    bool removeProperty(PartProperty* prop);

    void setDescriptionPattern(const QString& descPat);
    QString getDescriptionPattern() const { return descPattern; }
    int getSortIndex() const { return sortIdx; }
    void setSortIndex(int idx) { sortIdx = idx; }

    void setID(const QString& id) { this->id = id; }
    QString getID() const { return id; }

    void setUserReadableName(const QString& name) { this->userReadableName = name; }
    QString getUserReadableName() const { return userReadableName; }

    QString getTableName() const { return QString("pcat_") + id; }
    QString getIDField() const { return idField; }

    PropertyIterator getPropertyBegin() { return props.begin(); }
    PropertyIterator getPropertyEnd() { return props.end(); }
    ConstPropertyIterator getPropertyBegin() const { return props.begin(); }
    ConstPropertyIterator getPropertyEnd() const { return props.end(); }
    const PropertyList& getProperties() const { return props; }
    size_t getPropertyCount() const { return props.size(); }
    PartProperty* getProperty(const QString& fieldName);
    PropertyList getDescriptionProperties();

    void rebuildFullTextIndex() { rebuildFullTextIndex(true, PartList()); }
    void updateFullTextIndex(const PartList& parts) { rebuildFullTextIndex(false, parts); }
    void updateFullTextIndex(const Part& part) { updateFullTextIndex(PartList() << part); }
    void clearFullTextIndex();

    PartList find(Filter* filter, size_t offset, size_t numResults);
    size_t countMatches(Filter* filter);

    QList<PartLinkType*> getPartLinkTypes() const;
    QList<AbstractPartProperty*> getAbstractProperties(const QList<PartLinkType*>& allLtypes) const;

private:
    PartCategory(const PartCategory& other, const QMap<PartPropertyMetaType*, PartPropertyMetaType*>& mtypeMap);

    void rebuildFullTextIndex(bool fullRebuild, const PartList& parts);
    PartList findInternal(Filter* filter, size_t offset, size_t numResults, size_t* countOnlyOut = nullptr);

    void notifyPropertyOrderChanged();

private:
    QString id;
    QString userReadableName;
    QString idField;
    QString descPattern;
    int sortIdx;

    WritableDatabase xapianDb;

    PropertyList props;

    PropertyList descProps;
    bool descPropsDirty;
};


}

Q_DECLARE_METATYPE(electronicsdb::PartCategory*)
Q_DECLARE_OPAQUE_POINTER(electronicsdb::PartCategory*)

// Required for moving pointers around in QAbstractItemView using drag and drop
QDataStream& operator<<(QDataStream& s, const electronicsdb::PartCategory* pcat);
QDataStream& operator>>(QDataStream& s, electronicsdb::PartCategory*& pcat);
