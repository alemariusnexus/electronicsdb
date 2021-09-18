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

#include "PropertyMultiValueWidget.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLocale>
#include <QSplitter>
#include <QVBoxLayout>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../model/part/PartFactory.h"
#include "../../System.h"
#include "../GUIStatePersister.h"
#include "ChoosePartDialog.h"

namespace electronicsdb
{


PropertyMultiValueWidget::PropertyMultiValueWidget (
        PartCategory* pcat, AbstractPartProperty* aprop, QWidget* parent, QObject* keyEventFilter
)       : QWidget(parent), cat(pcat), aprop(aprop), state(Enabled), flags(0)
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);


    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(topLayout);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("mainSplitter");
    topLayout->addWidget(splitter);

    QWidget* listContWidget = new QWidget(splitter);
    splitter->addWidget(listContWidget);

    QVBoxLayout* listContLayout = new QVBoxLayout(listContWidget);
    listContWidget->setLayout(listContLayout);

    listWidget = new QListWidget(listContWidget);
    listWidget->installEventFilter(keyEventFilter);
    connect(listWidget, &QListWidget::currentItemChanged, this, &PropertyMultiValueWidget::currentItemChanged);
    listContLayout->addWidget(listWidget);


    QWidget* buttonWidget = new QWidget(listContWidget);
    listContLayout->addWidget(buttonWidget);

    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonWidget->setLayout(buttonLayout);

    buttonLayout->addStretch(1);

    listAddButton = new QPushButton("", buttonWidget);
    listAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    connect(listAddButton, &QPushButton::clicked, this, &PropertyMultiValueWidget::addRequested);
    buttonLayout->addWidget(listAddButton);

    listRemoveButton = new QPushButton("", buttonWidget);
    listRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));
    connect(listRemoveButton, &QPushButton::clicked, this, &PropertyMultiValueWidget::removeRequested);
    buttonLayout->addWidget(listRemoveButton);

    buttonLayout->addStretch(1);


    QWidget* detailWidget = new QWidget(splitter);
    splitter->addWidget(detailWidget);

    QVBoxLayout* detailLayout = new QVBoxLayout(detailWidget);
    detailWidget->setLayout(detailLayout);

    QWidget* detailFormWidget = new QWidget(detailWidget);
    detailLayout->addWidget(detailFormWidget);

    QFormLayout* detailFormLayout = new QFormLayout(detailFormWidget);
    detailFormLayout->setSizeConstraint(QFormLayout::SetMinimumSize);
    detailFormLayout->setHorizontalSpacing(20);
    detailFormLayout->setVerticalSpacing(10);
    detailFormWidget->setLayout(detailFormLayout);

    if (prop) {
        PartProperty::Type type = prop->getType();
        flags_t flags = prop->getFlags();

        if (type == PartProperty::Boolean) {
            boolBox = new QCheckBox(prop->getUserReadableName(), detailFormWidget);
            boolBox->installEventFilter(keyEventFilter);
            connect(boolBox, &QCheckBox::toggled, this, &PropertyMultiValueWidget::boolFieldToggled);
            detailFormLayout->addRow(boolBox);
        } else if (type == PartProperty::File) {
            fileWidget = new PropertyFileWidget(prop, detailFormWidget, keyEventFilter);
            connect(fileWidget, &PropertyFileWidget::changedByUser, this, &PropertyMultiValueWidget::fileChanged);
            detailFormLayout->addRow(fileWidget);
        } else if ( type == PartProperty::Date
                ||  type == PartProperty::Time
                ||  type == PartProperty::DateTime
        ) {
            dateTimeEdit = new PropertyDateTimeEdit(prop, detailFormWidget, keyEventFilter);
            connect(dateTimeEdit, &PropertyDateTimeEdit::editedByUser, this,
                    &PropertyMultiValueWidget::dateTimeEditedByUser);
            detailFormLayout->addRow(tr("Value"), dateTimeEdit);
        } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
            enumBox = new PropertyComboBox(prop, detailFormWidget);
            enumBox->installEventFilter(keyEventFilter);
            connect(enumBox, &PropertyComboBox::changedByUser, this, &PropertyMultiValueWidget::valueFieldEdited);
            detailFormLayout->addRow(tr("Value"), enumBox);
        } else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
            textEdit = new PropertyTextEdit(prop, detailFormWidget);
            textEdit->installEventFilter(keyEventFilter);
            connect(textEdit, &PropertyTextEdit::editedByUser, this, &PropertyMultiValueWidget::textEditEdited);
            detailFormLayout->addRow(tr("Value"), textEdit);
        } else {
            lineEdit = new PropertyLineEdit(prop, detailFormWidget);
            lineEdit->installEventFilter(keyEventFilter);
            connect(lineEdit, &PropertyLineEdit::textEdited, this, &PropertyMultiValueWidget::valueFieldEdited);
            detailFormLayout->addRow(tr("Value"), lineEdit);
        }

        detailLayout->addStretch(1);
    } else if (ltype) {
        linkWidget = new PartLinkEditWidget(pcat, ltype, detailFormWidget, keyEventFilter);
        connect(linkWidget, &PartLinkEditWidget::changedByUser,
                this, &PropertyMultiValueWidget::linkWidgetChangedByUser);

        detailFormLayout->addRow(linkWidget);

        detailLayout->addStretch(1);
    } else {
        assert(false);
    }

    QString cfgGroup = QString("gui/property_mv_widget/%1/%2").arg(pcat->getID(), aprop->getConfigID());
    GUIStatePersister::getInstance().registerSplitter(cfgGroup, splitter);

    applyState();
}


