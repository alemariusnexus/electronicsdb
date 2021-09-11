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

#include "PartLinkTypeEditDialog.h"

#include <QPalette>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QSplitter>
#include <nxcommon/log.h>
#include "../../model/PartCategory.h"
#include "../../model/StaticModelManager.h"
#include "../../System.h"
#include "SQLExecDialog.h"

namespace electronicsdb
{


void PartLinkTypeEditDialog::PTypeEntry::undo()
{
    dlg->undoEntry(this);
}


QString PartLinkTypeEditDialog::AddEntry::getEntryDescription() const
{
    return ltype->getID();
}

QString PartLinkTypeEditDialog::AddEntry::getChangeDescription() const
{
    QString desc = tr("<b>Added:</b>");

    desc += "<br/>";
    desc += tr("Pattern: %1").arg(ltype->getPatternDescription());

    return QString("%1").arg(desc);
}


QString PartLinkTypeEditDialog::EditEntry::getEntryDescription() const
{
    return ltype->getID();
}

QString PartLinkTypeEditDialog::EditEntry::getChangeDescription() const
{
    QStringList lines;
    lines << tr("<b>Edited:</b>");

    PartLinkType* orig = dlg->ltypeCloneToOrigMap[ltype];
    assert(orig);

    QStringList smallChanges;

    if (ltype->getID() != orig->getID()) {
        smallChanges << tr("ID (%1 -> %2)").arg(orig->getID(), ltype->getID());
    }
    if (ltype->getNameA() != orig->getNameA()) {
        smallChanges << tr("Name A");
    }
    if (ltype->getNameB() != orig->getNameB()) {
        smallChanges << tr("Name B");
    }
    if (ltype->getFlags() != orig->getFlags()) {
        smallChanges << tr("Flags");
    }

    if (!smallChanges.empty()) {
        lines << tr("Changes: %1").arg(smallChanges.join(", "));
    }

    if (    ltype->getPatternType() != orig->getPatternType()
        ||  ltype->getPatternPartCategoriesA() != orig->getPatternPartCategoriesA()
        ||  ltype->getPatternPartCategoriesB() != orig->getPatternPartCategoriesB()
    ) {
        lines << tr("Pattern: %1 -> %2").arg(orig->getPatternDescription(), ltype->getPatternDescription());
    }

    return lines.join("<br/>");
}


QString PartLinkTypeEditDialog::RemoveEntry::getEntryDescription() const
{
    return ltype->getID();
}

QString PartLinkTypeEditDialog::RemoveEntry::getChangeDescription() const
{
    QString desc = tr("<b>Removed</b>");
    return QString("%1").arg(desc);
}



PartLinkTypeEditDialog::PartLinkTypeEditDialog(QWidget* parent)
        : QDialog(parent), curLtype(nullptr)
{
    ui.setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    ui.ltypeAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    ui.ltypeRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));

    connect(ui.ltypeAddButton, &QPushButton::clicked, this, &PartLinkTypeEditDialog::addNewLtype);
    connect(ui.ltypeRemoveButton, &QPushButton::clicked, this, &PartLinkTypeEditDialog::removeCurrentLtype);

    connect(ui.aInvertBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::pcatInvertBoxStateChanged);
    connect(ui.bInvertBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::pcatInvertBoxStateChanged);

    connect(ui.idField, &QLineEdit::textChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.aNameEdit, &QLineEdit::textChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.bNameEdit, &QLineEdit::textChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.aShowBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.bShowBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.aEditableBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.bEditableBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.aDisplaySelListBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.bDisplaySelListBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.aInvertBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.bInvertBox, &QCheckBox::stateChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.aPcatList, &QListWidget::itemChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);
    connect(ui.bPcatList, &QListWidget::itemChanged, this, &PartLinkTypeEditDialog::applyLtypeChanges);

    connect(ui.ltypeList, &QListWidget::currentItemChanged, this, &PartLinkTypeEditDialog::ltypeListCurrentItemChanged);

    connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &PartLinkTypeEditDialog::buttonBoxClicked);
    connect(this, &QDialog::finished, this, &PartLinkTypeEditDialog::dialogClosed);

    ui.idField->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_]{1,64}$"), this));


    QSettings s;

    s.beginGroup("gui/ltype_edit_dialog");

    restoreGeometry(s.value("geometry").toByteArray());
    for (QSplitter* splitter : findChildren<QSplitter*>()) {
        splitter->restoreState(s.value(QString("splitter_state_%1").arg(splitter->objectName())).toByteArray());
    }

    s.endGroup();

    init();


    System::getInstance()->showDatabaseCorruptionWarningDialog(this);
}

