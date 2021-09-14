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

#include "StaticModelManager.h"

#include <cfloat>
#include <limits>
#include <memory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <nxcommon/exception/InvalidValueException.h>
#include <nxcommon/log.h>
#include "../db/SQLDatabaseWrapperFactory.h"
#include "../db/SQLTransaction.h"
#include "../exception/NoDatabaseConnectionException.h"
#include "../exception/SQLException.h"
#include "../System.h"

namespace electronicsdb
{


StaticModelManager* StaticModelManager::instance = nullptr;

StaticModelManager* StaticModelManager::getInstance()
{
    if (!instance) {
        instance = new StaticModelManager;
    }

    return instance;
}

void StaticModelManager::destroy()
{
    if (instance) {
        StaticModelManager* inst = instance;
        instance = nullptr;
        delete inst;
    }
}





// **********************************************************
// *                                                        *
// *                    SCHEMA LOADING                      *
// *                                                        *
// **********************************************************

void StaticModelManager::loadDefaultValues (
        const QList<PartCategory*>& pcats, const QList<QMap<QString, QMap<QString, QVariant>>>& columnData
) {
    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    // We will insert a dummy entry for each category, in order to fetch default values without having to parse them
    // manually. We do that in a transaction that will ultimately be rolled back, so that the changes never become
    // visible to the outside.
    SQLTransaction trans = dbw->beginTransaction();

    QStringList stmts;
    const dbid_t tmpPartID = 0; // All supported DB systems never automatically assign 0 for an INTEGER PRIMARY KEY.

    for (PartCategory* pcat : pcats) {
        stmts << QString("INSERT INTO %1 (%2) VALUES (%3);").arg(dbw->escapeIdentifier(pcat->getTableName()),
                                                                 dbw->escapeIdentifier(pcat->getIDField()))
                                                            .arg(tmpPartID);
    }

    QSqlQuery query(db);

    dbw->execAndCheckQueries(query, stmts, "Error inserting temporary default parts");

    for (int i = 0 ; i < pcats.size() ; i++) {
        PartCategory* pcat = pcats[i];
        const auto& colData = columnData[i];

        dbw->execAndCheckQuery(query, QString(
                "SELECT * FROM %1 WHERE %2=%3"
                ).arg(dbw->escapeIdentifier(pcat->getTableName()),
                      dbw->escapeIdentifier(pcat->getIDField()))
                 .arg(tmpPartID),
                "Error fetching temporary default part values");
        if (!query.next()) {
            throw SQLException("Temporary default part value fetch query returned empty results.", __FILE__, __LINE__);
        }

        for (PartProperty* prop : pcat->getProperties()) {
            if ((prop->getFlags() & PartProperty::MultiValue) != 0) {
                // TODO: Maybe allow other default values for multi-value properties somehow?
                prop->setDefaultValue(QList<QVariant>());
            } else {
                QString colName = prop->getFieldName();
                const auto& cdata = colData[colName];

                if (cdata.contains("default")) {
                    prop->setDefaultValueFunction(dbw->parsePartPropertyDefaultValueFunction(prop, cdata["default"]));
                }

                if (prop->getDefaultValueFunction() == PartProperty::DefaultValueConst) {
                    prop->setDefaultValue(query.value(colName));
                }
            }
        }

        assert(!query.next());
    }

    trans.rollback();
}

void StaticModelManager::loadMetaType(PartPropertyMetaType* mtype, const QSqlQuery& query, QString& parentMetaTypeID)
{
    parentMetaTypeID = QString();

    QString typeStr;

    if (!query.isNull("type")) {
        typeStr = query.value("type").toString().toLower();
    } else {
        throw InvalidValueException(QString("Missing type for meta type"));
    }

    bool isCoreType = true;

    if (typeStr == "string") {
        mtype->coreType = PartProperty::String;
    } else if (typeStr == "int"  ||  typeStr == "integer") {
        mtype->coreType = PartProperty::Integer;
    } else if (typeStr == "decimal") {
        mtype->coreType = PartProperty::Decimal;
    } else if (typeStr == "float"  ||  typeStr == "double") {
        mtype->coreType = PartProperty::Float;
    } else if (typeStr == "boolean"  ||  typeStr == "bool") {
        mtype->coreType = PartProperty::Boolean;
    } else if (typeStr == "file") {
        mtype->coreType = PartProperty::File;
    } else if (typeStr == "date") {
        mtype->coreType = PartProperty::Date;
    } else if (typeStr == "time") {
        mtype->coreType = PartProperty::Time;
    } else if (typeStr == "datetime") {
        mtype->coreType = PartProperty::DateTime;
    } else {
        isCoreType = false;
        parentMetaTypeID = typeStr;
    }

    if (!query.isNull("name")) {
        mtype->userReadableName = query.value("name").toString();
    }

    flags_t flags = 0;
    flags_t flagsWithNonNullValue = 0;

    struct FlagEntry
    {
        CString fieldName;
        flags_t flagCode;
    };

    const FlagEntry flagEntries[] = {
            {"flag_full_text_indexed", PartProperty::FullTextIndex},
            {"flag_multi_value", PartProperty::MultiValue},
            {"flag_si_prefixes_default_to_base2", PartProperty::SIPrefixesDefaultToBase2},
            {"flag_display_with_units", PartProperty::DisplayWithUnits},
            {"flag_display_selection_list", PartProperty::DisplaySelectionList},
            {"flag_display_hide_from_listing_table", PartProperty::DisplayHideFromListingTable},
            {"flag_display_text_area", PartProperty::DisplayTextArea},
            {"flag_display_multi_in_single_field", PartProperty::DisplayMultiInSingleField},
            {"flag_display_dynamic_enum", PartProperty::DisplayDynamicEnum}
    };

    for (size_t i = 0 ; i < sizeof(flagEntries) / sizeof(FlagEntry) ; i++) {
        const FlagEntry& entry = flagEntries[i];

        if (!query.isNull(entry.fieldName)) {
            flagsWithNonNullValue |= entry.flagCode;

            if (query.value(entry.fieldName).toBool()) {
                flags |= entry.flagCode;
            }
        }
    }

    if (!query.isNull("display_unit_affix")) {
        CString affix = query.value("display_unit_affix").toString().toLower();

        if (affix == "sibase10") {
            // Absence of a DisplayUnitPrefix* flag signals SI base 10
            mtype->unitPrefixType = PartProperty::UnitPrefixSIBase10;
        } else if (affix == "sibase2") {
            mtype->unitPrefixType = PartProperty::UnitPrefixSIBase2;
        } else if (affix == "percent") {
            mtype->unitPrefixType = PartProperty::UnitPrefixPercent;
        } else if (affix == "ppm") {
            mtype->unitPrefixType = PartProperty::UnitPrefixPPM;
        } else if (affix == "none") {
            mtype->unitPrefixType = PartProperty::UnitPrefixNone;
        } else {
            mtype->unitPrefixType = PartProperty::UnitPrefixNone;
        }
    }

    mtype->flags = flags;
    mtype->flagsWithNonNullValue = flagsWithNonNullValue;

    if (!query.isNull("string_max_length")) {
        mtype->strMaxLen = query.value("string_max_length").toInt();
    }

    if (!query.isNull("ft_user_prefix")) {
        mtype->ftUserPrefix = query.value("ft_user_prefix").toString();
    }

    if (!query.isNull("unit_suffix")) {
        mtype->unitSuffix = query.value("unit_suffix").toString();
    }

    if (!query.isNull("sql_natural_order_code")) {
        mtype->sqlNaturalOrderCode = query.value("sql_natural_order_code").toString();
    }
    if (!query.isNull("sql_ascending_order_code")) {
        mtype->sqlAscendingOrderCode = query.value("sql_ascending_order_code").toString();
    }
    if (!query.isNull("sql_descending_order_code")) {
        mtype->sqlDescendingOrderCode = query.value("sql_descending_order_code").toString();
    }

    if (!query.isNull("bool_true_text")) {
        mtype->boolTrueText = query.value("bool_true_text").toString();
    }
    if (!query.isNull("bool_false_text")) {
        mtype->boolFalseText = query.value("bool_false_text").toString();
    }

    if (!query.isNull("int_range_min")) {
        mtype->intRangeMin = query.value("int_range_min").toLongLong();
    }
    if (!query.isNull("int_range_max")) {
        mtype->intRangeMax = query.value("int_range_max").toLongLong();
    }

    if (!query.isNull("decimal_range_min")) {
        mtype->floatRangeMin = query.value("decimal_range_min").toDouble();
    }
    if (!query.isNull("decimal_range_max")) {
        mtype->floatRangeMax = query.value("decimal_range_max").toDouble();
    }

    if (!query.isNull("textarea_min_height")) {
        mtype->textAreaMinHeight = query.value("textarea_min_height").toInt();
    }

    if (query.record().indexOf("sortidx") >= 0  &&  !query.isNull("sortidx")) {
        mtype->sortIndex = query.value("sortidx").toInt();
    }
}

QList<PartPropertyMetaType*> StaticModelManager::buildPartPropertyMetaTypes()
{
    System* sys = System::getInstance();

    DatabaseConnection* conn = sys->getCurrentDatabaseConnection();
    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, "SELECT * FROM meta_type", "Error selecting meta_type entries");