void PropertyMultiValueWidget::display(const Part& part, QList<QVariant> rawVals)
{
    currentPart = part;

    listWidget->clear();

    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    if (prop) {
        for (const QVariant& val : rawVals) {
            QString desc = prop->formatSingleValueForEditing(val);

            QListWidgetItem* item = new QListWidgetItem(desc, listWidget);
            item->setData(Qt::UserRole, val);
        }
    } else if (ltype) {
        for (const QVariant& val : rawVals) {
            Part lpart = val.value<Part>();

            QListWidgetItem* item = new QListWidgetItem(lpart.getDescription(), listWidget);
            item->setData(Qt::UserRole, val);
        }
    } else {
        assert(false);
    }

    if (listWidget->count() != 0) {
        listWidget->setCurrentRow(0);
    }
}


void PropertyMultiValueWidget::setState(DisplayWidgetState state)
{
    this->state = state;
    applyState();
}


void PropertyMultiValueWidget::setFlags(flags_t flags)
{
    this->flags = flags;
    applyState();
}


QList<QVariant> PropertyMultiValueWidget::getValues(bool initialValueOnError) const
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    QList<QVariant> rawVals;

    for (int i = 0 ; i < listWidget->count() ; i++) {
        QVariant vdata = listWidget->item(i)->data(Qt::UserRole);

        if (vdata.isNull()) {
            if (prop  &&  initialValueOnError) {
                rawVals << prop->getDefaultValue();
            } else {
                throw InvalidValueException(tr("Value #%1 is invalid!").arg(i+1).toUtf8().constData(),
                        __FILE__, __LINE__);
            }
        } else {
            rawVals << vdata;
        }
    }

    return rawVals;
}


void PropertyMultiValueWidget::applyState()
{
    DisplayWidgetState state = this->state;

    if ((this->flags & ChoosePart)  !=  0) {
        state = ReadOnly;

        listAddButton->hide();
        listRemoveButton->hide();
    } else {
        listAddButton->show();
        listRemoveButton->show();
    }

    bool hasCurrentItem = (listWidget->currentItem()  !=  nullptr);

    DisplayWidgetState stateBeforeHasCurrentItem = state;

    if (!hasCurrentItem) {
        state = Disabled;
    }

    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    PartProperty::Type propType = prop ? prop->getType() : PartProperty::InvalidType;
    flags_t propFlags = prop ? prop->getFlags() : 0;

    if (prop) {
        if (propType == PartProperty::File) {
            fileWidget->setState(state);
            fileWidget->setFlags(this->flags);
        } else if ( propType == PartProperty::Date
                ||  propType == PartProperty::Time
                ||  propType == PartProperty::DateTime
        ) {
            dateTimeEdit->setState(state);
            dateTimeEdit->setFlags(this->flags);
        } else if ((propFlags & PartProperty::DisplayDynamicEnum)  !=  0) {
            enumBox->setState(state);
            enumBox->setFlags(this->flags);
        }
    } else if (ltype) {
        linkWidget->setState(state);
        linkWidget->setFlags(this->flags);
    } else {
        assert(false);
    }

    if (state == Enabled) {
        listWidget->setEnabled(true);
        listAddButton->setEnabled(true);
        listRemoveButton->setEnabled(true);

        if (prop) {
            if (propType == PartProperty::Boolean) {
                boolBox->setEnabled(true);
            } else if ((propFlags & PartProperty::DisplayTextArea)  !=  0) {
                textEdit->setEnabled(true);
                textEdit->setReadOnly(false);
            } else if (propType != PartProperty::File  &&  (propFlags & PartProperty::DisplayDynamicEnum) == 0) {
                lineEdit->setEnabled(true);
                lineEdit->setReadOnly(false);
            }
        }
    } else if (state == Disabled) {
        listWidget->setEnabled(false);
        listAddButton->setEnabled(false);
        listRemoveButton->setEnabled(false);

        if (prop) {
            if (propType == PartProperty::Boolean) {
                boolBox->setEnabled(false);
            } else if ((propFlags & PartProperty::DisplayTextArea)  !=  0) {
                textEdit->setEnabled(false);
            } else if (propType != PartProperty::File &&  (propFlags & PartProperty::DisplayDynamicEnum) == 0) {
                lineEdit->setEnabled(false);
            }
        }
    } else if (state == ReadOnly) {
        listWidget->setEnabled(true);
        listAddButton->setEnabled(false);
        listRemoveButton->setEnabled(false);

        if (prop) {
            if (propType == PartProperty::Boolean) {
                boolBox->setEnabled(false);
            } else if ((propFlags & PartProperty::DisplayTextArea)  !=  0) {
                textEdit->setEnabled(true);
                textEdit->setReadOnly(true);
            } else if (propType != PartProperty::File &&  (propFlags & PartProperty::DisplayDynamicEnum) == 0) {
                lineEdit->setEnabled(true);
                lineEdit->setReadOnly(true);
            }
        }
    }

    if (!hasCurrentItem  &&  stateBeforeHasCurrentItem  ==  Enabled) {
        listWidget->setEnabled(true);
        listAddButton->setEnabled(true);
    }
}


