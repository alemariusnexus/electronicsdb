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

#include "PartCategory.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextStream>
#include <QVariant>
#include <nxcommon/exception/InvalidValueException.h>
#include "../db/SQLDatabaseWrapperFactory.h"
#include "../exception/NoDatabaseConnectionException.h"
#include "../exception/SQLException.h"
#include "../util/elutil.h"
#include "../System.h"
#include "part/Part.h"
#include "part/PartFactory.h"
#include "PartLinkType.h"

using Xapian::TermGenerator;
using Xapian::Document;
using Xapian::QueryParser;
using Xapian::Query;
using Xapian::Enquire;
using Xapian::MSet;
using Xapian::MSetIterator;
using Xapian::Utf8Iterator;


#define MAX_INDEX_REBUILD_FETCH_ROWS 1000
#define FILTER_SEARCH_BLOCK_SIZE 50000
#define FILTER_SEARCH_SQL_BLOCK_SIZE 50000


namespace electronicsdb
{

static_assert(static_cast<uint64_t>(std::numeric_limits<Xapian::docid>::max())
              >= static_cast<uint64_t>(std::numeric_limits<dbid_t>::max()),
        "Xapian docid type is too small for dbid_t. Did you forget to build Xapian with --enable-64bit-docid?");


bool PartCategory::isValidID(const QString& id)
{
    static QRegularExpression regex("^[a-zA-Z0-9_]+$");
    static QSet<QString> reservedIDs({});
    return regex.match(id).hasMatch()  &&  !reservedIDs.contains(id)  &&  !id.startsWith("__");
}



PartCategory::PartCategory(const QString& id, const QString& userReadableName, const QString& idField)
        : id(id), userReadableName(userReadableName), idField(idField), sortIdx(10000),
          xapianDb("", Xapian::DB_BACKEND_INMEMORY),
          descPropsDirty(false)
{
    if (!isValidID(id)) {
        throw InvalidValueException(QString("Invalid part category ID: %1").arg(id).toUtf8().constData(),
                                    __FILE__, __LINE__);
    }
}

PartCategory::PartCategory(const PartCategory& other, const QMap<PartPropertyMetaType*, PartPropertyMetaType*>& mtypeMap)
        : id(other.id), userReadableName(other.userReadableName), idField(other.idField),
          descPattern(other.descPattern), sortIdx(other.sortIdx), xapianDb("", Xapian::DB_BACKEND_INMEMORY),
          descPropsDirty(other.descPropsDirty)
{
    for (PartProperty* otherProp : other.props) {
        props << new PartProperty(*otherProp, this, mtypeMap);
    }
    for (PartProperty* otherDescProp : other.descProps) {
        for (PartProperty* prop : props) {
            if (prop->getFieldName() == otherDescProp->getFieldName()) {
                descProps << prop;
                break;
            }
        }
    }
}

PartCategory::~PartCategory()
{
    for (PartProperty* prop : props) {
        delete prop;
    }
}

PartCategory* PartCategory::clone(const QMap<PartPropertyMetaType*, PartPropertyMetaType*>& mtypeMap) const
{
    return new PartCategory(*this, mtypeMap);
}

void PartCategory::addProperty(PartProperty* prop)
{
    props.push_back(prop);
    prop->cat = this;

    notifyPropertyOrderChanged();

    descPropsDirty = true;
}

bool PartCategory::removeProperty(PartProperty* prop)
{
    bool removed = props.removeOne(prop);
    if (removed) {
        delete prop;
    }
    return removed;
}

PartProperty* PartCategory::getProperty(const QString& fieldName)
{
    for (PartProperty* prop : props) {
        if (prop->getFieldName() == fieldName) {
            return prop;
        }
    }
    return nullptr;
}

void PartCategory::rebuildFullTextIndex(bool fullRebuild, const PartList& argParts)
{
    System* sys = System::getInstance();

    if (!sys->hasValidDatabaseConnection()) {
        throw NoDatabaseConnectionException("Database connection needed to build full text index",
                __FILE__, __LINE__);
    }

    PartList parts = argParts;

    try {
        PropertyList ftProps;
        for (PartProperty* prop : props) {
            flags_t flags = prop->getFlags();
            if ((flags & PartProperty::FullTextIndex)  !=  0) {
                ftProps << prop;
            }
        }

        size_t numFetched;

        TermGenerator termGen;

        termGen.set_stemmer(Xapian::Stem("en"));
        termGen.set_stemming_strategy(TermGenerator::STEM_SOME);

        size_t i = 0;
        do {
            size_t offset = i*MAX_INDEX_REBUILD_FETCH_ROWS;
            size_t limit = MAX_INDEX_REBUILD_FETCH_ROWS;

            PartList::iterator partsBeg, partsEnd;

            if (fullRebuild) {
                QString limitClause = QString("LIMIT %1 OFFSET %2").arg(limit).arg(offset);
                parts = PartFactory::getInstance().loadItemsSingleCategory(
                        this, QString(), QString(), limitClause, 0, ftProps);
                partsBeg = parts.begin();
                partsEnd = parts.end();
            } else {
                partsBeg = parts.begin() + offset;
                partsEnd = partsBeg + limit;
                PartFactory::getInstance().loadItemsSingleCategory(partsBeg, partsEnd, 0, ftProps);
            }

            numFetched = 0;

            for (auto it = partsBeg ; it != partsEnd ; it++) {
                const Part& part = *it;

                Document doc;
                doc.set_data(QString::number(part.getID()).toStdString());

                termGen.set_document(doc);

                int j = 0;
                for (PartProperty* prop : ftProps) {
                    QVariant value = part.getData(prop);

                    if ((prop->getFlags() & PartProperty::MultiValue)  !=  0) {
                        QList<QVariant> multiValues = value.toList();

                        for (const QVariant& singleValue : multiValues) {
                            QString ftText = singleValue.toString();
                            QByteArray ftTextUtf8 = ftText.toUtf8();

                            termGen.index_text(ftTextUtf8.constData(), 1, QString("X%1").arg(j).toUtf8().constData());
                            termGen.increase_termpos();

                            termGen.index_text(ftTextUtf8.constData());
                            termGen.increase_termpos();
                        }
                    } else {
                        QString ftText = value.toString();
                        QByteArray ftTextUtf8 = ftText.toUtf8();

                        termGen.index_text(ftTextUtf8.constData(), 1, QString("X%1").arg(j).toUtf8().constData());
                        termGen.increase_termpos();

                        termGen.index_text(ftTextUtf8.constData());
                        termGen.increase_termpos();
                    }

                    j++;
                }

                xapianDb.replace_document(part.getID(), doc);

                numFetched++;
            }

            i++;
        } while (numFetched >= MAX_INDEX_REBUILD_FETCH_ROWS);
    } catch (SQLException& ex) {
        SQLException eex("Error caught while rebuilding full-text index", __FILE__, __LINE__, &ex);
        throw eex;
    }
}

void PartCategory::clearFullTextIndex()
{
    xapianDb.close();
    xapianDb = WritableDatabase("", Xapian::DB_BACKEND_INMEMORY);
}

PartList PartCategory::findInternal(Filter* filter, size_t offset, size_t numResults, size_t* countOnlyOut)
{
    if (numResults == 0  &&  !countOnlyOut) {
        return PartList();
    }

    bool usesSqlFiltering = !filter->getSQLWhereCode().isEmpty();
    bool usesFtFiltering = !filter->getFullTextQuery().isEmpty();

    System* sys = System::getInstance();

    if (!sys->hasValidDatabaseConnection()) {
        throw NoDatabaseConnectionException("Database connection needed to find parts", __FILE__, __LINE__);
    }

    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);