    QMap<PartPropertyMetaType*, QString> metaTypeParentIDs;
    QMap<QString, PartPropertyMetaType*> metaTypesByID;
    QList<PartPropertyMetaType*> metaTypes;

    try {
        // Load meta types, but don't associate parents yet
        while (query.next()) {
            PartPropertyMetaType* mtype = new PartPropertyMetaType(query.value("id").toString());

            QString parentMetaTypeID;
            loadMetaType(mtype, query, parentMetaTypeID);

            metaTypeParentIDs[mtype] = parentMetaTypeID;
            metaTypesByID[mtype->metaTypeID] = mtype;
            metaTypes << mtype;
        }

        // Associate parents
        for (auto it = metaTypeParentIDs.begin() ; it != metaTypeParentIDs.end() ; ++it) {
            PartPropertyMetaType* mtype = it.key();
            QString parentTypeID = it.value();

            if (!parentTypeID.isNull()) {
                PartPropertyMetaType* pmtype = metaTypesByID[parentTypeID];
                if (!pmtype) {
                    throw InvalidValueException(QString("Invalid type '%1' for meta_type '%2'")
                            .arg(parentTypeID, mtype->metaTypeID).toUtf8().constData(), __FILE__, __LINE__);
                }
                mtype->parent = pmtype;
            } else {
                if (mtype->coreType == PartProperty::InvalidType) {
                    throw InvalidValueException(QString("Invalid type for meta_type '%1'").arg(mtype->metaTypeID)
                            .toUtf8().constData(), __FILE__, __LINE__);
                }
            }
        }

        // Check for loops
        for (PartPropertyMetaType* mtype : metaTypes) {
            QList<PartPropertyMetaType*> path;

            while (mtype) {
                int loopStartIdx = path.indexOf(mtype);
                path << mtype;

                if (loopStartIdx >= 0) {
                    QString loopStr;
                    for (int i = loopStartIdx ; i < path.size() ; i++) {
                        if (!loopStr.isEmpty()) {
                            loopStr += " -> ";
                        }
                        loopStr += path[i]->metaTypeID;
                    }
                    throw InvalidValueException(QString("Loop found in meta_type table: %1").arg(loopStr)
                            .toUtf8().constData(), __FILE__, __LINE__);
                }

                mtype = mtype->parent;
            }
        }
    } catch (std::exception&) {
        for (PartPropertyMetaType* mtype : metaTypes) {
            delete mtype;
        }
        throw;
    }

    return metaTypes;
}

QList<PartCategory*> StaticModelManager::buildCategories()
{
    System* sys = System::getInstance();

    cats.clear();

    if (!sys->hasValidDatabaseConnection()) {
        return cats;
    }

    LogDebug("Building part categories...");

    DatabaseConnection* conn = sys->getCurrentDatabaseConnection();
    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, QString(
            "SELECT id, name, desc_pattern, sortidx FROM part_category ORDER BY sortidx, name ASC"
            ),
            "Error fetching part categories");

    while (query.next()) {
        QString catName = query.value("id").toString();

        PartCategory* cat = new PartCategory(catName, query.value("name").toString());
        cat->setDescriptionPattern(query.value("desc_pattern").toString());
        cat->setSortIndex(query.value("sortidx").toInt());

        cats << cat;
    }

    QList<QMap<QString, QMap<QString, QVariant>>> colDataByCat;

    for (PartCategory* cat : cats) {
        QString catID = cat->getID();

        auto colData = dbw->listColumns(cat->getTableName(), true);
        QMap<QString, QMap<QString, QVariant>> colDataByName;
        for (const auto& cdata : colData) {
            colDataByName[cdata["name"].toString()] = cdata;
        }
        colDataByCat << colDataByName;

        dbw->execAndCheckQuery(query, QString("SELECT * FROM pcatmeta_%1 ORDER BY sortidx, id ASC").arg(catID),
                               "Error fetching category metadata");

        while (query.next()) {
            QString propID = query.value("id").toString();

            PartPropertyMetaType* mtype = new PartPropertyMetaType(QString());

            QString parentMetaTypeID;
            loadMetaType(mtype, query, parentMetaTypeID);

            if (!parentMetaTypeID.isNull()) {
                mtype->parent = sys->getPartPropertyMetaType(parentMetaTypeID);
                if (!mtype->parent) {
                    throw InvalidValueException(QString("Invalid type for part property '%1.%2': %3")
                            .arg(cat->getID(), propID, parentMetaTypeID).toUtf8().constData(), __FILE__, __LINE__);
                }
            }

            assert(mtype->parent  ||  mtype->coreType != PartProperty::InvalidType);

            const auto& cdata = colDataByName[propID];

            PartProperty* prop = new PartProperty(propID, mtype, cat, PartProperty::CustomMetaType{});

            if (cdata.contains("intMin")) {
                prop->setIntegerStorageRangeMinimum(cdata["intMin"].toLongLong());
            }
            if (cdata.contains("intMax")) {
                prop->setIntegerStorageRangeMaximum(cdata["intMax"].toLongLong());
            }

            if (cdata.contains("stringLenMax")) {
                prop->setStringStorageMaximumLength(cdata["stringLenMax"].toInt());
            }
        }
    }

    loadDefaultValues(cats, colDataByCat);

    for (PartCategory* cat : cats) {
        cat->rebuildFullTextIndex();
    }

    return cats;
}