PartLinkTypeEditDialog::~PartLinkTypeEditDialog()
{
    for (auto it = ltypeCloneToOrigMap.begin() ; it != ltypeCloneToOrigMap.end() ; ++it) {
        delete it.key();
    }
}

void PartLinkTypeEditDialog::closeEvent(QCloseEvent* evt)
{
    QSettings s;

    s.beginGroup("gui/ltype_edit_dialog");

    s.setValue("geometry", saveGeometry());
    for (QSplitter* splitter : findChildren<QSplitter*>()) {
        s.setValue(QString("splitter_state_%1").arg(splitter->objectName()), splitter->saveState());
    }

    s.endGroup();

    s.sync();

    emit dialogClosed();
}

void PartLinkTypeEditDialog::init()
{
    System* sys = System::getInstance();

    displayLtype(nullptr);

    QList<PartLinkType*> ltypes = sys->getPartLinkTypes();

    for (PartLinkType* ltype : ltypes) {
        PartLinkType* ltypeCloned = ltype->clone();
        ltypeCloneToOrigMap[ltypeCloned] = ltype;

        QListWidgetItem* item = new QListWidgetItem;
        updateLtypeListItem(ltypeCloned, item);
        ui.ltypeList->addItem(item);
    }

    checkGlobalErrors();
}

void PartLinkTypeEditDialog::addLtype(PartLinkType* ltype)
{
    ltypeCloneToOrigMap[ltype] = nullptr;

    AddEntry* e = new AddEntry(this, ltype);
    ui.clogWidget->addEntry(e);

    QListWidgetItem* item = new QListWidgetItem();
    updateLtypeListItem(ltype, item);
    ui.ltypeList->addItem(item);

    ui.ltypeList->setCurrentItem(item);

    checkGlobalErrors();
}

void PartLinkTypeEditDialog::removeLtype(PartLinkType* ltype)
{
    auto model = ui.clogWidget->getModel();

    AddEntry* addEntry = model->getEntry<AddEntry>([&](AddEntry* e) { return e->ltype == ltype; });

    removeLtypeListEntry(ltype);

    if (addEntry) {
        ui.clogWidget->removeEntry(addEntry);
        ltypeCloneToOrigMap.remove(ltype);
        delete ltype;
    } else {
        EditEntry* editEntry = model->getEntry<EditEntry>([&](EditEntry* e) { return e->ltype == ltype; });
        if (editEntry) {
            ui.clogWidget->removeEntry(editEntry);
        }

        RemoveEntry* entry = new RemoveEntry(this, ltype, editEntry != nullptr);
        ui.clogWidget->addEntry(entry);
    }

    checkGlobalErrors();
}

PartLinkTypeEditDialog::PTypeEntry* PartLinkTypeEditDialog::editLtype(PartLinkType* ltype)
{
    auto model = ui.clogWidget->getModel();

    AddEntry* addEntry = model->getEntry<AddEntry>([&](AddEntry* e) { return e->ltype == ltype; });
    EditEntry* editEntry = model->getEntry<EditEntry>([&](EditEntry* e) { return e->ltype == ltype; });

    PTypeEntry* resEntry = nullptr;

    if (addEntry) {
        resEntry = addEntry;
    } else if (editEntry) {
        resEntry = editEntry;
    } else {
        EditEntry* entry = new EditEntry(this, ltype);
        ui.clogWidget->addEntry(entry);
        resEntry = entry;
    }

    QListWidgetItem* item = ui.ltypeList->item(getLtypeListRow(ltype));
    updateLtypeListItem(ltype, item);

    checkGlobalErrors();

    return resEntry;
}

