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

#include "../../global.h"

#include <QDialog>
#include <QList>
#include <QListWidgetItem>
#include <QMap>
#include <QSet>
#include <QWidget>
#include <ui_PartCategoryEditDialog.h>
#include "../../model/AbstractPartProperty.h"
#include "../../model/PartCategory.h"
#include "../../model/PartLinkType.h"
#include "../../model/PartProperty.h"
#include "../util/ChangelogEntry.h"

namespace electronicsdb
{


class PartCategoryEditDialog : public QDialog
{
    Q_OBJECT

private:
    struct PCatEntry : public ChangelogEntry
    {
        PCatEntry(PartCategoryEditDialog* dlg, PartCategory* pcat) : dlg(dlg), pcat(pcat) {}

        void undo() override;

        PartCategoryEditDialog* dlg;
        PartCategory* pcat;
    };

    struct AddEntry : public PCatEntry
    {
        AddEntry(PartCategoryEditDialog* dlg, PartCategory* pcat) : PCatEntry(dlg, pcat) {}

        QString getEntryDescription() const override;
        QString getChangeDescription() const override;
    };

    struct EditEntry : public PCatEntry
    {
        EditEntry(PartCategoryEditDialog* dlg, PartCategory* pcat) : PCatEntry(dlg, pcat) {}

        QString getEntryDescription() const override;
        QString getChangeDescription() const override;
    };

    struct RemoveEntry : public PCatEntry
    {
        RemoveEntry(PartCategoryEditDialog* dlg, PartCategory* pcat, bool wasEdited) : PCatEntry(dlg, pcat) {}

        QString getEntryDescription() const override;
        QString getChangeDescription() const override;

        bool wasEdited;
    };

    struct ChangeOrderEntry : public PCatEntry
    {
        ChangeOrderEntry(PartCategoryEditDialog* dlg) : PCatEntry(dlg, nullptr) {}

        QString getEntryDescription() const override;
        QString getChangeDescription() const override;
    };

private:
    struct FlagEntry
    {
        flags_t flag;
        QCheckBox* box;
        bool invert;
    };

public:
    PartCategoryEditDialog(QWidget* parent = nullptr);
    virtual ~PartCategoryEditDialog();

protected:
    void closeEvent(QCloseEvent* evt) override;

private:
    void init();

    void addPcat(PartCategory* pcat);
    void removePcat(PartCategory* pcat);
    PCatEntry* editPcat(PartCategory* pcat);
    void undoEntry(ChangelogEntry* entry);

    void addProp(PartProperty* prop);
    void removeProp(PartProperty* prop);

    void checkGlobalErrors();

    int getPcatListRow(PartCategory* pcat);
    void removePcatListEntry(PartCategory* pcat);
    void updatePcatListItem(PartCategory* pcat, QListWidgetItem* item);

    int getPropListRow(AbstractPartProperty* aprop);
    void removePropListEntry(PartProperty* prop);
    void updatePropListItem(AbstractPartProperty* aprop, QListWidgetItem* item);

    QStringList generateSchemaStatements();
    void applyToDatabase(const QStringList& stmts);

private slots:
    void addNewPcat();
    void removeCurrentPcat();
    void applyPcatChanges();
    void applyPcatOrderChanges();

    void addNewProp();
    void removeCurrentProp();
    void applyPropChanges();

    void displayPcat(PartCategory* pcat);
    void displayProp(PartProperty* prop);

    void pcatListCurrentItemChanged();
    void propListCurrentItemChanged();

    void pcatListOrderChanged();
    void propListOrderChanged();

    void updateDescInsCb(PartCategory* pcat);
    void descInsCbActivated(int idx);
    void updatePropNumRangeSpinners(PartProperty* prop);

    void propNumRangeMinInfRequested();
    void propNumRangeMaxInfRequested();
    void propNumRangeMinBoxToggled();
    void propNumRangeMaxBoxToggled();
    void propStrMaxLenBoxToggled();

    void buttonBoxClicked(QAbstractButton* button);

signals:
    void dialogClosed();

private:
    Ui_PartCategoryEditDialog ui;

    QMap<PartCategory*, PartCategory*> pcatCloneToOrigMap;
    QMap<PartProperty*, PartProperty*> propCloneToOrigMap;
    QMap<PartLinkType*, PartLinkType*> ltypeCloneToOrigMap;

    QSet<PartProperty*> editedProps;

    PartCategory* curPcat;
    PartProperty* curProp;

    QString globalErrmsg;

    QList<FlagEntry> propFlagEntries;

    bool inhibitApplyChanges;
};


}