QList<PartLinkType*> StaticModelManager::buildPartLinkTypes()
{
    System* sys = System::getInstance();

    partLinkTypes.clear();

    if (!sys->hasValidDatabaseConnection()) {
        return partLinkTypes;
    }

    LogDebug("Building partlink types...");

    DatabaseConnection* conn = sys->getCurrentDatabaseConnection();
    QSqlDatabase db = QSqlDatabase::database();
    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QSqlQuery query(db);
    dbw->execAndCheckQuery(query, QString(
            "SELECT  t.*,\n"
            "        (SELECT %1 FROM partlink_type_patternpcat WHERE lid=t.id AND side='a') AS pcats_a,\n"
            "        (SELECT %1 FROM partlink_type_patternpcat WHERE lid=t.id AND side='b') AS pcats_b,\n"
            "        (SELECT %2 FROM partlink_type_pcat WHERE lid=t.id) AS pcatdata\n"
            "FROM partlink_type t"
            ).arg(dbw->groupConcatCode("pcat", ","),
                  dbw->groupJsonArrayCode(dbw->jsonArrayCode({"side", "pcat", "sortidx"}))),
            "Error fetching partlink types");

    struct FlagEntry
    {
        QString fieldName;
        int flagCode;
    };

    const FlagEntry flagEntries[] = {
            {"flag_show_in_a", PartLinkType::ShowInA},
            {"flag_show_in_b", PartLinkType::ShowInB},
            {"flag_edit_in_a", PartLinkType::EditInA},
            {"flag_edit_in_b", PartLinkType::EditInB},
            {"flag_display_selection_list_a", PartLinkType::DisplaySelectionListA},
            {"flag_display_selection_list_b", PartLinkType::DisplaySelectionListB}
    };

    while (query.next()) {
        QString id = query.value("id").toString();
        QString pcatPatternA = query.value("pcats_a").toString();
        QString pcatPatternB = query.value("pcats_b").toString();
        QJsonDocument jPcatData = QJsonDocument::fromJson(query.value("pcatdata").toString().toUtf8());

        QList<PartCategory*> pcatsA;
        QList<PartCategory*> pcatsB;

        for (QString pcatID : pcatPatternA.split(',', Qt::SkipEmptyParts)) {
            PartCategory* pcat = sys->getPartCategory(pcatID);
            if (pcat) {
                pcatsA << pcat;
            }
        }
        for (QString pcatID : pcatPatternB.split(',', Qt::SkipEmptyParts)) {
            PartCategory* pcat = sys->getPartCategory(pcatID);
            if (pcat) {
                pcatsB << pcat;
            }
        }

        QString patternTypeStr = query.value("pcat_type").toString();
        PartLinkType::PatternType patternType;

        if (patternTypeStr == "a") {
            patternType = PartLinkType::PatternTypeANegative;
        } else if (patternTypeStr == "b") {
            patternType = PartLinkType::PatternTypeBNegative;
        } else {
            patternType = PartLinkType::PatternTypeBothPositive;
        }

        PartLinkType* type = new PartLinkType(id, pcatsA, pcatsB, patternType);

        type->setNameA(query.value("name_a").toString());
        type->setNameB(query.value("name_b").toString());

        int flags = PartLinkType::Default;

        for (size_t i = 0 ; i < sizeof(flagEntries) / sizeof(FlagEntry) ; i++) {
            const FlagEntry& entry = flagEntries[i];

            if (!query.isNull(entry.fieldName)) {
                if (query.value(entry.fieldName).toBool()) {
                    flags |= entry.flagCode;
                } else {
                    flags &= ~entry.flagCode;
                }
            }
        }

        type->setFlags(flags);

        for (const QJsonValue& jval : jPcatData.array()) {
            QJsonArray jentry = jval.toArray();
            //QString side = jentry[0].toString();
            PartCategory* pcat = sys->getPartCategory(jentry[1].toString());
            int sortIdx = jentry[2].toInt();
            if (pcat) {
                type->setSortIndexInCategory(pcat, sortIdx);
            }
        }

        partLinkTypes << type;
    }

    return partLinkTypes;
}






// **********************************************************
// *                                                        *
// *            PART LINK SCHEMA GENERATION                 *
// *                                                        *
// **********************************************************

QStringList StaticModelManager::generatePartLinkTypeDeltaCode (
        const QList<PartLinkType*>& added,
        const QList<PartLinkType*>& removed,
        const QMap<PartLinkType*, PartLinkType*>& edited,
        const QSqlDatabase& targetDb
) {
    System* sys = System::getInstance();
    if (!sys->hasValidDatabaseConnection()) {
        throw NoDatabaseConnectionException("Need a database connection to generate part link type schema code",
                                            __FILE__, __LINE__);
    }


    QStringList schemaStmts;

    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(targetDb));

    // Process added types
    for (PartLinkType* ltype : added) {
        schemaStmts << generatePartLinkTypeAddCode(dbw.get(), ltype);
    }

    // Process edited types
    for (auto it = edited.begin() ; it != edited.end() ; ++it) {
        PartLinkType* oldLtype = it.key();
        PartLinkType* ltype = it.value();

        schemaStmts << generatePartLinkTypeEditCode(dbw.get(), oldLtype, ltype);
    }

    // Process removed types
    for (PartLinkType* ltype : removed) {
        schemaStmts << generatePartLinkTypeRemoveCode(dbw.get(), ltype);
    }

    return schemaStmts;
}