void PropertyMultiValueWidget::updateLinkListItem(QListWidgetItem* item)
{
    QString displayStr = tr("(Invalid)");

    QVariant vdata = item->data(Qt::UserRole);
    Part lpart;
    if (!vdata.isNull()) {
        lpart = vdata.value<Part>();
    }

    if (    lpart.isValid()  &&  lpart.hasID()
        &&  lpart.isDataLoaded(lpart.getPartCategory()->getDescriptionProperties())
    ) {
        displayStr = lpart.getDescription();
    }

    item->setText(displayStr);
}


void PropertyMultiValueWidget::currentItemChanged(QListWidgetItem* newItem, QListWidgetItem* oldItem)
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    if (!newItem) {
        if (prop) {
            PartProperty::Type type = prop->getType();
            flags_t flags = prop->getFlags();

            if (type == PartProperty::Boolean) {
                boolBox->blockSignals(true);
                boolBox->setChecked(false);
                boolBox->blockSignals(false);
            } else if (type == PartProperty::File) {
                fileWidget->displayFile(Part(), QString());
            } else if ( type == PartProperty::Date
                    ||  type == PartProperty::Time
                    ||  type == PartProperty::DateTime
            ) {
                dateTimeEdit->display(Part(), QVariant());
            } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                enumBox->displayRecord(Part(), QString());
            } else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
                textEdit->setText("");
            } else {
                lineEdit->clear();
            }
        } else if (ltype) {
            linkWidget->displayLinkedPart(Part());
        } else {
            assert(false);
        }

        applyState();

        return;
    }

    if (prop) {
        PartProperty::Type type = prop->getType();
        flags_t flags = prop->getFlags();

        QVariant rawVal = newItem->data(Qt::UserRole);

        if (type == PartProperty::Boolean) {
            bool val = rawVal.toBool();
            boolBox->blockSignals(true);
            boolBox->setChecked(val);
            boolBox->blockSignals(false);
        } else if (type == PartProperty::File) {
            fileWidget->displayFile(currentPart, rawVal.toString());
        } else if ( type == PartProperty::Date
                ||  type == PartProperty::Time
                ||  type == PartProperty::DateTime
        ) {
            dateTimeEdit->display(currentPart, rawVal);
        } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
            enumBox->displayRecord(currentPart, rawVal);
        } else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
            QString descVal = prop->formatSingleValueForEditing(rawVal);
            textEdit->setText(descVal);
        } else {
            QString descVal = prop->formatSingleValueForEditing(rawVal);
            lineEdit->setText(descVal);
        }
    } else if (ltype) {
        QVariant vdata = newItem->data(Qt::UserRole);
        Part lpart;

        if (!vdata.isNull()) {
            lpart = vdata.value<Part>();
        }

        linkWidget->displayLinkedPart(lpart);
    } else {
        assert(false);
    }

    applyState();
}


