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
#include <QWidget>
#include <ui_PartLinkTypeEditDialog.h>
#include "../../model/PartLinkType.h"
#include "../util/ChangelogEntry.h"

namespace electronicsdb
{


class PartLinkTypeEditDialog : public QDialog
{
    Q_OBJECT

private:
    struct PTypeEntry : public ChangelogEntry
    {
        PTypeEntry(PartLinkTypeEditDialog* dlg, PartLinkType* ltype) : dlg(dlg), ltype(ltype) {}

        void undo() override;

        PartLinkTypeEditDialog* dlg;
        PartLinkType* ltype;
    };

    struct AddEntry : public PTypeEntry
    {
        AddEntry(PartLinkTypeEditDialog* dlg, PartLinkType* ltype) : PTypeEntry(dlg, ltype) {}

        QString getEntryDescription() const override;
        QString getChangeDescription() const override;
    };

    struct EditEntry : public PTypeEntry
    {
        EditEntry(PartLinkTypeEditDialog* dlg, PartLinkType* ltype) : PTypeEntry(dlg, ltype) {}

        QString getEntryDescription() const override;
        QString getChangeDescription() const override;
    };

    struct RemoveEntry : public PTypeEntry
    {
        RemoveEntry(PartLinkTypeEditDialog* dlg, PartLinkType* ltype, bool wasEdited) : PTypeEntry(dlg, ltype) {}

        QString getEntryDescription() const override;
        QString getChangeDescription() const override;

        bool wasEdited;
    };

public:
    PartLinkTypeEditDialog(QWidget* parent = nullptr);
    virtual ~PartLinkTypeEditDialog();

protected:
    void closeEvent(QCloseEvent* evt) override;

private:
    void init();

    void addLtype(PartLinkType* ltype);
    void removeLtype(PartLinkType* ltype);
    PTypeEntry* editLtype(PartLinkType* ltype);
    void undoEntry(ChangelogEntry* entry);

    void checkGlobalErrors();

    int getLtypeListRow(PartLinkType* ltype);
    void removeLtypeListEntry(PartLinkType* ltype);
    void updateLtypeListItem(PartLinkType* ltype, QListWidgetItem* item);

    QListWidgetItem* createPcatListItem(PartLinkType* ltype, PartCategory* pcat, bool checked = false);

    QStringList generateSchemaStatements();
    void applyToDatabase(const QStringList& stmts);

private slots:
    void addNewLtype();
    void removeCurrentLtype();
    void applyLtypeChanges();

    void displayLtype(PartLinkType* ltype);

    void ltypeListCurrentItemChanged();
    void pcatInvertBoxStateChanged();

    void buttonBoxClicked(QAbstractButton* button);

signals:
    void dialogClosed();

private:
    Ui_PartLinkTypeEditDialog ui;

    QMap<PartLinkType*, PartLinkType*> ltypeCloneToOrigMap;

    PartLinkType* curLtype;

    QString globalErrmsg;
};


}