QStringList StaticModelManager::generatePartLinkTypeAddCode (
        SQLDatabaseWrapper* dbw, PartLinkType* ltype, const QString& overrideID
) {
    QStringList schemaStmts;

    QString id = overrideID.isNull() ? ltype->getID() : overrideID;

    auto flags = ltype->getFlags();
    QString pcatTypeStr;

    switch (ltype->getPatternType()) {
    case PartLinkType::PatternTypeANegative:
        pcatTypeStr = "a";
        break;
    case PartLinkType::PatternTypeBNegative:
        pcatTypeStr = "b";
        break;
    case PartLinkType::PatternTypeBothPositive:
        pcatTypeStr = "+";
        break;
    default:
        assert(false);
    }

    schemaStmts << QString(
            "INSERT INTO partlink_type (\n"
            "    id,\n"
            "    pcat_type,\n"
            "    name_a, name_b,\n"
            "    flag_show_in_a, flag_show_in_b,\n"
            "    flag_edit_in_a, flag_edit_in_b,\n"
            "    flag_display_selection_list_a, flag_display_selection_list_b\n"
            ") VALUES (\n"
            "    %1,   -- id\n"
            "    %2,   -- pcat_type\n"
            "    %3, %4,   -- name_a, name_b\n"
            "    %5, %6,   -- flag_show_in_a, flag_show_in_b\n"
            "    %7, %8,   -- flag_edit_in_a, flag_edit_in_b\n"
            "    %9, %10   -- flag_display_selection_list_a, flag_display_selection_list_b\n"
            ");"
            ).arg(dbw->escapeString(id),

                  dbw->escapeString(pcatTypeStr),

                  dbw->escapeString(ltype->getNameA()),
                  dbw->escapeString(ltype->getNameB()),

                  (flags & PartLinkType::ShowInA) != 0 ? "TRUE" : "FALSE",
                  (flags & PartLinkType::ShowInB) != 0 ? "TRUE" : "FALSE",
                  (flags & PartLinkType::EditInA) != 0 ? "TRUE" : "FALSE",
                  (flags & PartLinkType::EditInB) != 0 ? "TRUE" : "FALSE",
                  (flags & PartLinkType::DisplaySelectionListA) != 0 ? "TRUE" : "FALSE",
                  (flags & PartLinkType::DisplaySelectionListB) != 0 ? "TRUE" : "FALSE");

    QStringList patternPcatValsList;
    for (PartCategory* pcat : ltype->getPatternPartCategoriesA()) {
        patternPcatValsList << QString("(%1, 'a', %2)").arg(dbw->escapeString(id), dbw->escapeString(pcat->getID()));
    }
    for (PartCategory* pcat : ltype->getPatternPartCategoriesB()) {
        patternPcatValsList << QString("(%1, 'b', %2)").arg(dbw->escapeString(id), dbw->escapeString(pcat->getID()));
    }
    if (!patternPcatValsList.empty()) {
        schemaStmts << QString("INSERT INTO partlink_type_patternpcat (lid, side, pcat) VALUES %1;")
                .arg(patternPcatValsList.join(", "));
    }

    QStringList pcatValsList;
    auto sortIndices = ltype->getSortIndicesForCategories();
    for (auto it = sortIndices.begin() ; it != sortIndices.end() ; ++it) {
        PartCategory* pcat = it.key();
        int sortIdx = it.value();
        pcatValsList << QString("(%1, '%2', %3, %4)").arg(dbw->escapeString(id),
                                                        ltype->isPartCategoryA(pcat) ? "a" : "b",
                                                        dbw->escapeString(pcat->getID()))
                                                     .arg(ltype->getSortIndexInCategory(pcat));
    }
    if (!pcatValsList.empty()) {
        schemaStmts << QString("INSERT INTO partlink_type_pcat (lid, side, pcat, sortidx) VALUES %1;")
                .arg(pcatValsList.join(", "));
    }

    return schemaStmts;
}

QStringList StaticModelManager::generatePartLinkTypeRemoveCode (
        SQLDatabaseWrapper* dbw, PartLinkType* ltype, const QString& overrideID
) {
    QStringList schemaStmts;

    QString id = overrideID.isNull() ? ltype->getID() : overrideID;

    schemaStmts << QString("DELETE FROM partlink WHERE type=%1;").arg(dbw->escapeString(id));
    schemaStmts << QString("DELETE FROM partlink_type_patternpcat WHERE lid=%1;").arg(dbw->escapeString(id));
    schemaStmts << QString("DELETE FROM partlink_type_pcat WHERE lid=%1;").arg(dbw->escapeString(id));
    schemaStmts << QString("DELETE FROM partlink_type WHERE id=%1;").arg(dbw->escapeString(id));

    return schemaStmts;
}

QStringList StaticModelManager::generatePartLinkTypeEditCode (
        SQLDatabaseWrapper* dbw, PartLinkType* oldLtype, PartLinkType* ltype
) {
    QStringList schemaStmts;

    auto editDifferentID = [&](PartLinkType* from, const QString& fromID, PartLinkType* to, const QString& toID) {
        // Add new type
        schemaStmts << generatePartLinkTypeAddCode(dbw, to, toID);

        // Change references from old type to new type
        schemaStmts << QString(
                "UPDATE partlink SET type=%1 WHERE type=%2;"
                ).arg(dbw->escapeString(toID), dbw->escapeString(fromID));

        // Delete old type
        schemaStmts << generatePartLinkTypeRemoveCode(dbw, from, fromID);
    };

    if (oldLtype->getID() == ltype->getID()) {
        editDifferentID(oldLtype, oldLtype->getID(), ltype, "__temp_id");
        editDifferentID(ltype, "__temp_id", ltype, ltype->getID());
    } else {
        editDifferentID(oldLtype, oldLtype->getID(), ltype, ltype->getID());
    }

    return schemaStmts;
}






// **********************************************************
// *                                                        *
// *                CATEGORY SCHEMA GENERATION              *
// *                                                        *
// **********************************************************

QStringList StaticModelManager::generateCategorySchemaDeltaCode (
        const QList<PartCategory*>& added,
        const QList<PartCategory*>& removed,
        const QMap<PartCategory*, PartCategory*>& edited,
        const QMap<PartProperty*, PartProperty*>& editedProps,
        const QSqlDatabase& targetDb
) {
    System* sys = System::getInstance();
    if (!sys->hasValidDatabaseConnection()) {
        throw NoDatabaseConnectionException("Need a database connection to generate category schema code",
                                            __FILE__, __LINE__);
    }

    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(targetDb));

    QStringList schemaStmts;

    // They will otherwise cause foreign key constraint failures that are very hard to eliminate. Dropping them is fine,
    // because they are recreated at connect anyway.
    if (!added.empty()  ||  !edited.empty()  ||  !removed.empty()) {
        for (auto trigger : dbw->listTriggers()) {
            if (trigger["name"].toString().startsWith("trigger_clog_")) {
                schemaStmts << dbw->generateDropTriggerIfExistsCode(trigger["name"].toString(),
                                                                    trigger["table"].toString());
            }
        }
        schemaStmts << "DROP TABLE IF EXISTS clog_pcat;";
        schemaStmts << "DROP TABLE IF EXISTS clog_container;";
    }

    for (PartCategory* pcat : added) {
        schemaStmts << generateCategoryAddCode(dbw.get(), pcat);
    }
    for (auto it = edited.begin() ; it != edited.end() ; ++it) {
        schemaStmts << generateCategoryEditCode(dbw.get(), it.key(), it.value(), editedProps);
    }
    for (PartCategory* pcat : removed) {
        schemaStmts << generateCategoryRemoveCode(dbw.get(), pcat);
    }

    return schemaStmts;
}

