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

#include "PartLinkEditWidget.h"

#include <QFont>
#include <QIcon>
#include "../../model/part/PartFactory.h"
#include "../../System.h"
#include "ChoosePartDialog.h"

namespace electronicsdb
{


PartLinkEditWidget::PartLinkEditWidget (
        PartCategory* pcat, PartLinkType* ltype, QWidget* parent, QObject* keyEvtFilter
)       : QWidget(parent), pcat(pcat), ltype(ltype), state(Enabled), flags(0)
{
    ui.setupUi(this);

    System* sys = System::getInstance();

    QFont descLabelFont = ui.descLabel->font();
    descLabelFont.setPointSize(descLabelFont.pointSize()+2);
    descLabelFont.setBold(true);
    ui.descLabel->setFont(descLabelFont);

    ui.pcatCb->addItem(tr("(None)"), QVariant());
    for (PartCategory* lpcat : getLinkedCategories()) {
        QVariant udata;
        udata.setValue(lpcat);
        ui.pcatCb->addItem(lpcat->getUserReadableName(), udata);
    }

    ui.jumpButton->setIcon(QIcon::fromTheme("go-jump", QIcon(":/icons/go-jump.png")));
    ui.quickOpenButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/icons/document-open.png")));

    connect(ui.pcatCb, SIGNAL(currentIndexChanged(int)), this, SLOT(linkedPartChangedByUserDelayed()));
    connect(ui.idEdit, SIGNAL(textChanged(const QString&)), this, SLOT(linkedPartChangedByUserDelayed()));

    connect(ui.chooseButton, &QPushButton::clicked, this, &PartLinkEditWidget::choosePartRequested);
    connect(ui.jumpButton, &QPushButton::clicked, this, &PartLinkEditWidget::jumpRequested);
    connect(ui.quickOpenButton, &QPushButton::clicked, this, &PartLinkEditWidget::quickOpenRequested);

    connect(&delayedChangedTimer, &QTimer::timeout, this, &PartLinkEditWidget::linkedPartChangedByUser);

    delayedChangedTimer.setSingleShot(true);
    delayedChangedTimer.setInterval(250);
}

QList<PartCategory*> PartLinkEditWidget::getLinkedCategories() const
{
    auto allPcats = System::getInstance()->getPartCategories();
    return ltype->isPartCategoryA(pcat) ? ltype->getPartCategoriesB(allPcats) : ltype->getPartCategoriesA(allPcats);
}

void PartLinkEditWidget::displayLinkedPart(const Part& part)
{
    if (!part.isValid()) {
        ui.pcatCb->blockSignals(true);
        ui.pcatCb->setCurrentIndex(0);
        ui.pcatCb->blockSignals(false);
    } else {
        ui.pcatCb->blockSignals(true);
        for (int i = 1 ; i < ui.pcatCb->count() ; i++) {
            PartCategory* lpcat = ui.pcatCb->itemData(i, Qt::UserRole).value<PartCategory*>();
            if (lpcat == part.getPartCategory()) {
                ui.pcatCb->setCurrentIndex(i);
                break;
            }
        }
        ui.pcatCb->blockSignals(false);

        ui.idEdit->blockSignals(true);
        ui.idEdit->setText(QString::number(part.getID()));
        ui.idEdit->blockSignals(false);
    }

    updateState();
}

void PartLinkEditWidget::setState(DisplayWidgetState state)
{
    this->state = state;
    updateState();
}

void PartLinkEditWidget::setFlags(flags_t flags)
{
    this->flags = flags;
    updateState();
}

Part PartLinkEditWidget::getLinkedPart() const
{
    PartCategory* lpcat = getLinkedCategory();
    if (!lpcat) {
        return Part();
    }
    dbid_t id = static_cast<dbid_t>(ui.idEdit->text().toLongLong());
    Part part = PartFactory::getInstance().findPartByID(lpcat, id);
    return part;
}

PartCategory* PartLinkEditWidget::getLinkedCategory() const
{
    QVariant curUdata = ui.pcatCb->currentData(Qt::UserRole);
    if (curUdata.isNull()) {
        return nullptr;
    }
    return curUdata.value<PartCategory*>();
}

void PartLinkEditWidget::setValueFocus()
{
    ui.idEdit->setFocus();
    ui.idEdit->selectAll();
}

void PartLinkEditWidget::updateState()
{
    int pcatIdx = ui.pcatCb->currentIndex();

    if (pcatIdx <= 0) {
        ui.idEdit->setEnabled(false);
    } else {
        ui.idEdit->setEnabled(true);
    }

    PartCategory* lpcat = getLinkedCategory();
    bool hasSingleFileProp = false;

    if (lpcat) {
        for (PartProperty* prop : lpcat->getProperties()) {
            if (prop->getType() == PartProperty::File  &&  (prop->getFlags() & PartProperty::MultiValue) == 0) {
                if (hasSingleFileProp) {
                    hasSingleFileProp = false;
                    break;
                }
                hasSingleFileProp = true;
            }
        }
    }

    ui.quickOpenButton->setEnabled(hasSingleFileProp);

    Part part = getLinkedPart();

    if (part.isValid()  &&  part.hasID()  &&  part.isDataLoaded(part.getPartCategory()->getDescriptionProperties())) {
        ui.descLabel->setText(part.getDescription());
        ui.jumpButton->setEnabled(true);
    } else {
        ui.descLabel->setText(tr("(Invalid)"));
        ui.jumpButton->setEnabled(false);
        ui.quickOpenButton->setEnabled(false);
    }

    if (state == Enabled) {
        ui.idEdit->setEnabled(true);
        ui.descLabel->setEnabled(true);
        if ((flags & ChoosePart) != 0) {
            ui.pcatCb->setEnabled(false);
            ui.idEdit->setReadOnly(true);
            ui.chooseButton->setEnabled(false);
            ui.jumpButton->setEnabled(false);
            ui.quickOpenButton->setEnabled(false);
        } else {
            ui.pcatCb->setEnabled(true);
            ui.idEdit->setReadOnly(false);
            ui.chooseButton->setEnabled(true);
        }
    } else if (state == Disabled) {
        ui.pcatCb->setEnabled(false);
        ui.idEdit->setEnabled(false);
        ui.chooseButton->setEnabled(false);
        ui.descLabel->setEnabled(false);
        ui.jumpButton->setEnabled(false);
        ui.quickOpenButton->setEnabled(false);
    } else if (state == ReadOnly) {
        ui.pcatCb->setEnabled(false);
        ui.idEdit->setEnabled(true);
        ui.idEdit->setReadOnly(true);
        ui.chooseButton->setEnabled(false);
        ui.descLabel->setEnabled(true);
        if ((flags & ChoosePart) != 0) {
            ui.jumpButton->setEnabled(false);
            ui.quickOpenButton->setEnabled(false);
        }
    } else {
        assert(false);
    }
}

void PartLinkEditWidget::linkedPartChangedByUser()
{
    Part part = getLinkedPart();
    if (part.isValid()  &&  part.hasID()) {
        PartFactory::getInstance().loadItemsSingleCategory(&part, &part+1, 0,
                                                           part.getPartCategory()->getDescriptionProperties());
    }

    updateState();
    emit changedByUser();
}

void PartLinkEditWidget::linkedPartChangedByUserDelayed()
{
    delayedChangedTimer.start();
}

void PartLinkEditWidget::choosePartRequested()
{
    PartCategory* lpcat = getLinkedCategory();
    if (!lpcat) {
        return;
    }

    ChoosePartDialog dlg(lpcat, this);
    if (dlg.exec() == ChoosePartDialog::Accepted) {
        displayLinkedPart(dlg.getChosenPart());
        linkedPartChangedByUser();
    }
}

void PartLinkEditWidget::jumpRequested()
{
    Part part = getLinkedPart();
    if (part.isValid()) {
        System::getInstance()->jumpToPart(part);
    }
}

void PartLinkEditWidget::quickOpenRequested()
{
    Part part = getLinkedPart();
    if (!part.isValid()) {
        return;
    }

    PartProperty* fileProp = nullptr;
    for (PartProperty* prop : part.getPartCategory()->getProperties()) {
        if (prop->getType() == PartProperty::File  &&  (prop->getFlags() & PartProperty::MultiValue) == 0) {
            fileProp = prop;
            break;
        }
    }

    if (fileProp) {
        QString fpath = part.getData(fileProp).toString();
        System::getInstance()->openRelativeFile(fpath, this);
    }
}


}