    try {
        PartList results;

        QString queryStr;
        QString joinCode("");

        bool first = true;
        for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
            PartProperty* prop = *it;

            if ((prop->getFlags() & PartProperty::MultiValue)  !=  0) {
                if (!first)
                    joinCode += " ";

                joinCode += QString("LEFT JOIN %1 ON %1.%2=%3.%4")
                        .arg(dbw->escapeIdentifier(prop->getMultiValueForeignTableName()),
                             dbw->escapeIdentifier(prop->getMultiValueIDFieldName()),
                             dbw->escapeIdentifier(getTableName()),
                             dbw->escapeIdentifier(idField));

                first = false;
            }
        }

        if (!usesFtFiltering) {
            // We can bypass the whole Xapian search

            if (countOnlyOut) {
                queryStr = QString("SELECT COUNT(DISTINCT %2.%1) FROM %2 %3 %4")
                        .arg(dbw->escapeIdentifier(getIDField()),
                             dbw->escapeIdentifier(getTableName()),
                             joinCode,
                             filter->getSQLWhereCode());
            } else {
                queryStr = QString("SELECT %2.%1 FROM %2 %3 %4 GROUP BY %2.%1 %5 LIMIT %6 OFFSET %7")
                        .arg(dbw->escapeIdentifier(getIDField()),
                             dbw->escapeIdentifier(getTableName()),
                             joinCode,
                             filter->getSQLWhereCode(),
                             filter->getSQLOrderCode())
                        .arg(numResults)
                        .arg(offset);
            }

            if (!query.prepare(queryStr)) {
                throw SQLException(query, "Error preparing filter results query", __FILE__, __LINE__);
            }
            filter->fillSQLWhereCodeBindParams(query);
            dbw->execAndCheckPreparedQuery(query, "Error selecting filter results");

            if (countOnlyOut) {
                if (!query.next()) {
                    throw SQLException("Filter counting query returned zero records!", __FILE__, __LINE__);
                }

                *countOnlyOut = static_cast<size_t>(query.value(0).toLongLong());
            } else {
                while (query.next()) {
                    results << PartFactory::getInstance().findPartByID(this, VariantToDBID(query.value(0)));
                }
            }
        } else {
            QueryParser qp;
            qp.set_stemmer(Xapian::Stem("en"));
            qp.set_stemming_strategy(QueryParser::STEM_SOME);
            qp.set_database(xapianDb);

            int i = 0;
            for (PartProperty* prop : props) {
                flags_t flags = prop->getFlags();

                if ((flags & PartProperty::FullTextIndex)  !=  0  &&  (flags & PartProperty::MultiValue)  ==  0) {
                    qp.add_prefix(prop->getFullTextSearchUserPrefix().toUtf8().constData(),
                            QString("X%1").arg(i).toUtf8().constData());
                    i++;
                }
            }

            for (PartProperty* prop : props) {
                flags_t flags = prop->getFlags();

                if ((flags & PartProperty::FullTextIndex)  !=  0  &&  (flags & PartProperty::MultiValue)  !=  0) {
                    qp.add_prefix(prop->getFullTextSearchUserPrefix().toUtf8().constData(),
                            QString("X%1").arg(i).toUtf8().constData());
                    i++;
                }
            }

            Query q = qp.parse_query (
                    filter->getFullTextQuery().toUtf8().constData(),
                    QueryParser::FLAG_BOOLEAN | QueryParser::FLAG_PHRASE | QueryParser::FLAG_LOVEHATE
                            | QueryParser::FLAG_WILDCARD
                    );

            Enquire enq(xapianDb);
            enq.set_query(q);


            if (filter->getSQLOrderCode().isEmpty()  &&  !countOnlyOut) {
                // No SQL ordering, so we order by Xapian rank.

                size_t lastFtNumResults = FILTER_SEARCH_BLOCK_SIZE;
                while (numResults != 0  &&  lastFtNumResults == FILTER_SEARCH_BLOCK_SIZE) {
                    MSet mset = enq.get_mset(static_cast<Xapian::doccount>(offset), FILTER_SEARCH_BLOCK_SIZE);

                    lastFtNumResults = mset.size();

                    if (lastFtNumResults == 0)
                        break;

                    if (mset.empty()) {
                        // No matches -> nothing to do.
                    } else {
                        QString idList = "";

                        for (MSetIterator it = mset.begin() ; it != mset.end() ; it++) {
                            if (it != mset.begin())
                                idList += ",";
                            dbid_t id = it.get_document().get_docid();
                            idList += QString::number(id);
                        }

                        QString whereCode = filter->getSQLWhereCode();

                        if (whereCode.isEmpty()) {
                            whereCode = "WHERE";
                        } else {
                            whereCode += " AND";
                        }

                        queryStr = QString("SELECT %2.%1 FROM %2 %3 %4 %2.%1 IN(%5) GROUP BY %2.%1 ORDER BY %2.%1 ASC")
                                .arg(dbw->escapeIdentifier(getIDField()),
                                     dbw->escapeIdentifier(getTableName()),
                                     joinCode,
                                     whereCode,
                                     idList);
                        if (!query.prepare(queryStr)) {
                            throw SQLException(query, "Error preparing filter results query", __FILE__, __LINE__);
                        }
                        filter->fillSQLWhereCodeBindParams(query);
                        dbw->execAndCheckPreparedQuery(query, "Error selecting filter results");

                        QSet<dbid_t> sqlMatchedIds;

                        while (query.next()) {
                            sqlMatchedIds.insert(VariantToDBID(query.value(0)));
                        }

                        for (MSetIterator it = mset.begin() ; it != mset.end()  &&  numResults != 0 ; it++) {
                            dbid_t id = it.get_document().get_docid();

                            if (sqlMatchedIds.contains(id)) {
                                results << PartFactory::getInstance().findPartByID(this, id);
                                numResults--;
                            }
                        }
                    }

                    offset += lastFtNumResults;
                }
            } else {
                // SQL ordering code given, so we order by that and ignore Xapian rank.

                QSet<dbid_t> ftMatchingIds;

                MSet mset = enq.get_mset(0, xapianDb.get_doccount());

                for (MSetIterator it = mset.begin() ; it != mset.end() ; it++) {
                    ftMatchingIds << it.get_document().get_docid();
                }

                if (!countOnlyOut) {
                    size_t i = 0;

                    while (numResults != 0) {
                        queryStr = QString("SELECT %2.%1 FROM %2 %3 %4 GROUP BY %2.%1 %5 LIMIT %6 OFFSET %7")
                                .arg(dbw->escapeIdentifier(getIDField()),
                                     dbw->escapeIdentifier(getTableName()),
                                     joinCode,
                                     filter->getSQLWhereCode(),
                                     filter->getSQLOrderCode())
                                     .arg(FILTER_SEARCH_SQL_BLOCK_SIZE)
                                     .arg(i*FILTER_SEARCH_SQL_BLOCK_SIZE);
                        if (!query.prepare(queryStr)) {
                            throw SQLException(query, "Error preparing filter results query", __FILE__, __LINE__);
                        }
                        filter->fillSQLWhereCodeBindParams(query);
                        dbw->execAndCheckPreparedQuery(query, "Error selecting filter results");

                        bool hadNextRow;
                        while ((hadNextRow = query.next())  &&  numResults != 0) {
                            dbid_t id = VariantToDBID(query.value(0));

                            if (ftMatchingIds.contains(id)) {
                                results << PartFactory::getInstance().findPartByID(this, id);
                                numResults--;
                            }
                        }

                        if (!hadNextRow)
                            break;

                        i++;
                    }
                } else {
                    if (ftMatchingIds.empty()) {
                        *countOnlyOut = 0;
                    } else {
                        QString whereCode(filter->getSQLWhereCode());

                        if (whereCode.isEmpty()) {
                            whereCode = QString("WHERE ");
                        } else {
                            int conditionStartIdx = whereCode.indexOf("WHERE ") + 6;

                            if (conditionStartIdx == -1) {
                                throw InvalidValueException("Invalid WHERE code in Filter object!", __FILE__, __LINE__);
                            }

                            whereCode.insert(conditionStartIdx, '(');
                            whereCode.append(") AND ");
                        }

                        whereCode += QString("%2.%1 IN(")
                                .arg(dbw->escapeIdentifier(getIDField()),
                                     dbw->escapeIdentifier(getTableName()));

                        for (QSet<dbid_t>::iterator it = ftMatchingIds.begin() ; it != ftMatchingIds.end() ; it++) {
                            if (it != ftMatchingIds.begin())
                                whereCode += ',';
                            whereCode += QString::number(*it);
                        }

                        whereCode += ")";

                        queryStr = QString("SELECT COUNT(DISTINCT %2.%1) FROM %2 %3 %4")
                                .arg(dbw->escapeIdentifier(getIDField()),
                                     dbw->escapeIdentifier(getTableName()),
                                     joinCode,
                                     whereCode);
                        if (!query.prepare(queryStr)) {
                            throw SQLException(query, "Error preparing filter results count query", __FILE__, __LINE__);
                        }
                        filter->fillSQLWhereCodeBindParams(query);
                        dbw->execAndCheckPreparedQuery(query, "Error counting filter results");

                        if (!query.next()) {
                            throw SQLException("Filter counting query returned zero records!", __FILE__, __LINE__);
                        }

                        *countOnlyOut = VariantToDBID(query.value(0));
                    }
                }
            }
        }