QStringList StaticModelManager::generateCategoryAddCode (
        SQLDatabaseWrapper* dbw, PartCategory* pcat, const QString& overrideID
) {
    QStringList schemaStmts;

    QString pcatID = overrideID.isNull() ? pcat->getID() : overrideID;

    // Main table
    {
        // NOTE: Property columns will be inserted by generateCategorySchemaPropertiesDeltaCode() later.
        schemaStmts << QString(
                "CREATE TABLE %1 (\n"
                "    %2\n"
                ");"
                ).arg(dbw->escapeIdentifier(QString("pcat_%1").arg(pcatID)),
                      dbw->generateIntPrimaryKeyCode("id"));
    }

    // Meta table
    {
        schemaStmts << QString(
                "CREATE TABLE %1 (\n"
                "    id VARCHAR(64) PRIMARY KEY NOT NULL,\n"
                "    \n"
                "%2,\n"
                "    \n"
                "    sortidx INTEGER DEFAULT 2000000 NOT NULL\n"
                ");\n"
                ).arg(dbw->escapeIdentifier(QString("pcatmeta_%1").arg(pcatID)),
                      dbw->generateMetaTypeFieldDefCode());
    }

    // Properties
    schemaStmts << generateCategorySchemaPropertiesDeltaCode(dbw, pcat, pcat->getProperties(), {}, {}, overrideID);

    // Registry entry
    schemaStmts << generateCategoryRegEntryAddCode(dbw, pcat, overrideID);

    return schemaStmts;
}

QStringList StaticModelManager::generateCategoryRemoveCode (
        SQLDatabaseWrapper* dbw, PartCategory* pcat, const QString& overrideID
) {
    QStringList schemaStmts;

    QString pcatID = overrideID.isNull() ? pcat->getID() : overrideID;

    // Container assocs
    {
        schemaStmts << QString(
                "DELETE FROM container_part WHERE ptype=%1;"
                ).arg(dbw->escapeString(pcatID));
    }

    // Part links
    {
        schemaStmts << QString(
                "DELETE FROM partlink WHERE ptype_a=%1 OR ptype_b=%1;"
                ).arg(dbw->escapeString(pcatID));
    }

    // Part link types
    schemaStmts << PartLinkType::generatePatternSQLUpdateCode(dbw, {pcatID}, {});

    // Registry entry
    schemaStmts << generateCategoryRegEntryRemoveCode(dbw, pcat, overrideID);

    // Property entries + meta table
    {
        schemaStmts << QString (
                "DROP TABLE %1;"
                ).arg(dbw->escapeIdentifier(QString("pcatmeta_%1").arg(pcatID)));
    }

    // Multi-value tables
    {
        for (PartProperty* prop : pcat->getProperties()) {
            if ((prop->getFlags() & PartProperty::MultiValue) != 0) {
                schemaStmts << QString(
                        "DROP TABLE %1;"
                        ).arg(dbw->escapeIdentifier(QString("pcatmv_%1_%2").arg(pcatID, prop->getFieldName())));
            }
        }
    }

    // Main table
    {
        schemaStmts << QString(
                "DROP TABLE %1;"
                ).arg(dbw->escapeIdentifier(QString("pcat_%1").arg(pcatID)));
    }

    return schemaStmts;
}

QStringList StaticModelManager::generateCategoryEditCode (
        SQLDatabaseWrapper* dbw, PartCategory* oldPcat, PartCategory* pcat, const QMap<PartProperty*,
        PartProperty*>& editedProps
) {
    QStringList schemaStmts;

    QMap<PartProperty*, PartProperty*> fullEditedProps = editedProps;
    for (PartProperty* oldProp : oldPcat->getProperties()) {
        if (editedProps.contains(oldProp)) {
            continue;
        }
        for (PartProperty* prop : pcat->getProperties()) {
            if (oldProp->getFieldName() == prop->getFieldName()) {
                fullEditedProps[oldProp] = prop;
                break;
            }
        }
    }


    // ********** UPDATE EVERYTHING EXCEPT PROPERTIES **********

    auto editDifferentID = [&](PartCategory* from, const QString& fromID, PartCategory* to, const QString& toID) {
        // Insert new entry
        schemaStmts << generateCategoryRegEntryAddCode(dbw, to, toID);

        // Update foreign key references
        schemaStmts << QString(
                "UPDATE container_part SET ptype=%1 WHERE ptype=%2;"
                ).arg(dbw->escapeString(toID), dbw->escapeString(fromID));
        schemaStmts << QString(
                "UPDATE partlink SET\n"
                "    ptype_a=(CASE WHEN ptype_a=%1 THEN %2 ELSE ptype_a END),\n"
                "    ptype_b=(CASE WHEN ptype_b=%1 THEN %2 ELSE ptype_b END);"
                ).arg(dbw->escapeString(fromID), dbw->escapeString(toID));

        // Rename tables
        schemaStmts << QString(
                "ALTER TABLE %1 RENAME TO %2;"
                ).arg(dbw->escapeIdentifier(QString("pcat_%1").arg(fromID)),
                      dbw->escapeIdentifier(QString("pcat_%1").arg(toID)));
        schemaStmts << QString(
                "ALTER TABLE %1 RENAME TO %2;"
                ).arg(dbw->escapeIdentifier(QString("pcatmeta_%1").arg(fromID)),
                      dbw->escapeIdentifier(QString("pcatmeta_%1").arg(toID)));
        for (PartProperty* prop : from->getProperties()) {
            if ((prop->getFlags() & PartProperty::MultiValue) == 0) {
                continue;
            }
            schemaStmts << QString(
                    "ALTER TABLE %1 RENAME TO %2;"
                    ).arg(dbw->escapeIdentifier(QString("pcatmv_%1_%2").arg(fromID, prop->getFieldName())),
                          dbw->escapeIdentifier(QString("pcatmv_%1_%2").arg(toID, prop->getFieldName())));
        }

        // Part link types
        schemaStmts << PartLinkType::generatePatternSQLUpdateCode(dbw, {},
                                                                  {std::pair<QString, QString>(fromID, toID)});

        // Remove old entry
        schemaStmts << generateCategoryRegEntryRemoveCode(dbw, from, fromID);
    };

    if (oldPcat->getID() == pcat->getID()) {
        editDifferentID(oldPcat, oldPcat->getID(), oldPcat, "__temp_id");
        editDifferentID(oldPcat, "__temp_id", pcat, pcat->getID());
    } else {
        editDifferentID(oldPcat, oldPcat->getID(), pcat, pcat->getID());
    }


    // ********** UPDATE PROPERTIES **********

    QList<PartProperty*> propsAdded;
    QList<PartProperty*> propsRemoved;
    QMap<PartProperty*, PartProperty*> propsEdited;

    auto propEqFunc = [&](PartProperty* oldProp, PartProperty* prop) {
        auto it = fullEditedProps.find(oldProp);
        if (it != fullEditedProps.end()  &&  it.value() == prop) {
            return true;
        }
        return false;
    };

    calculateDelta(propsAdded, propsRemoved, propsEdited, oldPcat->getProperties(), pcat->getProperties(), propEqFunc);

    for (PartProperty* propAdded : propsAdded) {
        for (PartProperty* propRemoved : propsRemoved) {
            if (propAdded->getFieldName() == propRemoved->getFieldName()) {
                throw InvalidStateException("A property present in both old and new category was not marked as edited!"
                                            __FILE__, __LINE__);
            }
        }
    }

    schemaStmts << generateCategorySchemaPropertiesDeltaCode(dbw, pcat, propsAdded, propsRemoved, propsEdited);

    return schemaStmts;
}

