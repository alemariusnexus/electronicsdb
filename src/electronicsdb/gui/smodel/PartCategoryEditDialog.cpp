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

#include "PartCategoryEditDialog.h"

#include <limits>
#include <QFont>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QSplitter>
#include <nxcommon/log.h>
#include "../../model/StaticModelManager.h"
#include "../../System.h"
#include "SQLExecDialog.h"

// TODO: The QCheckBoxes don't display their PartiallyChecked state when using the classic Windows style (i.e. when
//       dark mode is enabled). Is there any way around that?

namespace electronicsdb
{


void PartCategoryEditDialog::PCatEntry::undo()
{
    dlg->undoEntry(this);
}


QString PartCategoryEditDialog::AddEntry::getEntryDescription() const
{
    return pcat->getID();
}

QString PartCategoryEditDialog::AddEntry::getChangeDescription() const
{
    QString desc = tr("<b>Added:</b>");

    QStringList propList;
    for (PartProperty* prop : pcat->getProperties()) {
        propList << prop->getFieldName();
    }

    desc += "<br/>";
    desc += tr("Properties: %1").arg(propList.join(", "));

    return QString("%1").arg(desc);
}


QString PartCategoryEditDialog::EditEntry::getEntryDescription() const
{
    return pcat->getID();
}

QString PartCategoryEditDialog::EditEntry::getChangeDescription() const
{
    QStringList lines;
    lines << tr("<b>Edited:</b>");

    PartCategory* orig = dlg->pcatCloneToOrigMap[pcat];
    assert(orig);

    QStringList smallChanges;

    if (pcat->getID() != orig->getID()) {
        smallChanges << tr("ID (%1 -> %2)").arg(orig->getID(), pcat->getID());
    }
    if (pcat->getUserReadableName() != orig->getUserReadableName()) {
        smallChanges << tr("Name");
    }
    if (pcat->getDescriptionPattern() != orig->getDescriptionPattern()) {
        smallChanges << tr("Description");
    }

    {
        bool orderChangedFromOrig = false;
        QList<AbstractPartProperty*> origAprops = orig->getAbstractProperties(dlg->ltypeCloneToOrigMap.values());
        int lastOrigIdx = -1;

        for (AbstractPartProperty* aprop : pcat->getAbstractProperties(dlg->ltypeCloneToOrigMap.keys())) {
            PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
            PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);
            AbstractPartProperty* aorig = aprop;
            if (prop) {
                aorig = dlg->propCloneToOrigMap[prop];
            } else if (ltype) {
                aorig = dlg->ltypeCloneToOrigMap[ltype];
            }

            if (aorig) {
                int origIdx = origAprops.indexOf(aorig);

                if (origIdx <= lastOrigIdx) {
                    orderChangedFromOrig = true;
                    break;
                }

                lastOrigIdx = origIdx;
            }
        }

        if (orderChangedFromOrig) {
            smallChanges << tr("Property Order");
        }
    }

    if (!smallChanges.empty()) {
        lines << tr("Changes: %1").arg(smallChanges.join(", "));
    }

    QList<PartProperty*> propsAdded;
    QList<PartProperty*> propsRemoved;
    QMap<PartProperty*, PartProperty*> propsEdited;

    for (PartProperty* prop : pcat->getProperties()) {
        PartProperty* propOrig = dlg->propCloneToOrigMap[prop];
        if (propOrig) {
            if (dlg->editedProps.contains(prop)) {
                propsEdited[propOrig] = prop;
            }
        } else {
            propsAdded << prop;
        }
    }
    for (PartProperty* propOrig : orig->getProperties()) {
        bool removed = true;
        for (auto it = dlg->propCloneToOrigMap.begin() ; it != dlg->propCloneToOrigMap.end() ; ++it) {
            if (it.value() == propOrig  &&  it.key()) {
                removed = false;
                break;
            }
        }
        if (removed) {
            propsRemoved << propOrig;
        }
    }

    QStringList propsAddedStrs;
    QStringList propsRemovedStrs;
    QStringList propsEditedStrs;

    for (PartProperty* prop : propsAdded) {
        propsAddedStrs << prop->getFieldName();
    }
    for (PartProperty* prop : propsRemoved) {
        propsRemovedStrs << prop->getFieldName();
    }
    for (auto it = propsEdited.begin() ; it != propsEdited.end() ; ++it) {
        if (it.key()->getFieldName() == it.value()->getFieldName()) {
            propsEditedStrs << it.key()->getFieldName();
        } else {
            propsEditedStrs << QString("%1 (%2)").arg(it.value()->getFieldName(), it.key()->getFieldName());
        }
    }

    if (!propsAddedStrs.empty()) {
        lines << tr("Properties added: %1").arg(propsAddedStrs.join(", "));
    }
    if (!propsRemovedStrs.empty()) {
        lines << tr("Properties removed: %1").arg(propsRemovedStrs.join(", "));
    }
    if (!propsEditedStrs.isEmpty()) {
        lines << tr("Properties edited: %1").arg(propsEditedStrs.join(", "));
    }

    return lines.join("<br/>");
}


QString PartCategoryEditDialog::RemoveEntry::getEntryDescription() const
{
    return pcat->getID();
}

QString PartCategoryEditDialog::RemoveEntry::getChangeDescription() const
{
    QString desc = tr("<b>Removed</b>");
    return QString("%1").arg(desc);
}


QString PartCategoryEditDialog::ChangeOrderEntry::getEntryDescription() const
{
    return tr("<i>Global</i>");
}

QString PartCategoryEditDialog::ChangeOrderEntry::getChangeDescription() const
{
    return tr("<b>Order changed</b>");
}