        return results;
    } catch (SQLException& ex) {
        SQLException eex("Error caught while finding parts", __FILE__, __LINE__, &ex);
        throw eex;
    }
}

PartList PartCategory::find(Filter* filter, size_t offset, size_t numResults)
{
    return findInternal(filter, offset, numResults);
}

size_t PartCategory::countMatches(Filter* filter)
{
    size_t count = 0;
    findInternal(filter, 0, 0, &count);
    return count;
}

PartCategory::PropertyList PartCategory::getDescriptionProperties()
{
    if (descPropsDirty) {
        descProps.clear();

        QString descPattern = getDescriptionPattern();

        QSet<QString> vars;
        ReplaceStringVariables(descPattern, {}, &vars);

        for (const QString& var : vars) {
            PartProperty* prop = getProperty(var);
            if (prop) {
                descProps << prop;
            }
        }

        descPropsDirty = false;
    }

    return descProps;
}

void PartCategory::setDescriptionPattern(const QString& descPat)
{
    descPattern = descPat;
    descPropsDirty = true;
}

QList<PartLinkType*> PartCategory::getPartLinkTypes() const
{
    QList<PartLinkType*> ltypes;
    PartCategory* ncThis = const_cast<PartCategory*>(this);
    for (PartLinkType* ltype : System::getInstance()->getPartLinkTypes()) {
        if (ltype->isPartCategoryAny(ncThis)) {
            ltypes << ltype;
        }
    }
    std::sort(ltypes.begin(), ltypes.end(), [&](const PartLinkType* a, const PartLinkType* b) {
        return a->getSortIndexInCategory(ncThis) < b->getSortIndexInCategory(ncThis);
    });
    return ltypes;
}