QStringList StaticModelManager::generateCategoryRegEntryAddCode (
        SQLDatabaseWrapper* dbw, PartCategory* pcat, const QString& overrideID
) {
    QStringList schemaStmts;

    QString pcatID = overrideID.isNull() ? pcat->getID() : overrideID;

    schemaStmts << QString(
            "INSERT INTO part_category (id, name, desc_pattern, sortidx) VALUES (%1, %2, %3, %4);"
            ).arg(dbw->escapeString(pcatID),
                  dbw->escapeString(pcat->getUserReadableName()),
                  dbw->escapeString(pcat->getDescriptionPattern()))
             .arg(pcat->getSortIndex());

    return schemaStmts;
}

QStringList StaticModelManager::generateCategoryRegEntryRemoveCode (
        SQLDatabaseWrapper* dbw, PartCategory* pcat, const QString& overrideID
) {
    QStringList schemaStmts;

    QString pcatID = overrideID.isNull() ? pcat->getID() : overrideID;

    schemaStmts << QString(
            "DELETE FROM part_category WHERE id=%1;"
            ).arg(dbw->escapeString(pcatID));

    return schemaStmts;
}








// **********************************************************
// *                                                        *
// *            PROPERTIES SCHEMA GENERATION                *
// *                                                        *
// **********************************************************

QStringList StaticModelManager::generateCategorySchemaPropertiesDeltaCode (
        SQLDatabaseWrapper* dbw,
        PartCategory* pcat,
        const QList<PartProperty*>& added,
        const QList<PartProperty*>& removed,
        const QMap<PartProperty*, PartProperty*>& edited,
        const QString& pcatOverrideID
) {
    QStringList schemaStmts;

    QString pcatID = pcatOverrideID.isNull() ? pcat->getID() : pcatOverrideID;

    for (PartProperty* prop : added) {
        schemaStmts << generatePartPropertyAddCode(dbw, prop, QString(), pcatID);
    }
    for (auto it = edited.begin() ; it != edited.end() ; ++it) {
        schemaStmts << generatePartPropertyEditCode(dbw, it.key(), it.value(), pcatID);
    }
    for (PartProperty* prop : removed) {
        schemaStmts << generatePartPropertyRemoveCode(dbw, prop, QString(), pcatID);
    }

    return schemaStmts;
}

QStringList StaticModelManager::generatePartPropertyAddCode (
        SQLDatabaseWrapper* dbw, PartProperty* prop, const QString& overrideID, const QString& pcatOverrideID
) {
    QStringList schemaStmts;

    PartCategory* pcat = prop->getPartCategory();

    QString pcatID = pcatOverrideID.isNull() ? pcat->getID() : pcatOverrideID;
    QString propFieldName = overrideID.isNull() ? prop->getFieldName() : overrideID;

    if ((prop->getFlags() & PartProperty::MultiValue) != 0) {
        // Multi-value table
        {
            schemaStmts << QString(
                    "CREATE TABLE IF NOT EXISTS %1 (\n"
                    "    %2,\n"
                    "    pid INTEGER NOT NULL REFERENCES %3(%4),\n"
                    "    %5 %6 NOT NULL,\n"
                    "    idx SMALLINT NOT NULL DEFAULT 0\n"
                    ");"
                    ).arg(dbw->escapeIdentifier(QString("pcatmv_%1_%2").arg(pcatID, propFieldName)),
                          dbw->generateIntPrimaryKeyCode("id"),
                          dbw->escapeIdentifier(QString("pcat_%1").arg(pcatID)),
                          dbw->escapeIdentifier(pcat->getIDField()),
                          dbw->escapeIdentifier(propFieldName),
                          dbw->getPartPropertySQLType(prop));
        }
    } else {
        // Main table
        {
            schemaStmts << QString(
                    "ALTER TABLE %1 ADD COLUMN %2 %3 NOT NULL DEFAULT %4;"
                    ).arg(dbw->escapeIdentifier(QString("pcat_%1").arg(pcatID)),
                          dbw->escapeIdentifier(propFieldName),
                          dbw->getPartPropertySQLType(prop),
                          prop->getDefaultValueSQLExpression());
        }
    }

    // Meta table entry
    {
        QString fieldsCode;
        QString valsCode;
        QString updateCode;
        generateMetaTypeUpsertParts(dbw, prop->getMetaType(), fieldsCode, valsCode, updateCode);

        schemaStmts << QString(
                "INSERT INTO %1 (\n"
                "    id,\n"
                "%2,\n"
                "    sortidx\n"
                ") VALUES (\n"
                "    %3,   -- id\n"
                "%4,\n"
                "    %5   -- sortidx\n"
                ");"
                ).arg(dbw->escapeIdentifier(QString("pcatmeta_%1").arg(pcatID)),
                      fieldsCode,
                      dbw->escapeString(propFieldName),
                      valsCode,
                      QString::number(prop->getSortIndex()));
    }

    return schemaStmts;
}

QStringList StaticModelManager::generatePartPropertyRemoveCode (
        SQLDatabaseWrapper* dbw, PartProperty* prop, const QString& overrideID, const QString& pcatOverrideID
) {
    QStringList schemaStmts;

    PartCategory* pcat = prop->getPartCategory();

    QString pcatID = pcatOverrideID.isNull() ? pcat->getID() : pcatOverrideID;
    QString propFieldName = overrideID.isNull() ? prop->getFieldName() : overrideID;

    // Meta table entry
    {
        schemaStmts << QString(
                "DELETE FROM %1 WHERE id=%2;"
                ).arg(dbw->escapeIdentifier(QString("pcatmeta_%1").arg(pcatID)),
                      dbw->escapeString(propFieldName));
    }

    if ((prop->getFlags() & PartProperty::MultiValue) != 0) {
        // Multi-value table
        schemaStmts << QString(
                "DROP TABLE %1;"
                ).arg(dbw->escapeIdentifier(QString("pcatmv_%1_%2").arg(pcatID, propFieldName)));
    } else {
        // Main table column
        schemaStmts << QString(
                "ALTER TABLE %1 DROP COLUMN %2;"
                ).arg(dbw->escapeIdentifier(QString("pcat_%1").arg(pcatID)),
                      dbw->escapeIdentifier(propFieldName));
    }

    return schemaStmts;
}