PartCategoryEditDialog::PartCategoryEditDialog(QWidget* parent)
        : QDialog(parent), curPcat(nullptr), curProp(nullptr), inhibitApplyChanges(false)
{
    ui.setupUi(this);

    System* sys = System::getInstance();

    setAttribute(Qt::WA_DeleteOnClose, true);

    ui.pcatAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    ui.pcatRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));

    ui.propAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    ui.propRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));

    connect(ui.pcatList, &QListWidget::currentItemChanged, this, &PartCategoryEditDialog::pcatListCurrentItemChanged);
    connect(ui.propList, &QListWidget::currentItemChanged, this, &PartCategoryEditDialog::propListCurrentItemChanged);

    connect(ui.pcatList->model(), &QAbstractItemModel::rowsMoved, this, &PartCategoryEditDialog::pcatListOrderChanged);
    connect(ui.propList->model(), &QAbstractItemModel::rowsMoved, this, &PartCategoryEditDialog::propListOrderChanged);

    connect(ui.pcatAddButton, &QToolButton::clicked, this, &PartCategoryEditDialog::addNewPcat);
    connect(ui.pcatRemoveButton, &QToolButton::clicked, this, &PartCategoryEditDialog::removeCurrentPcat);

    connect(ui.propAddButton, &QToolButton::clicked, this, &PartCategoryEditDialog::addNewProp);
    connect(ui.propRemoveButton, &QToolButton::clicked, this, &PartCategoryEditDialog::removeCurrentProp);

    connect(ui.descInsCb, SIGNAL(activated(int)), this, SLOT(descInsCbActivated(int)));

    connect(ui.propNumRangeMinInfButton, &QToolButton::clicked,
            this, &PartCategoryEditDialog::propNumRangeMinInfRequested);
    connect(ui.propNumRangeMaxInfButton, &QToolButton::clicked,
            this, &PartCategoryEditDialog::propNumRangeMaxInfRequested);

    connect(ui.propNumRangeMinBox, &QCheckBox::stateChanged, this, &PartCategoryEditDialog::propNumRangeMinBoxToggled);
    connect(ui.propNumRangeMaxBox, &QCheckBox::stateChanged, this, &PartCategoryEditDialog::propNumRangeMaxBoxToggled);

    connect(ui.propStrMaxLenBox, &QCheckBox::stateChanged, this, &PartCategoryEditDialog::propStrMaxLenBoxToggled);

    connect(ui.idEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPcatChanges);
    connect(ui.nameEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPcatChanges);
    connect(ui.descEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPcatChanges);

    connect(ui.propIDEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propNameEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propTypeCb, SIGNAL(currentIndexChanged(int)), this, SLOT(applyPropChanges()));
    connect(ui.propFTSPrefixEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propNumUnitSuffixEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propNumUnitPrefixCb, SIGNAL(currentIndexChanged(int)), this, SLOT(applyPropChanges()));
    connect(ui.propNumRangeMinSpinner, SIGNAL(valueChanged(double)), this, SLOT(applyPropChanges()));
    connect(ui.propNumRangeMaxSpinner, SIGNAL(valueChanged(double)), this, SLOT(applyPropChanges()));
    connect(ui.propNumRangeMinBox, &QCheckBox::stateChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propNumRangeMaxBox, &QCheckBox::stateChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propStrMaxLenSpinner, SIGNAL(valueChanged(int)), this, SLOT(applyPropChanges()));
    connect(ui.propStrMaxLenBox, &QCheckBox::stateChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propStrTextAreaMinHeightSpinner, SIGNAL(valueChanged(int)), this, SLOT(applyPropChanges()));
    connect(ui.propBoolTrueTextEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propBoolFalseTextEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    for (QCheckBox* flagCb : ui.propFlagsCont->findChildren<QCheckBox*>()) {
        connect(flagCb, &QCheckBox::stateChanged, this, &PartCategoryEditDialog::applyPropChanges);
    }
    connect(ui.propSQLOrderCodeNatEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propSQLOrderCodeAscEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);
    connect(ui.propSQLOrderCodeDescEdit, &QLineEdit::textChanged, this, &PartCategoryEditDialog::applyPropChanges);

    connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &PartCategoryEditDialog::buttonBoxClicked);
    connect(this, &QDialog::finished, this, &PartCategoryEditDialog::dialogClosed);

    ui.idEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_]{1,64}$"), this));
    ui.propIDEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_]{1,64}$"), this));

    ui.propTypeCb->clear();
    ui.propTypeCb->addItem("bool", QString::number(static_cast<int>(PartProperty::Boolean)));
    ui.propTypeCb->addItem("date", QString::number(static_cast<int>(PartProperty::Date)));
    ui.propTypeCb->addItem("datetime", QString::number(static_cast<int>(PartProperty::DateTime)));
    ui.propTypeCb->addItem("decimal", QString::number(static_cast<int>(PartProperty::Decimal)));
    ui.propTypeCb->addItem("file", QString::number(static_cast<int>(PartProperty::File)));
    ui.propTypeCb->addItem("float", QString::number(static_cast<int>(PartProperty::Float)));
    ui.propTypeCb->addItem("int", QString::number(static_cast<int>(PartProperty::Integer)));
    ui.propTypeCb->addItem("string", QString::number(static_cast<int>(PartProperty::String)));
    ui.propTypeCb->addItem("time", QString::number(static_cast<int>(PartProperty::Time)));
    ui.propTypeCb->insertSeparator(ui.propTypeCb->count());
    QStringList mtypeStrs;
    for (PartPropertyMetaType* mtype : sys->getPartPropertyMetaTypes()) {
        mtypeStrs << mtype->metaTypeID;
    }
    mtypeStrs.sort(Qt::CaseInsensitive);
    for (const QString& mtypeID : mtypeStrs) {
        ui.propTypeCb->addItem(mtypeID, QString("mtype::%1").arg(mtypeID));
    }

    ui.propStrMaxLenSpinner->setMaximum(std::numeric_limits<int>::max());

    ui.propNumUnitPrefixCb->addItem(tr("(Default)"),
                                    QString::number(static_cast<int>(PartProperty::UnitPrefixInvalid)));
    ui.propNumUnitPrefixCb->addItem(tr("SI Base 10 (kB)"),
                                    QString::number(static_cast<int>(PartProperty::UnitPrefixSIBase10)));
    ui.propNumUnitPrefixCb->addItem(tr("SI Base 2 (kiB)"),
                                    QString::number(static_cast<int>(PartProperty::UnitPrefixSIBase2)));
    ui.propNumUnitPrefixCb->addItem(tr("Percent (%)"),
                                    QString::number(static_cast<int>(PartProperty::UnitPrefixPercent)));
    ui.propNumUnitPrefixCb->addItem(tr("Parts Per Million (ppm)"),
                                    QString::number(static_cast<int>(PartProperty::UnitPrefixPPM)));
    ui.propNumUnitPrefixCb->addItem(tr("None"),
                                    QString::number(static_cast<int>(PartProperty::UnitPrefixNone)));

    ui.propNumRangeMinSpinner->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
    ui.propNumRangeMaxSpinner->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());

    propFlagEntries = {
            {PartProperty::FullTextIndex,                   ui.propFlagFTIBox,                  false},
            {PartProperty::MultiValue,                      ui.propFlagMultiValBox,             false},
            {PartProperty::SIPrefixesDefaultToBase2,        ui.propFlagSIDefBase2Box,           false},

            {PartProperty::DisplayWithUnits,                ui.propFlagDispUnitsBox,            false},
            {PartProperty::DisplaySelectionList,            ui.propFlagDispSelListBox,          false},
            {PartProperty::DisplayHideFromListingTable,     ui.propFlagDispHideLTBox,           false},
            {PartProperty::DisplayTextArea,                 ui.propFlagDispTextAreaBox,         false},
            {PartProperty::DisplayMultiInSingleField,       ui.propFlagDispSingleFieldBox,      false},
            {PartProperty::DisplayDynamicEnum,              ui.propFlagDispDynEnumBox,          false}
    };


    QSettings s;

    s.beginGroup("gui/pcat_edit_dialog");

    restoreGeometry(s.value("geometry").toByteArray());
    for (QSplitter* splitter : findChildren<QSplitter*>()) {
        splitter->restoreState(s.value(QString("splitter_state_%1").arg(splitter->objectName())).toByteArray());
    }

    s.endGroup();

    init();


    sys->showDatabaseCorruptionWarningDialog(this);
}

