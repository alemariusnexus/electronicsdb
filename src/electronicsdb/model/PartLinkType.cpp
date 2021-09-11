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

#include "PartLinkType.h"

#include <QRegularExpression>
#include <QSet>
#include <nxcommon/exception/InvalidValueException.h>
#include "../System.h"
#include "part/Part.h"
#include "PartCategory.h"

namespace electronicsdb
{


bool PartLinkType::isValidID(const QString& id)
{
    static QRegularExpression regex("^[a-zA-Z0-9_]+$");
    static QSet<QString> reservedIDs({});
    return regex.match(id).hasMatch()  &&  !reservedIDs.contains(id)  &&  !id.startsWith("__");
}

bool PartLinkType::checkPattern (
        const QList<PartCategory*>& a, const QList<PartCategory*>& b, PatternType type, QString* outErrmsg
) {
    // NOTE: Currently, a and b must be disjoint (i.e. no PartCategory may appear in both of them). This is because a
    // part's category is used to determine uniquely whether the part plays the role of A or B, which would otherwise
    // be ambiguous.
    // TODO: Remove this limitation. It prevents us from creating link types like "see also" that can link parts of
    //       any two categories together (even the same category).

    if (type == PatternTypeBothPositive) {
        QStringList overlappingIDs;
        for (PartCategory* pcat : a) {
            if (b.contains(pcat)) {
                overlappingIDs << pcat->getID();
            }
        }
        if (!overlappingIDs.empty()) {
            if (outErrmsg) {
                *outErrmsg = tr("Overlapping categories: %1").arg(overlappingIDs.join(", "));
            }
            return false;
        }
    } else {
        const QList<PartCategory*>* pos;
        const QList<PartCategory*>* neg;

        if (type == PatternTypeANegative) {
            pos = &b;
            neg = &a;
        } else {
            pos = &a;
            neg = &b;
        }

        QStringList overlappingIDs;

        for (PartCategory* p : *pos) {
            if (!neg->contains(p)) {
                overlappingIDs << p->getID();
            }
        }

        if (!overlappingIDs.empty()) {
            if (outErrmsg) {
                *outErrmsg = tr("Overlapping categories: %1").arg(overlappingIDs.join(", "));
            }
            return false;
        }
    }

    return true;
}

QStringList PartLinkType::generatePatternSQLUpdateCode (
        SQLDatabaseWrapper* dbw,
        const QList<QString>& removedCatIDs,
        const QMap<QString, QString>& editedCatIDs
) {
    QStringList stmts;

    if (!removedCatIDs.empty()) {
        QStringList pcatIDs;
        for (QString id : removedCatIDs) {
            pcatIDs << dbw->escapeString(id);
        }
        stmts << QString("DELETE FROM partlink_type_patternpcat WHERE pcat IN (%1);").arg(pcatIDs.join(", "));
        stmts << QString("DELETE FROM partlink_type_pcat WHERE pcat IN (%1);").arg(pcatIDs.join(", "));
    }

    if (!editedCatIDs.empty()) {
        QStringList caseParts;
        for (auto it = editedCatIDs.begin() ; it != editedCatIDs.end() ; ++it) {
            if (it.key() == it.value()) {
                continue;
            }
            caseParts << QString("WHEN %1 THEN %2").arg(dbw->escapeString(it.key()), dbw->escapeString(it.value()));
        }
        if (!caseParts.empty()) {
            stmts << QString("UPDATE partlink_type_patternpcat SET pcat=(CASE pcat %1 ELSE pcat END);")
                    .arg(caseParts.join(" "));
            stmts << QString("UPDATE partlink_type_pcat SET pcat=(CASE pcat %1 ELSE pcat END);")
                    .arg(caseParts.join(" "));
        }
    }

    return stmts;
}



PartLinkType::PartLinkType (
        const QString& id,
        const QList<PartCategory*>& pcatsPatternA,
        const QList<PartCategory*>& pcatsPatternB,
        PatternType patternType
) : id(id), flags(0)
{
    checkValidID(id);
    setPatternPartCategories(pcatsPatternA, pcatsPatternB, patternType);
}

PartLinkType::PartLinkType(const PartLinkType& other)
        : id(other.id), patternPcatsA(other.patternPcatsA), patternPcatsB(other.patternPcatsB),
          patternType(other.patternType), nameA(other.nameA), nameB(other.nameB),
          flags(other.flags), sortIndicesByPcat(other.sortIndicesByPcat)
{
}

PartLinkType* PartLinkType::clone() const
{
    return new PartLinkType(*this);
}

PartLinkType* PartLinkType::cloneWithMappedCategories(const QMap<PartCategory*, PartCategory*>& pcatMap) const
{
    PartLinkType* cloned = clone();
    QList<PartCategory*> mappedPatternPcatsA;
    QList<PartCategory*> mappedPatternPcatsB;

    for (PartCategory* pcat : patternPcatsA) {
        mappedPatternPcatsA << pcatMap[pcat];
    }
    for (PartCategory* pcat : patternPcatsB) {
        mappedPatternPcatsB << pcatMap[pcat];
    }

    cloned->setPatternPartCategories(mappedPatternPcatsA, mappedPatternPcatsB, cloned->patternType);

    cloned->sortIndicesByPcat.clear();
    for (auto it = sortIndicesByPcat.begin() ; it != sortIndicesByPcat.end() ; ++it) {
        cloned->sortIndicesByPcat[pcatMap[it.key()]] = it.value();
    }

    return cloned;
}

void PartLinkType::setID(const QString& id)
{
    checkValidID(id);
    this->id = id;
}

void PartLinkType::setPatternPartCategories (
        const QList<PartCategory*>& a, const QList<PartCategory*>& b, PatternType type
) {
    QString errmsg;
    if (!checkPattern(a, b, type, &errmsg)) {
        throw InvalidValueException(QString("Invalid part link type pattern: %1").arg(errmsg).toUtf8().constData(),
                                    __FILE__, __LINE__);
    }

    patternPcatsA = a;
    patternPcatsB = b;
    patternType = type;
}

QList<PartCategory*> PartLinkType::getPartCategoriesA(const QList<PartCategory*>& allPcats) const
{
    QList<PartCategory*> matches;
    for (PartCategory* pcat : allPcats) {
        if (isPartCategoryA(pcat)) {
            matches << pcat;
        }
    }
    return matches;
}

QList<PartCategory*> PartLinkType::getPartCategoriesB(const QList<PartCategory*>& allPcats) const
{
    QList<PartCategory*> matches;
    for (PartCategory* pcat : allPcats) {
        if (isPartCategoryB(pcat)) {
            matches << pcat;
        }
    }
    return matches;
}

int PartLinkType::getFlagsForCategory(PartCategory* pcat) const
{
    if (isPartCategoryA(pcat)) {
        return flags & ~(ShowInB | EditInB | DisplaySelectionListB);
    } else {
        return flags & ~(ShowInA | EditInA | DisplaySelectionListA);
    }
}

bool PartLinkType::isPartCategoryA(PartCategory* pcat) const
{
    return matchesPattern(pcat, patternPcatsA, (patternType != PatternTypeANegative));
}

bool PartLinkType::isPartCategoryB(PartCategory* pcat) const
{
    return matchesPattern(pcat, patternPcatsB, (patternType != PatternTypeBNegative));
}

bool PartLinkType::isPartCategoryAny(PartCategory* pcat) const
{
    return isPartCategoryA(pcat)  ||  isPartCategoryB(pcat);
}

bool PartLinkType::isPartA(const Part& part) const
{
    return isPartCategoryA(const_cast<PartCategory*>(part.getPartCategory()));
}

bool PartLinkType::isPartB(const Part& part) const
{
    return isPartCategoryB(const_cast<PartCategory*>(part.getPartCategory()));
}

bool PartLinkType::isPartAny(const Part& part) const
{
    return isPartA(part)  ||  isPartB(part);
}

bool PartLinkType::matchesPattern(PartCategory* pcat, const QList<PartCategory*>& pattern, bool positive) const
{
    return pattern.contains(pcat) == positive;
}

QString PartLinkType::getPatternDescription() const
{
    QStringList aIDs;
    QStringList bIDs;

    for (PartCategory* pcat : patternPcatsA) {
        aIDs << pcat->getID();
    }
    for (PartCategory* pcat : patternPcatsB) {
        bIDs << pcat->getID();
    }

    return QString("A: %1%2 | B: %3%4").arg((patternType == PatternTypeANegative) ? "-" : "",
                                            aIDs.join(","),
                                            (patternType == PatternTypeBNegative) ? "-" : "",
                                            bIDs.join(","));
}

int PartLinkType::getSortIndexInCategory(PartCategory* pcat) const
{
    auto it = sortIndicesByPcat.find(pcat);
    if (it == sortIndicesByPcat.end()) {
        return 10000000;
    }
    return it.value();
}

QMap<PartCategory*, int> PartLinkType::getSortIndicesForCategories() const
{
    return sortIndicesByPcat;
}

void PartLinkType::setSortIndexInCategory(PartCategory* pcat, int sortIdx)
{
    sortIndicesByPcat[pcat] = sortIdx;
}


}


QDataStream& operator<<(QDataStream& s, const electronicsdb::PartLinkType* ltype)
{
    return s << reinterpret_cast<quint64>(ltype);
}

QDataStream& operator>>(QDataStream& s, electronicsdb::PartLinkType*& ltype)
{
    quint64 ptr;
    s >> ptr;
    ltype = reinterpret_cast<electronicsdb::PartLinkType*>(ptr);
    return s;
}