void PropertyMultiValueWidget::addRequested()
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    if (prop) {
        QVariant rawVal = prop->getDefaultValue();
        QListWidgetItem* item = new QListWidgetItem(prop->formatSingleValueForDisplay(rawVal), listWidget);
        item->setData(Qt::UserRole, QVariant(rawVal));
        listWidget->setCurrentItem(item);
    } else if (ltype) {
        QList<PartCategory*> candPcats;
        if (ltype->isPartCategoryA(cat)) {
            candPcats = ltype->getPartCategoriesB(System::getInstance()->getPartCategories());
        } else {
            candPcats = ltype->getPartCategoriesA(System::getInstance()->getPartCategories());
        }

        if (candPcats.size() == 1) {
            ChoosePartDialog dlg(candPcats[0], this);
            if (dlg.exec() == ChoosePartDialog::Accepted) {
                PartList parts = dlg.getSelectedParts();

                QListWidgetItem* lastItem = nullptr;
                for (const Part& part : parts) {
                    QListWidgetItem* item = new QListWidgetItem(tr("(Invalid)"), listWidget);
                    lastItem = item;
                    QVariant udata;
                    udata.setValue(part);
                    item->setData(Qt::UserRole, udata);
                    updateLinkListItem(item);
                }

                if (lastItem) {
                    listWidget->setCurrentItem(lastItem);
                }
            }
        } else {
            QListWidgetItem* item = new QListWidgetItem(tr("(Invalid)"), listWidget);
            QVariant udata;
            udata.setValue(Part());
            item->setData(Qt::UserRole, udata);
            listWidget->setCurrentItem(item);
        }
    } else {
        assert(false);
    }

    focusValueWidget();

    emit changedByUser();
}


void PropertyMultiValueWidget::removeRequested()
{
    if (listWidget->currentRow() == -1)
        return;

    delete listWidget->takeItem(listWidget->currentRow());

    emit changedByUser();
}


void PropertyMultiValueWidget::boolFieldToggled(bool val)
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    assert(prop);

    QVariant rawVal = val;

    QListWidgetItem* item = listWidget->currentItem();
    item->setText(prop->formatSingleValueForDisplay(rawVal));
    item->setData(Qt::UserRole, rawVal);

    emit changedByUser();
}


void PropertyMultiValueWidget::valueFieldEdited(const QString&)
{
    // This is also called for the QPlainTextEdit, with an invalid parameter, and for
    // DisplayDynamicEnum types, with a valid parameter!

    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    assert(prop);

    PartProperty::Type type = prop->getType();
    flags_t flags = prop->getFlags();

    QString text;

    if ((flags & PartProperty::DisplayTextArea)  !=  0) {
        text = textEdit->toPlainText();
    } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
        text = enumBox->getUserValue();
    } else {
        text = lineEdit->text();
    }

    QListWidgetItem* item = listWidget->currentItem();

    QVariant rawVal;

    try {
        rawVal = prop->parseSingleUserInputValue(text);
    } catch (InvalidValueException&) {
        //rawVal = prop->getDefaultValue();
        rawVal = QVariant();
    }

    item->setText(prop->formatSingleValueForDisplay(rawVal));

    item->setData(Qt::UserRole, rawVal);

    emit changedByUser();
}


void PropertyMultiValueWidget::textEditEdited()
{
    valueFieldEdited(QString());
}


void PropertyMultiValueWidget::linkWidgetChangedByUser()
{
    QListWidgetItem* item = listWidget->currentItem();
    if (!item) {
        return;
    }

    Part lpart = linkWidget->getLinkedPart();
    QVariant udata;
    if (lpart.isValid()) {
        udata.setValue(lpart);
    }
    item->setData(Qt::UserRole, udata);

    updateLinkListItem(item);

    emit changedByUser();
}


void PropertyMultiValueWidget::fileChanged()
{
    QListWidgetItem* item = listWidget->currentItem();

    QString fpath = fileWidget->getValue();

    item->setText(fpath);
    item->setData(Qt::UserRole, fpath);

    emit changedByUser();
}


void PropertyMultiValueWidget::dateTimeEditedByUser()
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    assert(prop);

    QListWidgetItem* item = listWidget->currentItem();

    QVariant val = dateTimeEdit->getValue();

    item->setText(prop->formatSingleValueForDisplay(val));
    item->setData(Qt::UserRole, val);

    emit changedByUser();
}


void PropertyMultiValueWidget::focusValueWidget()
{
    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    if (listWidget->currentRow() == -1  &&  listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
    }

    if (listWidget->currentRow() == -1) {
        listWidget->setFocus();
    } else {
        if (prop) {
            PartProperty::Type type = prop->getType();
            flags_t flags = prop->getFlags();

            if (type == PartProperty::Boolean) {
                boolBox->setFocus();
            } else if (type == PartProperty::File) {
                fileWidget->setValueFocus();
            } else if ( type == PartProperty::Date
                    ||  type == PartProperty::Time
                    ||  type == PartProperty::DateTime
            ) {
                dateTimeEdit->setValueFocus();
            } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                enumBox->setFocus();
                enumBox->lineEdit()->selectAll();
            } else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
                textEdit->setFocus();
                textEdit->selectAll();
            } else {
                lineEdit->setFocus();
                lineEdit->selectAll();
            }
        } else if (ltype) {
            linkWidget->setValueFocus();
        } else {
            assert(false);
        }
    }
}


}