PartCategoryEditDialog::~PartCategoryEditDialog()
{
    for (auto it = pcatCloneToOrigMap.begin() ; it != pcatCloneToOrigMap.end() ; ++it) {
        delete it.key();
    }
    for (auto it = ltypeCloneToOrigMap.begin() ; it != ltypeCloneToOrigMap.end() ; ++it) {
        delete it.key();
    }
}

void PartCategoryEditDialog::closeEvent(QCloseEvent* evt)
{
    QSettings s;

    s.beginGroup("gui/pcat_edit_dialog");

    s.setValue("geometry", saveGeometry());
    for (QSplitter* splitter : findChildren<QSplitter*>()) {
        s.setValue(QString("splitter_state_%1").arg(splitter->objectName()), splitter->saveState());
    }

    s.endGroup();

    s.sync();

    emit dialogClosed();
}

void PartCategoryEditDialog::init()
{
    System* sys = System::getInstance();

    displayPcat(nullptr);
    displayProp(nullptr);

    QMap<PartCategory*, PartCategory*> pcatOrigToCloneMap;
    QList<PartCategory*> pcats = sys->getPartCategories();

    for (PartCategory* pcat : pcats) {
        PartCategory* pcatCloned = pcat->clone();
        pcatCloneToOrigMap[pcatCloned] = pcat;
        pcatOrigToCloneMap[pcat] = pcatCloned;

        for (auto i = 0 ; i < pcat->getPropertyCount() ; i++) {
            propCloneToOrigMap[pcatCloned->getProperties()[i]] = pcat->getProperties()[i];
        }

        QListWidgetItem* item = new QListWidgetItem;
        updatePcatListItem(pcatCloned, item);
        ui.pcatList->addItem(item);
    }


    for (PartLinkType* ltype : sys->getPartLinkTypes()) {
        PartLinkType* cloned = ltype->cloneWithMappedCategories(pcatOrigToCloneMap);
        ltypeCloneToOrigMap[cloned] = ltype;
    }

    checkGlobalErrors();
}

void PartCategoryEditDialog::addPcat(PartCategory* pcat)
{
    pcatCloneToOrigMap[pcat] = nullptr;

    AddEntry* e = new AddEntry(this, pcat);
    ui.clogWidget->addEntry(e);

    QListWidgetItem* item = new QListWidgetItem;
    updatePcatListItem(pcat, item);
    ui.pcatList->addItem(item);

    ui.pcatList->setCurrentItem(item);

    checkGlobalErrors();
}

void PartCategoryEditDialog::removePcat(PartCategory* pcat)
{
    auto model = ui.clogWidget->getModel();

    AddEntry* addEntry = model->getEntry<AddEntry>([&](AddEntry* e) { return e->pcat == pcat; });

    removePcatListEntry(pcat);

    if (addEntry) {
        ui.clogWidget->removeEntry(addEntry);
        pcatCloneToOrigMap.remove(pcat);
        delete pcat;
    } else {
        EditEntry* editEntry = model->getEntry<EditEntry>([&](EditEntry* e) { return e->pcat == pcat; });
        if (editEntry) {
            ui.clogWidget->removeEntry(editEntry);
        }

        RemoveEntry* entry = new RemoveEntry(this, pcat, editEntry != nullptr);
        ui.clogWidget->addEntry(entry);
    }

    checkGlobalErrors();
}

PartCategoryEditDialog::PCatEntry* PartCategoryEditDialog::editPcat(PartCategory* pcat)
{
    auto model = ui.clogWidget->getModel();

    AddEntry* addEntry = model->getEntry<AddEntry>([&](AddEntry* e) { return e->pcat == pcat; });
    EditEntry* editEntry = model->getEntry<EditEntry>([&](EditEntry* e) { return e->pcat == pcat; });

    PCatEntry* resEntry = nullptr;

    if (addEntry) {
        resEntry = addEntry;
    } else if (editEntry) {
        resEntry = editEntry;
    } else {
        EditEntry* entry = new EditEntry(this, pcat);
        ui.clogWidget->addEntry(entry);
        resEntry = entry;
    }

    QListWidgetItem* item = ui.pcatList->item(getPcatListRow(pcat));
    updatePcatListItem(pcat, item);

    checkGlobalErrors();

    return resEntry;
}

void PartCategoryEditDialog::undoEntry(ChangelogEntry* entry)
{
    System* sys = System::getInstance();

    AddEntry* addEntry = dynamic_cast<AddEntry*>(entry);
    RemoveEntry* removeEntry = dynamic_cast<RemoveEntry*>(entry);
    EditEntry* editEntry = dynamic_cast<EditEntry*>(entry);
    ChangeOrderEntry* orderEntry = dynamic_cast<ChangeOrderEntry*>(entry);

    if (addEntry) {
        removePcatListEntry(addEntry->pcat);
        pcatCloneToOrigMap.remove(addEntry->pcat);

        delete addEntry->pcat;
        addEntry->pcat = nullptr;

        ui.clogWidget->removeEntry(addEntry);
    } else if (removeEntry) {
        if (removeEntry->wasEdited) {
            EditEntry* newEntry = new EditEntry(this, removeEntry->pcat);
            ui.clogWidget->addEntry(newEntry);
        }

        QListWidgetItem* item = new QListWidgetItem;
        updatePcatListItem(removeEntry->pcat, item);
        ui.pcatList->addItem(item);

        ui.pcatList->setCurrentItem(item);

        ui.clogWidget->removeEntry(removeEntry);
    } else if (editEntry) {
        PartCategory* pcatOrig = pcatCloneToOrigMap[editEntry->pcat];
        assert(pcatOrig);

        PartCategory* pcat = pcatOrig->clone();

        pcatCloneToOrigMap.remove(editEntry->pcat);
        pcatCloneToOrigMap[pcat] = pcatOrig;

        for (PartProperty* prop : editEntry->pcat->getProperties()) {
            propCloneToOrigMap.remove(prop);
            editedProps.remove(prop);
        }
        for (PartProperty* prop : pcat->getProperties()) {
            propCloneToOrigMap[prop] = pcatOrig->getProperty(prop->getFieldName());
        }

        QListWidgetItem* item = ui.pcatList->item(getPcatListRow(editEntry->pcat));
        updatePcatListItem(pcat, item);

        if (curPcat == editEntry->pcat) {
            displayPcat(pcat);
        }

        delete editEntry->pcat;
        editEntry->pcat = nullptr;

        ui.clogWidget->removeEntry(editEntry);
    } else if (orderEntry) {
        int nextRowIdx = 0;
        for (PartCategory* orig : sys->getPartCategories()) {
            PartCategory* pcat = nullptr;
            for (auto it = pcatCloneToOrigMap.begin() ; it != pcatCloneToOrigMap.end() ; ++it) {
                if (it.value() == orig) {
                    pcat = it.key();
                    break;
                }
            }
            if (pcat) {
                int row = getPcatListRow(pcat);
                if (row >= 0) {
                    QListWidgetItem* item = ui.pcatList->takeItem(row);
                    ui.pcatList->insertItem(nextRowIdx, item);
                    nextRowIdx++;
                }
            }
        }

        applyPcatOrderChanges();

        ui.clogWidget->removeEntry(orderEntry);
    }

    checkGlobalErrors();
}