QStringList StaticModelManager::generatePartPropertyEditCode (
        SQLDatabaseWrapper* dbw, PartProperty* oldProp, PartProperty* prop, const QString& pcatOverrideID
) {
    QStringList schemaStmts;

    if ((oldProp->getFlags() & PartProperty::MultiValue) != (prop->getFlags() & PartProperty::MultiValue)) {
        throw InvalidStateException("Can't change multi-value flag of properties!", __FILE__, __LINE__);
    }

    PartCategory* pcat = prop->getPartCategory();

    QString pcatID = pcatOverrideID.isNull() ? pcat->getID() : pcatOverrideID;

    auto editDifferentID = [&](PartProperty* from, const QString& fromID, PartProperty* to, const QString& toID) {
        // Add new property
        schemaStmts << generatePartPropertyAddCode(dbw, to, toID, pcatID);

        if ((to->getFlags() & PartProperty::MultiValue) != 0) {
            // Move multi-value data to new table
            schemaStmts << QString(
                    "INSERT INTO %1 SELECT * FROM %2;"
                    ).arg(dbw->escapeIdentifier(QString("pcatmv_%1_%2").arg(pcatID, toID)),
                          dbw->escapeIdentifier(QString("pcatmv_%1_%2").arg(pcatID, fromID)));
        } else {
            // Copy data from old to new column
            schemaStmts << QString(
                    "UPDATE %1 SET %2=%3;"
                    ).arg(dbw->escapeIdentifier(QString("pcat_%1").arg(pcatID)),
                          dbw->escapeIdentifier(toID),
                          dbw->escapeIdentifier(fromID));
        }

        // Remove old property
        schemaStmts << generatePartPropertyRemoveCode(dbw, from, fromID, pcatID);
    };

    if (oldProp->getFieldName() == prop->getFieldName()) {
        editDifferentID(oldProp, oldProp->getFieldName(), prop, "__temp_id");
        editDifferentID(prop, "__temp_id", prop, prop->getFieldName());
    } else {
        editDifferentID(oldProp, oldProp->getFieldName(), prop, prop->getFieldName());
    }

    return schemaStmts;
}








// **********************************************************
// *                                                        *
// *            META TYPE SCHEMA GENERATION                 *
// *                                                        *
// **********************************************************

QStringList StaticModelManager::generateMetaTypeCode(PartPropertyMetaType* mtype, const QSqlDatabase& targetDb)
{
    QStringList stmts;

    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(targetDb));

    QString fieldsCode;
    QString valsCode;
    QString updateCode;
    generateMetaTypeUpsertParts(dbw.get(), mtype, fieldsCode, valsCode, updateCode);

    stmts << QString(
            "INSERT INTO meta_type (\n"
            "    id,\n"
            "%1\n"
            ") VALUES (\n"
            "    %2,   -- id\n"
            "%3"
            ") %4\n"
            "%5;"
            ).arg(fieldsCode,
                  dbw->escapeString(mtype->metaTypeID),
                  valsCode,
                  dbw->generateUpsertPrefixCode({"id"}),
                  updateCode);

    return stmts;
}