void PartLinkTypeEditDialog::undoEntry(ChangelogEntry* entry)
{
    AddEntry* addEntry = dynamic_cast<AddEntry*>(entry);
    RemoveEntry* removeEntry = dynamic_cast<RemoveEntry*>(entry);
    EditEntry* editEntry = dynamic_cast<EditEntry*>(entry);

    if (addEntry) {
        removeLtypeListEntry(addEntry->ltype);
        ltypeCloneToOrigMap.remove(addEntry->ltype);

        delete addEntry->ltype;
        addEntry->ltype = nullptr;

        ui.clogWidget->removeEntry(addEntry);
    } else if (removeEntry) {
        if (removeEntry->wasEdited) {
            EditEntry* newEntry = new EditEntry(this, removeEntry->ltype);
            ui.clogWidget->addEntry(newEntry);
        }

        QListWidgetItem* item = new QListWidgetItem;
        updateLtypeListItem(removeEntry->ltype, item);
        ui.ltypeList->addItem(item);

        ui.ltypeList->setCurrentItem(item);

        ui.clogWidget->removeEntry(removeEntry);
    } else if (editEntry) {
        PartLinkType* ltypeOrig = ltypeCloneToOrigMap[editEntry->ltype];
        assert(ltypeOrig);

        PartLinkType* ltype = ltypeOrig->clone();

        ltypeCloneToOrigMap.remove(editEntry->ltype);
        ltypeCloneToOrigMap[ltype] = ltypeOrig;

        QListWidgetItem* item = ui.ltypeList->item(getLtypeListRow(editEntry->ltype));
        updateLtypeListItem(ltype, item);

        if (curLtype == editEntry->ltype) {
            displayLtype(ltype);
        }

        delete editEntry->ltype;
        editEntry->ltype = nullptr;

        ui.clogWidget->removeEntry(editEntry);
    }

    checkGlobalErrors();
}

void PartLinkTypeEditDialog::checkGlobalErrors()
{
    System* sys = System::getInstance();
    auto model = ui.clogWidget->getModel();

    QString errmsg;

    QSet<PartLinkType*> allLtypes;

    for (PartLinkType* ltype : sys->getPartLinkTypes()) {
        allLtypes.insert(ltype);
    }

    for (PTypeEntry* e : model->getEntries<PTypeEntry>()) {
        if (!e->getErrorMessages().empty()) {
            errmsg = tr("See changelog for errors.");
            break;
        }
        for (PartLinkType* ltype : allLtypes) {
            if (ltype != e->ltype  &&  ltype != ltypeCloneToOrigMap[e->ltype]  &&  ltype->getID() == e->ltype->getID()) {
                errmsg = tr("Duplicate IDs.");
                break;
            }
        }
        allLtypes.insert(e->ltype);
    }

    if (errmsg.isNull()) {
        ui.globalErrorLabel->setText(tr("No errors."));
        ui.globalErrorLabel->setStyleSheet(QString("QLabel { color: %1; }")
                .arg(sys->getAppPaletteColor(System::PaletteColorStatusOK).name()));
    } else {
        ui.globalErrorLabel->setText(tr("Error: %1").arg(errmsg));
        ui.globalErrorLabel->setStyleSheet(QString("QLabel { color: %1; }")
                .arg(sys->getAppPaletteColor(System::PaletteColorStatusError).name()));
    }

    globalErrmsg = errmsg;
}