void PartCategoryEditDialog::addProp(PartProperty* prop)
{
    QListWidgetItem* item = new QListWidgetItem;
    updatePropListItem(prop, item);
    ui.propList->addItem(item);

    ui.propList->setCurrentItem(item);

    propListOrderChanged();

    PCatEntry* entry = editPcat(prop->getPartCategory());
    if (entry) {
        entry->update();
    }
}

void PartCategoryEditDialog::removeProp(PartProperty* prop)
{
    PartCategory* pcat = prop->getPartCategory();

    propCloneToOrigMap.remove(prop);

    removePropListEntry(prop);
    curPcat->removeProperty(prop);

    propListOrderChanged();

    PCatEntry* entry = editPcat(pcat);
    if (entry) {
        entry->update();
    }
}

void PartCategoryEditDialog::checkGlobalErrors()
{
    System* sys = System::getInstance();
    auto model = ui.clogWidget->getModel();

    QString errmsg;

    QSet<PartCategory*> allPcats;

    for (PartCategory* pcat : sys->getPartCategories()) {
        allPcats.insert(pcat);
    }

    for (PCatEntry* e : model->getEntries<PCatEntry>()) {
        if (!e->getErrorMessages().empty()) {
            errmsg = tr("See changelog for errors.");
            break;
        }
        for (PartCategory* pcat : allPcats) {
            if (pcat != e->pcat  &&  pcat != pcatCloneToOrigMap[e->pcat]  &&  pcat->getID() == e->pcat->getID()) {
                errmsg = tr("Duplicate IDs.");
                break;
            }
        }
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

int PartCategoryEditDialog::getPcatListRow(PartCategory* pcat)
{
    for (int i = 0 ; i < ui.pcatList->count() ; i++) {
        PartCategory* listPcat = ui.pcatList->item(i)->data(Qt::UserRole).value<PartCategory*>();
        if (listPcat == pcat) {
            return i;
        }
    }
    return -1;
}

void PartCategoryEditDialog::removePcatListEntry(PartCategory* pcat)
{
    int row = getPcatListRow(pcat);
    if (row >= 0) {
        delete ui.pcatList->takeItem(row);
    }
}

void PartCategoryEditDialog::updatePcatListItem(PartCategory* pcat, QListWidgetItem* item)
{
    PartCategory* orig = pcatCloneToOrigMap[pcat];

    if (orig  &&  orig->getID() != pcat->getID()) {

        item->setText(QString("%1 (%2)").arg(pcat->getID(), orig->getID()));
    } else {
        item->setText(pcat->getID());
    }

    QVariant udata;
    udata.setValue(pcat);
    item->setData(Qt::UserRole, udata);
}

int PartCategoryEditDialog::getPropListRow(AbstractPartProperty* aprop)
{
    for (int i = 0 ; i < ui.propList->count() ; i++) {
        AbstractPartProperty* listProp = ui.propList->item(i)->data(Qt::UserRole).value<AbstractPartProperty*>();
        if (listProp == aprop) {
            return i;
        }
    }
    return -1;
}

void PartCategoryEditDialog::removePropListEntry(PartProperty* prop)
{
    int row = getPropListRow(prop);
    if (row >= 0) {
        delete ui.propList->takeItem(row);
    }
}

void PartCategoryEditDialog::updatePropListItem(AbstractPartProperty* aprop, QListWidgetItem* item)
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    if (prop) {
        PartProperty* orig = propCloneToOrigMap[prop];

        if (orig  &&  orig->getFieldName() != prop->getFieldName()) {
            item->setText(QString("%1 (%2)").arg(prop->getFieldName(), orig->getFieldName()));
        } else {
            item->setText(prop->getFieldName());
        }
    } else if (ltype) {
        item->setText(tr("Link: %1").arg(ltype->getID()));

        if (!item->font().italic()) {
            QFont font = item->font();
            font.setItalic(true);
            item->setFont(font);
        }
    }

    QVariant udata;
    udata.setValue(aprop);
    item->setData(Qt::UserRole, udata);
}

void PartCategoryEditDialog::addNewPcat()
{
    auto model = ui.clogWidget->getModel();

    QSet<QString> takenIDs;

    // All original IDs are blocked, even if the original category was renamed or removed. That's because the backend
    // doesn't guarantee a particular order in which changes are applied, so the new categories might be added before
    // the rename/remove happens, leading to an ID clash.
    for (PartCategory* pcat : System::getInstance()->getPartCategories()) {
        takenIDs.insert(pcat->getID());
    }
    for (AddEntry* entry : model->getEntries<AddEntry>()) {
        takenIDs.insert(entry->pcat->getID());
    }
    for (EditEntry* entry : model->getEntries<EditEntry>()) {
        takenIDs.insert(entry->pcat->getID());
    }

    QString initialIDTemplate = tr("unnamed");
    QString initialID = initialIDTemplate;

    int seqIdx = 1;
    while (takenIDs.contains(initialID)) {
        seqIdx++;
        initialID = QString("%1_%2").arg(initialIDTemplate).arg(seqIdx);
    }

    PartCategory* pcat = new PartCategory(initialID, tr("Unnamed"));

    addPcat(pcat);
}

void PartCategoryEditDialog::removeCurrentPcat()
{
    auto item = ui.pcatList->currentItem();
    if (!item) {
        return;
    }

    PartCategory* pcat = item->data(Qt::UserRole).value<PartCategory*>();
    removePcat(pcat);
}

void PartCategoryEditDialog::applyPcatChanges()
{
    if (inhibitApplyChanges) {
        return;
    }
    if (!curPcat) {
        return;
    }

    System* sys = System::getInstance();

    PartCategory* pcat = curPcat;
    PartCategory* orig = pcatCloneToOrigMap[pcat];

    QStringList errmsgs;

    if (PartCategory::isValidID(ui.idEdit->text())) {
        pcat->setID(ui.idEdit->text());
    } else {
        errmsgs << tr("Invalid ID");
    }

    pcat->setUserReadableName(ui.nameEdit->text());
    pcat->setDescriptionPattern(ui.descEdit->text());

    if (curProp) {
        PartProperty* prop = curProp;
        PartProperty* propOrig = propCloneToOrigMap[prop];

        PartPropertyMetaType* mtype = prop->getMetaType();

        if (PartProperty::isValidFieldName(ui.propIDEdit->text())) {
            prop->setFieldName(ui.propIDEdit->text());
        } else {
            errmsgs << tr("Invalid property ID: %1").arg(ui.propIDEdit->text());
        }

        if (ui.propNameEdit->text().isEmpty()) {
            prop->setUserReadableName(QString());
        } else {
            prop->setUserReadableName(ui.propNameEdit->text());
        }

        PartProperty::Type oldType = prop->getType();

        QString mtypeStr = ui.propTypeCb->currentData(Qt::UserRole).toString();
        if (mtypeStr.startsWith("mtype::")) {
            QString mtypeID = mtypeStr.mid(strlen("mtype::"));
            prop->getMetaType()->parent = sys->getPartPropertyMetaType(mtypeID);
            prop->getMetaType()->coreType = PartProperty::InvalidType;
        } else {
            PartProperty::Type coreType = static_cast<PartProperty::Type>(mtypeStr.toInt());
            prop->getMetaType()->parent = nullptr;
            prop->getMetaType()->coreType = coreType;
        }

        if (prop->getType() != oldType) {
            prop->setDefaultValueFromType();
        }

        if (ui.propFTSPrefixEdit->text().isEmpty()) {
            prop->setFullTextSearchUserPrefix(QString());
        } else {
            prop->setFullTextSearchUserPrefix(ui.propFTSPrefixEdit->text());
        }

        PartProperty::Type ptype = prop->getType();

        if (ptype == PartProperty::String) {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerString);

            if (ui.propStrMaxLenBox->isChecked()) {
                if (ui.propStrMaxLenSpinner->value() == ui.propStrMaxLenSpinner->minimum()) {
                    prop->setStringMaximumLength(std::numeric_limits<int>::max());
                } else {
                    prop->setStringMaximumLength(ui.propStrMaxLenSpinner->value());
                }
            }

            if (ui.propStrTextAreaMinHeightSpinner->value() == ui.propStrTextAreaMinHeightSpinner->minimum()) {
                prop->setTextAreaMinimumHeight(-1);
            } else {
                prop->setTextAreaMinimumHeight(ui.propStrTextAreaMinHeightSpinner->value());
            }
        } else if ( ptype == PartProperty::Integer
                ||  ptype == PartProperty::Float
                ||  ptype == PartProperty::Decimal
        ) {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerNumeric);

            if (ui.propNumUnitSuffixEdit->text().isEmpty()) {
                prop->setUnitSuffix(QString());
            } else {
                prop->setUnitSuffix(ui.propNumUnitSuffixEdit->text());
            }

            prop->setUnitPrefixType(static_cast<PartProperty::UnitPrefixType>(
                    ui.propNumUnitPrefixCb->currentData(Qt::UserRole).toInt()));

            if (ui.propNumRangeMinBox->isChecked()) {
                if (ui.propNumRangeMinSpinner->value() == ui.propNumRangeMinSpinner->minimum()) {
                    // Special value: unlimited
                    if (ptype == PartProperty::Integer) {
                        prop->setIntegerMinimum(std::numeric_limits<int64_t>::min());
                    } else {
                        prop->setDecimalMinimum(std::numeric_limits<double>::lowest());
                    }
                } else {
                    if (ptype == PartProperty::Integer) {
                        prop->setIntegerMinimum(static_cast<int64_t>(ui.propNumRangeMinSpinner->value()));
                    } else {
                        prop->setDecimalMinimum(ui.propNumRangeMinSpinner->value());
                    }
                }
            } else {
                // Inherit value
                if (ptype == PartProperty::Integer) {
                    prop->setIntegerMinimum(std::numeric_limits<int64_t>::max());
                } else {
                    prop->setDecimalMinimum(std::numeric_limits<double>::max());
                }
            }

            if (ui.propNumRangeMaxBox->isChecked()) {
                if (ui.propNumRangeMaxSpinner->value() == ui.propNumRangeMaxSpinner->minimum()) {
                    // Special value: unlimited
                    if (ptype == PartProperty::Integer) {
                        prop->setIntegerMaximum(std::numeric_limits<int64_t>::max());
                    } else {
                        prop->setDecimalMaximum(std::numeric_limits<double>::max());
                    }
                } else {
                    if (ptype == PartProperty::Integer) {
                        prop->setIntegerMaximum(static_cast<int64_t>(ui.propNumRangeMaxSpinner->value()));
                    } else {
                        prop->setDecimalMaximum(ui.propNumRangeMaxSpinner->value());
                    }
                }
            } else {
                // Inherit value
                if (ptype == PartProperty::Integer) {
                    prop->setIntegerMaximum(std::numeric_limits<int64_t>::min());
                } else {
                    prop->setDecimalMaximum(std::numeric_limits<double>::lowest());
                }
            }

            updatePropNumRangeSpinners(prop);
        } else if (ptype == PartProperty::Boolean) {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerBool);

            if (ui.propBoolTrueTextEdit->text().isEmpty()) {
                prop->setBooleanTrueDisplayText(QString());
            } else {
                prop->setBooleanTrueDisplayText(ui.propBoolTrueTextEdit->text());
            }

            if (ui.propBoolFalseTextEdit->text().isEmpty()) {
                prop->setBooleanFalseDisplayText(QString());
            } else {
                prop->setBooleanFalseDisplayText(ui.propBoolFalseTextEdit->text());
            }
        } else {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerOther);
        }

        mtype->flags = 0;
        mtype->flagsWithNonNullValue = 0;

        for (const FlagEntry& e : propFlagEntries) {
            Qt::CheckState state = e.box->checkState();

            if (state == Qt::Checked) {
                mtype->flagsWithNonNullValue |= e.flag;
                mtype->flags |= e.flag;
            } else if (state == Qt::Unchecked) {
                mtype->flagsWithNonNullValue |= e.flag;
                mtype->flags &= ~e.flag;
            } else {
                mtype->flagsWithNonNullValue &= ~e.flag;
                mtype->flags &= ~e.flag;
            }
        }

        if (ui.propSQLOrderCodeNatEdit->text().isEmpty()) {
            prop->setSQLNaturalOrderCode(QString());
        } else {
            prop->setSQLNaturalOrderCode(ui.propSQLOrderCodeNatEdit->text());
        }

        if (ui.propSQLOrderCodeAscEdit->text().isEmpty()) {
            prop->setSQLAscendingOrderCode(QString());
        } else {
            prop->setSQLAscendingOrderCode(ui.propSQLOrderCodeAscEdit->text());
        }

        if (ui.propSQLOrderCodeDescEdit->text().isEmpty()) {
            prop->setSQLDescendingOrderCode(QString());
        } else {
            prop->setSQLDescendingOrderCode(ui.propSQLOrderCodeDescEdit->text());
        }

        QListWidgetItem* item = ui.propList->item(getPropListRow(prop));
        updatePropListItem(prop, item);
    }

    QSet<QString> takenPropIDs;
    if (orig) {
        for (PartProperty* prop : orig->getProperties()) {
            takenPropIDs.insert(prop->getFieldName());
        }
    }
    for (PartProperty* prop : pcat->getProperties()) {
        PartProperty* propOrig = propCloneToOrigMap[prop];
        if (!propOrig  ||  propOrig->getFieldName() != prop->getFieldName()) {
            if (takenPropIDs.contains(prop->getFieldName())) {
                errmsgs << tr("Duplicate property ID: %1").arg(prop->getFieldName());
            }
        }
        takenPropIDs.insert(prop->getFieldName());
    }

    PCatEntry* entry = editPcat(pcat);
    if (entry) {
        entry->setErrorMessages(errmsgs);
        entry->update();
    }

    checkGlobalErrors();
}

