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
#include <QList>
#include <QObject>
#include <QString>
#include <nxcommon/exception/InvalidValueException.h>
#include "../db/SQLDatabaseWrapper.h"
#include "AbstractPartProperty.h"

namespace electronicsdb
{

class Part;
class PartCategory;


class PartLinkType : public AbstractPartProperty
{
    Q_OBJECT

public:
    enum Flags
    {
        ShowInA = 0x0001,
        ShowInB = 0x0002,
        EditInA = 0x0004,
        EditInB = 0x0008,
        DisplaySelectionListA = 0x0010,
        DisplaySelectionListB = 0x0020,

        // Default state when flags are not overridden by database settings, i.e. when DB flags are NULL
        Default = 0,

        // Default for newly created PartLinkTypes (e.g. in the static model editing GUI)
        DefaultForNew = ShowInA | ShowInB | EditInA | EditInB | DisplaySelectionListA | DisplaySelectionListB
    };

    enum PatternType
    {
        PatternTypeBothPositive,
        PatternTypeANegative,
        PatternTypeBNegative
    };

    static bool isValidID(const QString& id);

    static bool checkPattern(const QList<PartCategory*>& a, const QList<PartCategory*>& b, PatternType type,
            QString* outErrmsg);

    static QStringList generatePatternSQLUpdateCode (
            SQLDatabaseWrapper* dbw,
            const QList<QString>& removedCatIDs,
            const QMap<QString, QString>& editedCatIDs);

public:
    PartLinkType (
            const QString& id,
            const QList<PartCategory*>& pcatsPatternA,
            const QList<PartCategory*>& pcatsPatternB,
            PatternType patternType);

    PartLinkType* clone() const;
    PartLinkType* cloneWithMappedCategories(const QMap<PartCategory*, PartCategory*>& pcatMap) const;

    QString getID() const { return id; }
    void setID(const QString& id);

    const QList<PartCategory*>& getPatternPartCategoriesA() const { return patternPcatsA; }
    const QList<PartCategory*>& getPatternPartCategoriesB() const { return patternPcatsB; }
    PatternType getPatternType() const { return patternType; }

    void setPatternPartCategories(const QList<PartCategory*>& a, const QList<PartCategory*>& b, PatternType type);

    QList<PartCategory*> getPartCategoriesA(const QList<PartCategory*>& allPcats) const;
    QList<PartCategory*> getPartCategoriesB(const QList<PartCategory*>& allPcats) const;

    QString getNameA() const { return nameA; }
    QString getNameB() const { return nameB; }
    void setNameA(const QString& name) { nameA = name; }
    void setNameB(const QString& name) { nameB = name; }

    int getFlags() const { return flags; }
    int getFlagsForCategory(PartCategory* pcat) const;
    void setFlags(int flags) { this->flags = flags; }

    bool isPartCategoryA(PartCategory* pcat) const;
    bool isPartCategoryB(PartCategory* pcat) const;
    bool isPartCategoryAny(PartCategory* pcat) const;
    bool isPartA(const Part& part) const;
    bool isPartB(const Part& part) const;
    bool isPartAny(const Part& part) const;

    QString getPatternDescription() const;

    int getSortIndexInCategory(PartCategory* pcat) const override;
    void setSortIndexInCategory(PartCategory* pcat, int sortIdx) override;

    QMap<PartCategory*, int> getSortIndicesForCategories() const;

private:
    PartLinkType(const PartLinkType& other);

    static void checkValidID(const QString& id)
    {
        if (!isValidID(id)) {
            throw InvalidValueException("Invalid part link type ID", __FILE__, __LINE__);
        }
    }

    bool matchesPattern(PartCategory* pcat, const QList<PartCategory*>& pattern, bool positive) const;

private:
    QString id;
    QList<PartCategory*> patternPcatsA;
    QList<PartCategory*> patternPcatsB;
    PatternType patternType;
    QString nameA;
    QString nameB;
    int flags;
    QMap<PartCategory*, int> sortIndicesByPcat;
};


}

Q_DECLARE_METATYPE(electronicsdb::PartLinkType*)
Q_DECLARE_OPAQUE_POINTER(electronicsdb::PartLinkType*)

// Required for moving pointers around in QAbstractItemView using drag and drop
QDataStream& operator<<(QDataStream& s, const electronicsdb::PartLinkType* ltype);
QDataStream& operator>>(QDataStream& s, electronicsdb::PartLinkType*& ltype);