QStringList PartLinkTypeEditDialog::generateSchemaStatements()
{
    auto model = ui.clogWidget->getModel();
    StaticModelManager& smMgr = *StaticModelManager::getInstance();

    QList<PartLinkType*> added;
    QList<PartLinkType*> removed;
    QMap<PartLinkType*, PartLinkType*> edited;

    for (AddEntry* e : model->getEntries<AddEntry>()) {
        added << e->ltype;
    }
    for (RemoveEntry* e : model->getEntries<RemoveEntry>()) {
        removed << e->ltype;
    }
    for (EditEntry* e : model->getEntries<EditEntry>()) {
        PartLinkType* ltypeOrig = ltypeCloneToOrigMap[e->ltype];
        if (ltypeOrig) {
            edited[ltypeOrig] = e->ltype;
        }
    }

    return smMgr.generatePartLinkTypeDeltaCode(added, removed, edited);
}

void PartLinkTypeEditDialog::applyToDatabase(const QStringList& stmts)
{
    System* sys = System::getInstance();
    StaticModelManager& smMgr = *StaticModelManager::getInstance();

    sys->unloadAppModel();

    smMgr.executeSchemaStatements(stmts);

    sys->loadAppModel();
}

QListWidgetItem* PartLinkTypeEditDialog::createPcatListItem(PartLinkType* ltype, PartCategory* pcat, bool checked)
{
    QListWidgetItem* item = new QListWidgetItem(QString("%1").arg(pcat->getID()));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);

    QVariant udata;
    udata.setValue(static_cast<void*>(pcat));
    item->setData(Qt::UserRole, udata);

    return item;
}

int PartLinkTypeEditDialog::getLtypeListRow(PartLinkType* ltype)
{
    for (int i = 0 ; i < ui.ltypeList->count() ; i++) {
        PartLinkType* listLtype = static_cast<PartLinkType*>(ui.ltypeList->item(i)->data(Qt::UserRole).value<void*>());
        if (listLtype == ltype) {
            return i;
        }
    }
    return -1;
}

void PartLinkTypeEditDialog::removeLtypeListEntry(PartLinkType* ltype)
{
    int row = getLtypeListRow(ltype);
    if (row >= 0) {
        delete ui.ltypeList->takeItem(row);
    }
}

void PartLinkTypeEditDialog::updateLtypeListItem(PartLinkType* ltype, QListWidgetItem* item)
{
    PartLinkType* ltypeOrig = ltypeCloneToOrigMap[ltype];

    if (ltypeOrig) {
        item->setText(QString("%1 (%2)").arg(ltype->getID(), ltypeOrig->getID()));
    } else {
        item->setText(ltype->getID());
    }

    QVariant udata;
    udata.setValue(static_cast<void*>(ltype));
    item->setData(Qt::UserRole, udata);
}