void PartCategoryEditDialog::applyPcatOrderChanges()
{
    System* sys = System::getInstance();
    auto model = ui.clogWidget->getModel();

    bool orderChangedFromOrig = false;

    const int sortIdxStep = 1000;
    int nextSortIdx = sortIdxStep;

    int lastOrigIdx = -1;
    for (int i = 0 ; i < ui.pcatList->count() ; i++) {
        PartCategory* pcat = ui.pcatList->item(i)->data(Qt::UserRole).value<PartCategory*>();
        PartCategory* orig = pcatCloneToOrigMap[pcat];

        pcat->setSortIndex(nextSortIdx);
        nextSortIdx += sortIdxStep;

        if (orig) {
            int origIdx = sys->getPartCategories().indexOf(orig);

            if (origIdx <= lastOrigIdx) {
                orderChangedFromOrig = true;
            }

            lastOrigIdx = origIdx;
        }
    }

    ChangeOrderEntry* entry = model->getEntry<ChangeOrderEntry>();

    if (orderChangedFromOrig) {
        if (!entry) {
            entry = new ChangeOrderEntry(this);
            model->addEntry(entry);
        }
    } else {
        if (entry) {
            model->removeEntry(entry);
        }
    }
}

void PartCategoryEditDialog::addNewProp()
{
    if (!curPcat) {
        return;
    }

    PartCategory* pcat = curPcat;
    PartCategory* orig = pcatCloneToOrigMap[pcat];

    QSet<QString> takenIDs;

    // All original IDs are blocked, even if the original property was renamed or removed. That's because the backend
    // doesn't guarantee a particular order in which changes are applied, so the new properties might be added before
    // the rename/remove happens, leading to an ID clash.
    if (orig) {
        for (PartProperty* prop : orig->getProperties()) {
            takenIDs.insert(prop->getFieldName());
        }
    }
    for (PartProperty* prop : pcat->getProperties()) {
        takenIDs.insert(prop->getFieldName());
    }

    QString initialIDTemplate = tr("unnamed");
    QString initialID = initialIDTemplate;

    int seqIdx = 1;
    while (takenIDs.contains(initialID)) {
        seqIdx++;
        initialID = QString("%1_%2").arg(initialIDTemplate).arg(seqIdx);
    }

    PartProperty* prop = new PartProperty(initialID, PartProperty::String, pcat);

    addProp(prop);
}