void StaticModelManager::generateMetaTypeUpsertParts (
        SQLDatabaseWrapper* dbw, PartPropertyMetaType* mtype,
        QString& fieldsCode, QString& valsCode, QString& updateCode
) {
    QString typeStr;
    if (mtype->parent) {
        typeStr = mtype->parent->metaTypeID;
    }
    if (typeStr.isNull()) {
        switch (mtype->coreType) {
        case PartProperty::String:
            typeStr = "string";
            break;
        case PartProperty::Integer:
            typeStr = "int";
            break;
        case PartProperty::Float:
            typeStr = "float";
            break;
        case PartProperty::Decimal:
            typeStr = "decimal";
            break;
        case PartProperty::Boolean:
            typeStr = "bool";
            break;
        case PartProperty::File:
            typeStr = "file";
            break;
        case PartProperty::Date:
            typeStr = "date";
            break;
        case PartProperty::Time:
            typeStr = "time";
            break;
        case PartProperty::DateTime:
            typeStr = "datetime";
            break;
        default:
            throw InvalidValueException("Invalid core type for meta type", __FILE__, __LINE__);
        }
    }

    QString displayUnitAffix;
    switch (mtype->unitPrefixType) {
    case PartProperty::UnitPrefixNone:
        displayUnitAffix = "none";
        break;
    case PartProperty::UnitPrefixSIBase10:
        displayUnitAffix = "sibase10";
        break;
    case PartProperty::UnitPrefixSIBase2:
        displayUnitAffix = "sibase2";
        break;
    case PartProperty::UnitPrefixPercent:
        displayUnitAffix = "percent";
        break;
    case PartProperty::UnitPrefixPPM:
        displayUnitAffix = "ppm";
        break;
    case PartProperty::UnitPrefixInvalid:
        break;
    }

    fieldsCode = QString (
            "    name, type,\n"
            "    unit_suffix, display_unit_affix,\n"
            "    int_range_min, int_range_max,\n"
            "    decimal_range_min, decimal_range_max,\n"
            "    string_max_length,\n"
            "    bool_true_text, bool_false_text,\n"
            "    ft_user_prefix,\n"
            "    textarea_min_height,\n"
            "    sql_natural_order_code, sql_ascending_order_code, sql_descending_order_code\n"
            "    "
            );

    QStringList values = {
            mtype->userReadableName.isNull() ? "NULL" : dbw->escapeString(mtype->userReadableName),
            dbw->escapeString(typeStr),

            mtype->unitSuffix.isNull() ? "NULL" : dbw->escapeString(mtype->unitSuffix),
            displayUnitAffix.isNull() ? "NULL" : dbw->escapeString(displayUnitAffix),

            mtype->intRangeMin == std::numeric_limits<int64_t>::max()
                    ? "NULL" : QString::number(mtype->intRangeMin),
            mtype->intRangeMax == std::numeric_limits<int64_t>::min()
                    ? "NULL" : QString::number(mtype->intRangeMax),

            mtype->floatRangeMin == std::numeric_limits<double>::max()
                    ? "NULL" : QString::number(mtype->floatRangeMin, 'g', 54),
            mtype->floatRangeMax == std::numeric_limits<double>::lowest()
                    ? "NULL" : QString::number(mtype->floatRangeMax, 'g', 54),

            mtype->strMaxLen < 0 ? "NULL" : QString::number(mtype->strMaxLen),

            mtype->boolTrueText.isNull() ? "NULL" : dbw->escapeString(mtype->boolTrueText),
            mtype->boolFalseText.isNull() ? "NULL" : dbw->escapeString(mtype->boolFalseText),

            mtype->ftUserPrefix.isNull() ? "NULL" : dbw->escapeString(mtype->ftUserPrefix),

            mtype->textAreaMinHeight < 0 ? "NULL" : QString::number(mtype->textAreaMinHeight),

            mtype->sqlNaturalOrderCode.isNull() ? "NULL" : dbw->escapeString(mtype->sqlNaturalOrderCode),
            mtype->sqlAscendingOrderCode.isNull() ? "NULL" : dbw->escapeString(mtype->sqlAscendingOrderCode),
            mtype->sqlDescendingOrderCode.isNull() ? "NULL" : dbw->escapeString(mtype->sqlDescendingOrderCode)
    };

    valsCode = QString (
            "    %1, %2,   -- name, type\n"
            "    %3, %4,   -- unit_suffix, display_unit_affix\n"
            "    %5, %6,   -- int_range_min, int_range_max\n"
            "    %7, %8,   -- decimal_range_min, decimal_range_max\n"
            "    %9,   -- string_max_length\n"
            "    %10, %11,   -- bool_true_text, bool_false_text\n"
            "    %12,   -- ft_user_prefix\n"
            "    %13,   -- textarea_min_height\n"
            "    %14, %15, %16   -- sql_natural_order_code, sql_ascending_order_code, sql_descending_order_code\n"
            "    "
            );

    updateCode = QString (
            "    name=%1, type=%2,\n"
            "    unit_suffix=%3, display_unit_affix=%4,\n"
            "    int_range_min=%5, int_range_max=%6,\n"
            "    decimal_range_min=%7, decimal_range_max=%8,\n"
            "    string_max_length=%9,\n"
            "    bool_true_text=%10, bool_false_text=%11,\n"
            "    ft_user_prefix=%12,\n"
            "    textarea_min_height=%13,\n"
            "    sql_natural_order_code=%14, sql_ascending_order_code=%15, sql_descending_order_code=%16\n"
            "    "
            );

    struct FlagEntry
    {
        CString fieldName;
        flags_t flagCode;
    };

    const FlagEntry flagEntries[] = {
            {"flag_full_text_indexed", PartProperty::FullTextIndex},
            {"flag_multi_value", PartProperty::MultiValue},
            {"flag_si_prefixes_default_to_base2", PartProperty::SIPrefixesDefaultToBase2},
            {"flag_display_with_units", PartProperty::DisplayWithUnits},
            {"flag_display_selection_list", PartProperty::DisplaySelectionList},
            {"flag_display_hide_from_listing_table", PartProperty::DisplayHideFromListingTable},
            {"flag_display_text_area", PartProperty::DisplayTextArea},
            {"flag_display_multi_in_single_field", PartProperty::DisplayMultiInSingleField},
            {"flag_display_dynamic_enum", PartProperty::DisplayDynamicEnum}
    };

    auto flags = mtype->flags;

    for (size_t i = 0 ; i < sizeof(flagEntries) / sizeof(FlagEntry) ; i++) {
        const FlagEntry& entry = flagEntries[i];

        if ((mtype->flagsWithNonNullValue & entry.flagCode) != 0) {
            fieldsCode = QString("    %1,\n").arg(entry.fieldName)
                    + fieldsCode;
            valsCode = QString("    %1,   -- %2\n")
                    .arg((flags & entry.flagCode) != 0 ? "TRUE" : "FALSE", entry.fieldName)
                    + valsCode;
            updateCode = QString("    %1=%2,\n")
                    .arg(entry.fieldName, (flags & entry.flagCode) != 0 ? "TRUE" : "FALSE")
                    + updateCode;
        }
    }

    for (const QString& val : values) {
        valsCode = valsCode.arg(val);
        updateCode = updateCode.arg(val);
    }
}








// **********************************************************
// *                                                        *
// *                    MISCELLANEOUS                       *
// *                                                        *
// **********************************************************

void StaticModelManager::executeSchemaStatements (
        const QStringList& schemaStmts, bool transaction, const QSqlDatabase& targetDb
) {
    System* sys = System::getInstance();

    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(targetDb));

    bool changelogProcessingWasInhibited = sys->isChangelogProcessingInhibited();
    sys->setChangelogProcessingInhibited(true);

    try {
        auto trans = transaction ? dbw->beginDDLTransaction() : SQLTransaction();

        int stmtIdx = 0;
        for (const QString& stmt : schemaStmts) {
            dbw->execAndCheckQuery(stmt, QString("Error executing schema statement #%1:\n\n%2").arg(stmtIdx).arg(stmt));
            stmtIdx++;
        }

        trans.commit();
    } catch (std::exception&) {
        //LogInfo("Executed the following:\n\n\n%s\n\n\n", dumpSchemaCode(schemaStmts).toUtf8().constData());
        sys->setChangelogProcessingInhibited(changelogProcessingWasInhibited);
        throw;
    }
    sys->setChangelogProcessingInhibited(changelogProcessingWasInhibited);
    dbw->createLogSchema();
}

QString StaticModelManager::dumpSchemaCode (
        const QStringList& schemaStmts, bool dumpTransactionCode, const QSqlDatabase& targetDb
) {
    if (schemaStmts.empty()) {
        return QString();
    }

    QString code;

    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(targetDb));

    if (dumpTransactionCode) {
        if (dbw->supportsTransactionalDDL()) {
            code += "-- Begin DDL transaction\n";
            code += dbw->beginDDLTransactionCode() + "\n";
        } else {
            code += "-- !!! This database doesn't support DDL transactions, so any error will lead to corruption! !!!\n";
        }
    }

    int stmtIdx = 0;
    for (const QString& stmt : schemaStmts) {
        code += QString("-- Statement #%1:\n").arg(stmtIdx);
        code += stmt + "\n";
        stmtIdx++;
    }

    if (dumpTransactionCode  &&  dbw->supportsTransactionalDDL()) {
        code += "-- Commit DDL transaction\n";
        code += dbw->commitDDLTransactionCode() + "\n";
    }

    return code;
}

QStringList parseSchemaCode(const QString& code)
{
    QStringList stmts;

    QRegularExpression regex(";$", QRegularExpression::MultilineOption);
    stmts = code.split(regex, Qt::SkipEmptyParts);

    for (int i = 0 ; i < stmts.size() ; i++) {
        if (!stmts[i].endsWith(';')) {
            stmts[i] += ';';
        }
    }

    return stmts;
}


}