void PartLinkTypeEditDialog::displayLtype(PartLinkType* ltype)
{
    int row = getLtypeListRow(ltype);
    ui.ltypeList->setCurrentRow(row);

    System* sys = System::getInstance();

    if (ltype) {
        PartLinkType* ltypeOrig = ltypeCloneToOrigMap[ltype];

        curLtype = nullptr;

        QString origID = ltype->getID();
        if (ltypeOrig) {
            origID = ltypeOrig->getID();
        }

        auto flags = ltype->getFlags();

        ui.idOrigLabel->setText(tr("Original: %1").arg(origID));
        ui.idField->setText(ltype->getID());

        ui.aNameEdit->setText(ltype->getNameA());
        ui.bNameEdit->setText(ltype->getNameB());

        ui.aShowBox->setChecked((flags & PartLinkType::ShowInA) != 0);
        ui.bShowBox->setChecked((flags & PartLinkType::ShowInB) != 0);

        ui.aEditableBox->setChecked((flags & PartLinkType::EditInA) != 0);
        ui.bEditableBox->setChecked((flags & PartLinkType::EditInB) != 0);

        ui.aDisplaySelListBox->setChecked((flags & PartLinkType::DisplaySelectionListA) != 0);
        ui.bDisplaySelListBox->setChecked((flags & PartLinkType::DisplaySelectionListB) != 0);

        switch (ltype->getPatternType()) {
        case PartLinkType::PatternTypeANegative:
            ui.aInvertBox->setChecked(true);
            ui.bInvertBox->setChecked(false);
            break;
        case PartLinkType::PatternTypeBNegative:
            ui.aInvertBox->setChecked(false);
            ui.bInvertBox->setChecked(true);
            break;
        case PartLinkType::PatternTypeBothPositive:
            ui.aInvertBox->setChecked(false);
            ui.bInvertBox->setChecked(false);
            break;
        default:
            assert(false);
        }

        ui.aPcatList->clear();
        ui.bPcatList->clear();

        for (PartCategory* pcat : sys->getPartCategories()) {
            ui.aPcatList->addItem(createPcatListItem(ltype, pcat, ltype->getPatternPartCategoriesA().contains(pcat)));
        }
        for (PartCategory* pcat : sys->getPartCategories()) {
            ui.bPcatList->addItem(createPcatListItem(ltype, pcat, ltype->getPatternPartCategoriesB().contains(pcat)));
        }

        curLtype = ltype;

        ui.ltypeEditBox->setEnabled(true);
    } else {
        curLtype = nullptr;

        ui.idField->clear();

        ui.aNameEdit->clear();
        ui.bNameEdit->clear();

        ui.aShowBox->setChecked(false);
        ui.bShowBox->setChecked(false);

        ui.aEditableBox->setChecked(false);
        ui.bEditableBox->setChecked(false);

        ui.aDisplaySelListBox->setChecked(false);
        ui.bDisplaySelListBox->setChecked(false);

        ui.aInvertBox->setChecked(false);
        ui.bInvertBox->setChecked(false);

        ui.aPcatList->clear();
        ui.bPcatList->clear();

        ui.ltypeEditBox->setEnabled(false);
    }
}

void PartLinkTypeEditDialog::ltypeListCurrentItemChanged()
{
    auto item = ui.ltypeList->currentItem();
    PartLinkType* ltype = nullptr;
    if (item) {
        ltype = static_cast<PartLinkType*>(item->data(Qt::UserRole).value<void*>());
    }
    displayLtype(ltype);
}

void PartLinkTypeEditDialog::pcatInvertBoxStateChanged()
{
    if (sender() == ui.aInvertBox) {
        if (ui.aInvertBox->isChecked()) {
            ui.bInvertBox->setChecked(false);
        }
    } else {
        if (ui.bInvertBox->isChecked()) {
            ui.aInvertBox->setChecked(false);
        }
    }
}

void PartLinkTypeEditDialog::addNewLtype()
{
    auto model = ui.clogWidget->getModel();

    QSet<QString> takenIDs;

    // All original IDs are blocked, even if the original link type was renamed or removed. That's because the backend
    // doesn't guarantee a particular order in which changes are applied, so the new link type might be added before the
    // rename/remove happens, leading to an ID clash.
    for (PartLinkType* ltype : System::getInstance()->getPartLinkTypes()) {
        takenIDs.insert(ltype->getID());
    }
    for (AddEntry* entry : model->getEntries<AddEntry>()) {
        takenIDs.insert(entry->ltype->getID());
    }
    for (EditEntry* entry : model->getEntries<EditEntry>()) {
        takenIDs.insert(entry->ltype->getID());
    }

    QString initialIDTemplate = tr("unnamed");
    QString initialID = initialIDTemplate;

    int seqIdx = 1;
    while (takenIDs.contains(initialID)) {
        seqIdx++;
        initialID = QString("%1_%2").arg(initialIDTemplate).arg(seqIdx);
    }

    PartLinkType* ltype = new PartLinkType(initialID, {}, {}, PartLinkType::PatternTypeBothPositive);
    ltype->setFlags(PartLinkType::DefaultForNew);

    addLtype(ltype);
}