void PartCategoryEditDialog::removeCurrentProp()
{
    if (!curPcat  ||  !curProp) {
        return;
    }

    removeProp(curProp);
}

void PartCategoryEditDialog::applyPropChanges()
{
    if (inhibitApplyChanges) {
        return;
    }

    if (curProp) {
        editedProps.insert(curProp);
    }
    applyPcatChanges();
}

void PartCategoryEditDialog::displayPcat(PartCategory* pcat)
{
    inhibitApplyChanges = true;

    curPcat = pcat;
    int row = getPcatListRow(pcat);
    ui.pcatList->setCurrentRow(row);

    if (pcat) {
        PartCategory* orig = pcatCloneToOrigMap[pcat];

        curPcat = nullptr;

        QString origID = pcat->getID();
        if (orig) {
            origID = orig->getID();
        }

        ui.idOrigLabel->setText(tr("Original: %1").arg(origID));
        ui.idEdit->setText(pcat->getID());
        ui.nameEdit->setText(pcat->getUserReadableName());
        ui.descEdit->setText(pcat->getDescriptionPattern());
        updateDescInsCb(pcat);

        ui.propList->clear();
        for (AbstractPartProperty* prop : pcat->getAbstractProperties(ltypeCloneToOrigMap.keys())) {
            QListWidgetItem* item = new QListWidgetItem;
            updatePropListItem(prop, item);
            ui.propList->addItem(item);
        }

        curPcat = pcat;

        ui.pcatEditBox->setEnabled(true);
    } else {
        curPcat = nullptr;

        ui.idEdit->clear();
        ui.nameEdit->clear();
        ui.descEdit->clear();
        ui.descInsCb->clear();
        ui.propList->clear();

        ui.pcatEditBox->setEnabled(false);
    }

    inhibitApplyChanges = false;
}

