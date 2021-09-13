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

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "PartCategory.h"
#include "PartLinkType.h"
#include "PartPropertyMetaType.h"

namespace electronicsdb
{


class StaticModelManager : public QObject
{
    Q_OBJECT

public:
    static StaticModelManager* getInstance();
    static void destroy();

public:
    ~StaticModelManager() {}

    QList<PartPropertyMetaType*> buildPartPropertyMetaTypes();
    QList<PartCategory*> buildCategories();
    QList<PartLinkType*> buildPartLinkTypes();

    QStringList generatePartLinkTypeDeltaCode (
            const QList<PartLinkType*>& added,
            const QList<PartLinkType*>& removed,
            const QMap<PartLinkType*, PartLinkType*>& edited,
            const QSqlDatabase& targetDb = QSqlDatabase::database());

    QStringList generateCategorySchemaDeltaCode (
            const QList<PartCategory*>& added,
            const QList<PartCategory*>& removed,
            const QMap<PartCategory*, PartCategory*>& edited,
            const QMap<PartProperty*, PartProperty*>& editedProps,
            const QSqlDatabase& targetDb = QSqlDatabase::database());

    QStringList generateMetaTypeCode (
            PartPropertyMetaType* mtype,
            const QSqlDatabase& targetDb = QSqlDatabase::database());

    void executeSchemaStatements(const QStringList& schemaStmts, bool transaction = true,
            const QSqlDatabase& targetDb = QSqlDatabase::database());
    QString dumpSchemaCode(const QStringList& schemaStmts, bool dumpTransactionCode = false,
            const QSqlDatabase& targetDb = QSqlDatabase::database());
    QStringList parseSchemaCode(const QString& code);

private:
    void loadDefaultValues(const QList<PartCategory*>& pcats,
            const QList<QMap<QString, QMap<QString, QVariant>>>& columnData);
    void loadMetaType(PartPropertyMetaType* mtype, const QSqlQuery& query, QString& parentMetaTypeID);

    QStringList generatePartLinkTypeAddCode(SQLDatabaseWrapper* dbw, PartLinkType* ltype,
            const QString& overrideID = QString());
    QStringList generatePartLinkTypeRemoveCode(SQLDatabaseWrapper* dbw, PartLinkType* ltype,
            const QString& overrideID = QString());
    QStringList generatePartLinkTypeEditCode(SQLDatabaseWrapper* dbw, PartLinkType* oldLtype, PartLinkType* ltype);

    QStringList generateCategoryAddCode(SQLDatabaseWrapper* dbw, PartCategory* pcat,
            const QString& overrideID = QString());
    QStringList generateCategoryRemoveCode(SQLDatabaseWrapper* dbw, PartCategory* pcat,
            const QString& overrideID = QString());
    QStringList generateCategoryEditCode(SQLDatabaseWrapper* dbw, PartCategory* oldPcat, PartCategory* pcat,
            const QMap<PartProperty*, PartProperty*>& editedProps);
    QStringList generateCategoryRegEntryAddCode(SQLDatabaseWrapper* dbw, PartCategory* pcat,
            const QString& overrideID = QString());
    QStringList generateCategoryRegEntryRemoveCode(SQLDatabaseWrapper* dbw, PartCategory* pcat,
            const QString& overrideID = QString());

    QStringList generatePartPropertyAddCode(SQLDatabaseWrapper* dbw, PartProperty* prop,
            const QString& overrideID = QString(), const QString& pcatOverrideID = QString());
    QStringList generatePartPropertyRemoveCode(SQLDatabaseWrapper* dbw, PartProperty* prop,
            const QString& overrideID = QString(), const QString& pcatOverrideID = QString());
    QStringList generatePartPropertyEditCode(SQLDatabaseWrapper* dbw, PartProperty* oldProp, PartProperty* prop,
            const QString& pcatOverrideID = QString());
    QStringList generateCategorySchemaPropertiesDeltaCode (
            SQLDatabaseWrapper* dbw,
            PartCategory* pcat,
            const QList<PartProperty*>& added,
            const QList<PartProperty*>& removed,
            const QMap<PartProperty*, PartProperty*>& edited,
            const QString& pcatOverrideID = QString());

    void generateMetaTypeUpsertParts (
            SQLDatabaseWrapper* dbw,
            PartPropertyMetaType* mtype,
            QString& fieldsCode, QString& valsCode, QString& updateCode
            );

private:
    template <typename Entity, typename EqFunc>
    void calculateDelta (
            QList<Entity*>& outAdded,
            QList<Entity*>& outRemoved,
            QMap<Entity*, Entity*>& outEdited,
            const QList<Entity*>& from,
            const QList<Entity*>& to,
            const EqFunc& eqFunc
    ) {
        for (Entity* newEnt : to) {
            bool alreadyExists = false;
            for (Entity* oldEnt : from) {
                if (eqFunc(oldEnt, newEnt)) {
                    alreadyExists = true;
                    outEdited[oldEnt] = newEnt;
                    break;
                }
            }
            if (!alreadyExists) {
                outAdded << newEnt;
            }
        }
        for (Entity* oldEnt : from) {
            if (!outAdded.contains(oldEnt)  &&  !outEdited.contains(oldEnt)) {
                outRemoved << oldEnt;
            }
        }
    }

private:
    QList<PartCategory*> cats;
    QList<PartLinkType*> partLinkTypes;

private:
    static StaticModelManager* instance;
};


}