void PartLinkTypeEditDialog::removeCurrentLtype()
{
    auto item = ui.ltypeList->currentItem();
    if (!item) {
        return;
    }

    PartLinkType* ltype = static_cast<PartLinkType*>(item->data(Qt::UserRole).value<void*>());
    removeLtype(ltype);
}

void PartLinkTypeEditDialog::applyLtypeChanges()
{
    if (!curLtype) {
        return;
    }

    PartLinkType* ltype = curLtype;
    PartLinkType* ltypeOrig = ltypeCloneToOrigMap[ltype];

    QStringList errmsgs;

    if (PartLinkType::isValidID(ui.idField->text())) {
        ltype->setID(ui.idField->text());
    } else {
        errmsgs << tr("Invalid ID");
    }

    ltype->setNameA(ui.aNameEdit->text());
    ltype->setNameB(ui.bNameEdit->text());

    int flags = 0;

    if (ui.aShowBox->isChecked()) {
        flags |= PartLinkType::ShowInA;
    }
    if (ui.bShowBox->isChecked()) {
        flags |= PartLinkType::ShowInB;
    }
    if (ui.aEditableBox->isChecked()) {
        flags |= PartLinkType::EditInA;
    }
    if (ui.bEditableBox->isChecked()) {
        flags |= PartLinkType::EditInB;
    }
    if (ui.aDisplaySelListBox->isChecked()) {
        flags |= PartLinkType::DisplaySelectionListA;
    }
    if (ui.bDisplaySelListBox->isChecked()) {
        flags |= PartLinkType::DisplaySelectionListB;
    }

    ltype->setFlags(flags);

    PartLinkType::PatternType ptype = PartLinkType::PatternTypeBothPositive;
    if (ui.aInvertBox->isChecked()) {
        ptype = PartLinkType::PatternTypeANegative;
    } else if (ui.bInvertBox->isChecked()) {
        ptype = PartLinkType::PatternTypeBNegative;
    }

    QList<PartCategory*> pcatsA;
    QList<PartCategory*> pcatsB;

    for (int i = 0 ; i < ui.aPcatList->count() ; i++) {
        if (ui.aPcatList->item(i)->checkState() == Qt::Checked) {
            pcatsA << static_cast<PartCategory*>(ui.aPcatList->item(i)->data(Qt::UserRole).value<void*>());
        }
    }
    for (int i = 0 ; i < ui.bPcatList->count() ; i++) {
        if (ui.bPcatList->item(i)->checkState() == Qt::Checked) {
            pcatsB << static_cast<PartCategory*>(ui.bPcatList->item(i)->data(Qt::UserRole).value<void*>());
        }
    }

    QString errmsg;
    if (PartLinkType::checkPattern(pcatsA, pcatsB, ptype, &errmsg)) {
        ltype->setPatternPartCategories(pcatsA, pcatsB, ptype);
    } else {
        errmsgs << tr("Invalid part categories: %1").arg(errmsg);
    }

    PTypeEntry* entry = editLtype(ltype);
    if (entry) {
        entry->setErrorMessages(errmsgs);
        entry->update();
    }

    checkGlobalErrors();
}

void PartLinkTypeEditDialog::buttonBoxClicked(QAbstractButton* button)
{
    auto role = ui.buttonBox->buttonRole(button);

    if (    role == QDialogButtonBox::ApplyRole
        ||  role == QDialogButtonBox::AcceptRole
        ||  role == QDialogButtonBox::YesRole
    ) {
        StaticModelManager& smMgr = *StaticModelManager::getInstance();
        QStringList stmts = generateSchemaStatements();

        SQLExecDialog dlg(this);
        dlg.setSQLCode(smMgr.dumpSchemaCode(stmts, true));

        int res = dlg.exec();
        if (res == QDialog::Accepted) {
            applyToDatabase(stmts);
            accept();
        }
    } else if ( role == QDialogButtonBox::RejectRole
            ||  role == QDialogButtonBox::NoRole
    ) {
        reject();
    }
}


}