void PartCategoryEditDialog::displayProp(PartProperty* prop)
{
    System* sys = System::getInstance();

    inhibitApplyChanges = true;

    curProp = prop;
    int row = getPropListRow(prop);
    ui.propList->setCurrentRow(row);

    if (prop) {
        PartProperty* orig = propCloneToOrigMap[prop];
        PartPropertyMetaType* mtype = prop->getMetaType();

        curProp = nullptr;

        QString origID = prop->getFieldName();
        if (orig) {
            origID = orig->getFieldName();
        }

        ui.propIDEdit->setText(prop->getFieldName());
        ui.propIDOrigLabel->setText(tr("Original: %1").arg(origID));

        ui.propNameEdit->setText(mtype->userReadableName);
        ui.propNameEdit->setPlaceholderText(mtype->userReadableName.isNull() ? prop->getUserReadableName() : QString());

        QString curMtypeStr;
        if (prop->getMetaType()->parent) {
            curMtypeStr = QString("mtype::%1").arg(prop->getMetaType()->parent->metaTypeID);
        } else {
            curMtypeStr = QString::number(static_cast<int>(prop->getMetaType()->coreType));
        }
        for (int i = 0 ; i < ui.propTypeCb->count() ; i++) {
            if (ui.propTypeCb->itemData(i, Qt::UserRole).toString() == curMtypeStr) {
                ui.propTypeCb->setCurrentIndex(i);
                break;
            }
        }

        ui.propFTSPrefixEdit->setText(mtype->ftUserPrefix);
        ui.propFTSPrefixEdit->setPlaceholderText(mtype->ftUserPrefix.isNull() ? prop->getFullTextSearchUserPrefix()
                : QString());

        PartProperty::Type ptype = prop->getType();

        if (ptype == PartProperty::String) {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerString);
        } else if ( ptype == PartProperty::Integer
                ||  ptype == PartProperty::Float
                ||  ptype == PartProperty::Decimal
        ) {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerNumeric);
        } else if (ptype == PartProperty::Boolean) {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerBool);
        } else {
            ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerOther);
        }

        // NOTE: We update values in the GUI controls for ALL property types. Otherwise, there will be stale values
        // when the user switches property type.

        // String
        {
            ui.propStrMaxLenBox->setChecked(mtype->strMaxLen >= 0);
            if (prop->getStringMaximumLength() < 0  ||  prop->getStringMaximumLength() == std::numeric_limits<int>::max()) {
                ui.propStrMaxLenSpinner->setValue(-1); // Unlimited
            } else {
                ui.propStrMaxLenSpinner->setValue(prop->getStringMaximumLength());
            }

            ui.propStrTextAreaMinHeightSpinner->setSpecialValueText(
                    tr("(Default: %1)").arg(prop->getTextAreaMinimumHeight()));
            if (mtype->textAreaMinHeight < 0) {
                ui.propStrTextAreaMinHeightSpinner->setValue(-1); // Default
            } else {
                ui.propStrTextAreaMinHeightSpinner->setValue(prop->getTextAreaMinimumHeight());
            }
        }

        // Numeric
        {
            ui.propNumUnitSuffixEdit->setText(mtype->unitSuffix);
            ui.propNumUnitSuffixEdit->setPlaceholderText(mtype->unitSuffix.isNull()
                    ? prop->getUnitSuffix() : QString());

            for (int i = 0 ; i < ui.propNumUnitPrefixCb->count() ; i++) {
                if (mtype->unitPrefixType == ui.propNumUnitPrefixCb->itemData(i, Qt::UserRole).toString().toInt()) {
                    ui.propNumUnitPrefixCb->setCurrentIndex(i);
                    break;
                }
            }

            updatePropNumRangeSpinners(prop);
        }

        // Boolean
        {
            ui.propBoolTrueTextEdit->setText(mtype->boolTrueText);
            ui.propBoolTrueTextEdit->setPlaceholderText(mtype->boolTrueText.isNull()
                    ? prop->getBooleanTrueDisplayText() : QString());

            ui.propBoolFalseTextEdit->setText(mtype->boolFalseText);
            ui.propBoolFalseTextEdit->setPlaceholderText(mtype->boolFalseText.isNull()
                    ? prop->getBooleanFalseDisplayText() : QString());
        }

        for (const FlagEntry& e : propFlagEntries) {
            if ((mtype->flagsWithNonNullValue & e.flag) != 0) {
                e.box->setCheckState((mtype->flags & e.flag) != 0 ? Qt::Checked : Qt::Unchecked);
            } else {
                e.box->setCheckState(Qt::PartiallyChecked);
            }
        }

        if (orig) {
            // Changing multi-value to single-value (or vice-versa) is not supported by the backend.
            ui.propFlagMultiValBox->setEnabled(false);
        } else {
            ui.propFlagMultiValBox->setEnabled(true);
        }

        ui.propSQLOrderCodeNatEdit->setText(mtype->sqlNaturalOrderCode);
        ui.propSQLOrderCodeNatEdit->setPlaceholderText(mtype->sqlNaturalOrderCode.isNull()
                ? prop->getSQLNaturalOrderCode() : QString());

        ui.propSQLOrderCodeAscEdit->setText(mtype->sqlAscendingOrderCode);
        ui.propSQLOrderCodeAscEdit->setPlaceholderText(mtype->sqlAscendingOrderCode.isNull()
                ? prop->getSQLAscendingOrderCode() : QString());

        ui.propSQLOrderCodeDescEdit->setText(mtype->sqlDescendingOrderCode);
        ui.propSQLOrderCodeDescEdit->setPlaceholderText(mtype->sqlDescendingOrderCode.isNull()
                ? prop->getSQLDescendingOrderCode() : QString());

        curProp = prop;

        ui.propEditBox->setEnabled(true);
    } else {
        curProp = nullptr;

        ui.propIDEdit->clear();

        ui.propNameEdit->clear();
        ui.propNameEdit->setPlaceholderText(QString());

        ui.propTypeCb->setCurrentIndex(0);

        ui.propFTSPrefixEdit->clear();
        ui.propFTSPrefixEdit->setPlaceholderText(QString());

        ui.propTypeStacker->setCurrentWidget(ui.propTypeStackerOther);

        for (QCheckBox* flagBox : ui.propFlagsCont->findChildren<QCheckBox*>()) {
            flagBox->setChecked(false);
        }

        ui.propSQLOrderCodeNatEdit->clear();
        ui.propSQLOrderCodeNatEdit->setPlaceholderText(QString());

        ui.propSQLOrderCodeAscEdit->clear();
        ui.propSQLOrderCodeAscEdit->setPlaceholderText(QString());

        ui.propSQLOrderCodeDescEdit->clear();
        ui.propSQLOrderCodeDescEdit->setPlaceholderText(QString());

        ui.propEditBox->setEnabled(false);
    }

    inhibitApplyChanges = false;
}

void PartCategoryEditDialog::pcatListCurrentItemChanged()
{
    auto item = ui.pcatList->currentItem();
    PartCategory* pcat = nullptr;
    if (item) {
        pcat = item->data(Qt::UserRole).value<PartCategory*>();
    }
    displayPcat(pcat);
}

void PartCategoryEditDialog::propListCurrentItemChanged()
{
    auto item = ui.propList->currentItem();
    AbstractPartProperty* aprop = nullptr;
    if (item) {
        aprop = item->data(Qt::UserRole).value<AbstractPartProperty*>();
    }

    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    if (prop) {
        displayProp(prop);
    } else {
        displayProp(nullptr);
    }
}

void PartCategoryEditDialog::pcatListOrderChanged()
{
    applyPcatOrderChanges();
}

void PartCategoryEditDialog::propListOrderChanged()
{
    if (!curPcat) {
        return;
    }

    const int sortIdxStep = 1000;
    int nextSortIdx = sortIdxStep;

    for (int i = 0 ; i < ui.propList->count() ; i++) {
        AbstractPartProperty* aprop = ui.propList->item(i)->data(Qt::UserRole).value<AbstractPartProperty*>();
        aprop->setSortIndexInCategory(curPcat, nextSortIdx);
        nextSortIdx += sortIdxStep;
    }

    PCatEntry* entry = editPcat(curPcat);
    if (entry) {
        entry->update();
    }
}

void PartCategoryEditDialog::updateDescInsCb(PartCategory* pcat)
{
    ui.descInsCb->clear();
    ui.descInsCb->addItem(tr("- Insert Property -"), QVariant());
    ui.descInsCb->addItem(tr("ID"), QString("id"));

    for (PartProperty* prop : pcat->getProperties()) {
        QVariant udata;
        udata.setValue(prop);
        ui.descInsCb->addItem(prop->getFieldName(), udata);
    }

    ui.descInsCb->setCurrentIndex(0);
}

void PartCategoryEditDialog::descInsCbActivated(int idx)
{
    if (idx == 0) {
        return;
    }

    QVariant udata = ui.descInsCb->itemData(idx, Qt::UserRole);

    QString insText;

    if (udata.toString() == "id") {
        insText = "${id}";
    } else {
        PartProperty* prop = udata.value<PartProperty*>();
        if (!prop) {
            return;
        }
        insText = QString("${%1}").arg(prop->getFieldName());
    }

    if (!insText.isNull()) {
        ui.descEdit->insert(insText);
    }

    ui.descInsCb->setCurrentIndex(0);

    ui.descEdit->setFocus(Qt::OtherFocusReason);
}

