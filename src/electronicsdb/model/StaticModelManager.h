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
            const QMap<PartLinkType*, PartLinkType*>& edited);

    QStringList generateCategorySchemaDeltaCode (
            const QList<PartCategory*>& added,
            const QList<PartCategory*>& removed,
            const QMap<PartCategory*, PartCategory*>& edited,
            const QMap<PartProperty*, PartProperty*>& editedProps);

    QStringList generateMetaTypeCode(PartPropertyMetaType* mtype);

    void executeSchemaStatements(const QStringList& schemaStmts);
    QString dumpSchemaCode(const QStringList& schemaStmts, bool dumpTransactionCode = false);
    QStringList parseSchemaCode(const QString& code);

private:
    void loadDefaultValues(const QList<PartCategory*>& pcats,
            const QList<QMap<QString, QMap<QString, QVariant>>>& columnData);
    void loadMetaType(PartPropertyMetaType* mtype, const QSqlQuery& query, QString& parentMetaTypeID);

    QStringList generatePartLinkTypeAddCode(PartLinkType* ltype, const QString& overrideID = QString());
    QStringList generatePartLinkTypeRemoveCode(PartLinkType* ltype, const QString& overrideID = QString());
    QStringList generatePartLinkTypeEditCode(PartLinkType* oldLtype, PartLinkType* ltype);

    QStringList generateCategoryAddCode(PartCategory* pcat, const QString& overrideID = QString());
    QStringList generateCategoryRemoveCode(PartCategory* pcat, const QString& overrideID = QString());
    QStringList generateCategoryEditCode(PartCategory* oldPcat, PartCategory* pcat,
            const QMap<PartProperty*, PartProperty*>& editedProps);
    QStringList generateCategoryRegEntryAddCode(PartCategory* pcat, const QString& overrideID = QString());
    QStringList generateCategoryRegEntryRemoveCode(PartCategory* pcat, const QString& overrideID = QString());

    QStringList generatePartPropertyAddCode(PartProperty* prop, const QString& overrideID = QString(),
            const QString& pcatOverrideID = QString());
    QStringList generatePartPropertyRemoveCode(PartProperty* prop, const QString& overrideID = QString(),
            const QString& pcatOverrideID = QString());
    QStringList generatePartPropertyEditCode(PartProperty* oldProp, PartProperty* prop,
            const QString& pcatOverrideID = QString());
    QStringList generateCategorySchemaPropertiesDeltaCode (
            PartCategory* pcat,
            const QList<PartProperty*>& added,
            const QList<PartProperty*>& removed,
            const QMap<PartProperty*, PartProperty*>& edited,
            const QString& pcatOverrideID = QString());

    void generateMetaTypeUpsertParts (
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