QList<AbstractPartProperty*> PartCategory::getAbstractProperties(const QList<PartLinkType*>& allLtypes) const
{
    PartCategory* ncThis = const_cast<PartCategory*>(this);
    QList<AbstractPartProperty*> allProps;
    for (PartProperty* prop : props) {
        allProps << prop;
    }
    for (PartLinkType* ltype : allLtypes) {
        if (ltype->isPartCategoryAny(ncThis)) {
            allProps << ltype;
        }
    }
    std::sort(allProps.begin(), allProps.end(), [&](const AbstractPartProperty* a, const AbstractPartProperty* b) {
        return a->getSortIndexInCategory(ncThis) < b->getSortIndexInCategory(ncThis);
    });
    return allProps;
}

void PartCategory::notifyPropertyOrderChanged()
{
    QList<PartProperty*> newProps(props);
    std::sort(newProps.begin(), newProps.end(), [&](const PartProperty* a, const PartProperty* b) {
        return a->getSortIndex() < b->getSortIndex();
    });
    props = newProps;
}


}


QDataStream& operator<<(QDataStream& s, const electronicsdb::PartCategory* pcat)
{
    return s << reinterpret_cast<quint64>(pcat);
}

QDataStream& operator>>(QDataStream& s, electronicsdb::PartCategory*& pcat)
{
    quint64 ptr;
    s >> ptr;
    pcat = reinterpret_cast<electronicsdb::PartCategory*>(ptr);
    return s;
}