void PartCategoryEditDialog::updatePropNumRangeSpinners(PartProperty* prop)
{
    PartPropertyMetaType* mtype = prop->getMetaType();

    if (prop->getType() == PartProperty::Integer) {
        ui.propNumRangeMinSpinner->setDecimals(0);
        ui.propNumRangeMaxSpinner->setDecimals(0);
        ui.propNumRangeMinSpinner->setRange(static_cast<double>(std::numeric_limits<int64_t>::min()),
                                            static_cast<double>(std::numeric_limits<int64_t>::max()));
        ui.propNumRangeMaxSpinner->setRange(static_cast<double>(std::numeric_limits<int64_t>::min()),
                                            static_cast<double>(std::numeric_limits<int64_t>::max()));
        ui.propNumRangeMinBox->setChecked(mtype->intRangeMin != std::numeric_limits<int64_t>::max());
        ui.propNumRangeMaxBox->setChecked(mtype->intRangeMax != std::numeric_limits<int64_t>::min());
        ui.propNumRangeMinSpinner->setValue(prop->getIntegerMinimum() == std::numeric_limits<int64_t>::min()
                                            ? ui.propNumRangeMinSpinner->minimum() : static_cast<double>(prop->getIntegerMinimum()));
        ui.propNumRangeMaxSpinner->setValue(prop->getIntegerMaximum() == std::numeric_limits<int64_t>::max()
                                            ? ui.propNumRangeMaxSpinner->minimum() : static_cast<double>(prop->getIntegerMaximum()));
    } else {
        ui.propNumRangeMinSpinner->setDecimals(3);
        ui.propNumRangeMaxSpinner->setDecimals(3);
        ui.propNumRangeMinSpinner->setRange(std::numeric_limits<double>::lowest(),
                                            std::numeric_limits<double>::max());
        ui.propNumRangeMaxSpinner->setRange(std::numeric_limits<double>::lowest(),
                                            std::numeric_limits<double>::max());
        ui.propNumRangeMinBox->setChecked(mtype->floatRangeMin != std::numeric_limits<double>::max());
        ui.propNumRangeMaxBox->setChecked(mtype->floatRangeMax != std::numeric_limits<double>::lowest());
        ui.propNumRangeMinSpinner->setValue(prop->getFloatMinimum() == std::numeric_limits<double>::lowest()
                                            ? ui.propNumRangeMinSpinner->minimum() : prop->getFloatMinimum());
        ui.propNumRangeMaxSpinner->setValue(prop->getFloatMaximum() == std::numeric_limits<double>::max()
                                            ? ui.propNumRangeMaxSpinner->minimum() : prop->getFloatMaximum());
    }
}

void PartCategoryEditDialog::propNumRangeMinInfRequested()
{
    // Special value is ALWAYS at minimum
    ui.propNumRangeMinSpinner->setValue(ui.propNumRangeMinSpinner->minimum());
}

void PartCategoryEditDialog::propNumRangeMaxInfRequested()
{
    // Special value is ALWAYS at minimum
    ui.propNumRangeMaxSpinner->setValue(ui.propNumRangeMaxSpinner->minimum());
}

void PartCategoryEditDialog::propNumRangeMinBoxToggled()
{
    ui.propNumRangeMinCont->setEnabled(ui.propNumRangeMinBox->isChecked());
}

void PartCategoryEditDialog::propNumRangeMaxBoxToggled()
{
    ui.propNumRangeMaxCont->setEnabled(ui.propNumRangeMaxBox->isChecked());
}

void PartCategoryEditDialog::propStrMaxLenBoxToggled()
{
    ui.propStrMaxLenSpinner->setEnabled(ui.propStrMaxLenBox->isChecked());
}

QStringList PartCategoryEditDialog::generateSchemaStatements()
{
    auto model = ui.clogWidget->getModel();
    StaticModelManager& smMgr = *StaticModelManager::getInstance();

    QList<PartCategory*> added;
    QList<PartCategory*> removed;
    QMap<PartCategory*, PartCategory*> edited;
    QMap<PartProperty*, PartProperty*> propsEdited;

    for (AddEntry* e : model->getEntries<AddEntry>()) {
        added << e->pcat;
    }
    for (RemoveEntry* e : model->getEntries<RemoveEntry>()) {
        removed << e->pcat;
    }
    for (EditEntry* e : model->getEntries<EditEntry>()) {
        PartCategory* pcatOrig = pcatCloneToOrigMap[e->pcat];
        if (pcatOrig) {
            edited[pcatOrig] = e->pcat;
        }
    }
    ChangeOrderEntry* orderEntry = model->getEntry<ChangeOrderEntry>();
    if (orderEntry) {
        for (auto it = pcatCloneToOrigMap.begin() ; it != pcatCloneToOrigMap.end() ; ++it) {
            edited[it.value()] = it.key();
        }
    }
    for (auto it = propCloneToOrigMap.begin() ; it != propCloneToOrigMap.end() ; ++it) {
        if (it.key()  &&  it.value()) {
            propsEdited[it.value()] = it.key();
        }
    }

    QStringList stmts;

    stmts << smMgr.generateCategorySchemaDeltaCode(added, removed, edited, propsEdited);

    QMap<PartLinkType*, PartLinkType*> ltypesEdited;
    for (auto it = ltypeCloneToOrigMap.begin() ; it != ltypeCloneToOrigMap.end() ; ++it) {
        PartLinkType* ltype = it.key();
        PartLinkType* orig = it.value();
        assert(ltype  &&  orig);
        auto sortIndices = ltype->getSortIndicesForCategories();
        auto sortIndicesOrig = orig->getSortIndicesForCategories();
        for (auto it = sortIndices.begin() ; it != sortIndices.end() ; ++it) {
            if (sortIndicesOrig[pcatCloneToOrigMap[it.key()]] != it.value()) {
                ltypesEdited[orig] = ltype;
                break;
            }
        }
    }

    /*{
        QStringList addedStrs;
        QStringList removedStrs;
        QStringList editedStrs;
        QStringList editedPropsStrs;
        QStringList editedLtypesStrs;

        for (auto pcat : added) {
            addedStrs << pcat->getID();
        }
        for (auto pcat : removed) {
            removedStrs << pcat->getID();
        }
        for (auto it = edited.begin() ; it != edited.end() ; ++it) {
            editedStrs << QString("%1->%2").arg(it.key()->getID(), it.value()->getID());
        }
        for (auto it = propsEdited.begin() ; it != propsEdited.end() ; ++it) {
            editedPropsStrs << QString("%1->%2").arg(it.key()->getFieldName(), it.value()->getFieldName());
        }
        for (auto it = ltypesEdited.begin() ; it != ltypesEdited.end() ; ++it) {
            editedLtypesStrs << QString("%1->%2").arg(it.key()->getID(), it.value()->getID());
        }

        LogInfo("Added:   %s", addedStrs.join(", ").toUtf8().constData());
        LogInfo("Removed: %s", removedStrs.join(", ").toUtf8().constData());
        LogInfo("Edited:  %s", editedStrs.join(", ").toUtf8().constData());
        LogInfo("Props:   %s", editedPropsStrs.join(", ").toUtf8().constData());
        LogInfo("Ltypes:  %s", editedLtypesStrs.join(", ").toUtf8().constData());
    }*/

    stmts << smMgr.generatePartLinkTypeDeltaCode({}, {}, ltypesEdited);

    return stmts;
}

void PartCategoryEditDialog::applyToDatabase(const QStringList& stmts)
{
    System* sys = System::getInstance();
    StaticModelManager& smMgr = *StaticModelManager::getInstance();

    sys->unloadAppModel();

    smMgr.executeSchemaStatements(stmts);

    sys->loadAppModel();
}

void PartCategoryEditDialog::buttonBoxClicked(QAbstractButton* button)
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
