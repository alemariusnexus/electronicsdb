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

#include "PartDisplayWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTabWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <nxcommon/exception/InvalidValueException.h>
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../model/container/PartContainer.h"
#include "../../model/container/PartContainerFactory.h"
#include "../../model/part/PartFactory.h"
#include "../../System.h"
#include "../util/PlainLineEdit.h"
#include "ChoosePartDialog.h"
#include "PartCategoryWidget.h"

namespace electronicsdb
{


PartDisplayWidget::PartDisplayWidget(PartCategory* partCat, QWidget* parent)
        : QWidget(parent), partCat(partCat), state(Enabled), flags(0), localChanges(false)
{
    System* sys = System::getInstance();


    bool hasMultiListProps = false;

    for (PartProperty* prop : partCat->getProperties()) {
        if ((prop->getFlags() & PartProperty::DisplayMultiInSingleField)  ==  0) {
            hasMultiListProps = true;
            break;
        }
    }
    for (PartLinkType* ltype : partCat->getPartLinkTypes()) {
        if ((ltype->getFlagsForCategory(partCat) & (PartLinkType::ShowInA | PartLinkType::ShowInB)) != 0) {
            hasMultiListProps = true;
            break;
        }
    }



    QVBoxLayout* topLayout = new QVBoxLayout(this);
    setLayout(topLayout);

    headerLabel = new QLabel(tr("(No part selected)"));
    /* !!! */ headerLabel->hide(); /* !!! */
    QFont font = headerLabel->font();
    font.setBold(true);
    headerLabel->setFont(font);
    topLayout->addWidget(headerLabel, 0, Qt::AlignCenter);

    tabber = nullptr;
    QScrollArea* formScrollArea;

    if (hasMultiListProps) {
        tabber = new QTabWidget(this);
        topLayout->addWidget(tabber);

        formScrollArea = new QScrollArea(tabber);
        tabber->addTab(formScrollArea, tr("General"));
    } else {
        formScrollArea = new QScrollArea(this);
        topLayout->addWidget(formScrollArea);
    }

    formScrollArea->setObjectName("generalWidget");
    formScrollArea->setWidgetResizable(true);
    formScrollArea->setFrameShadow(QFrame::Plain);

    QWidget* formWidget = new QWidget(formScrollArea);
    formScrollArea->setWidget(formWidget);

    QFormLayout* formLayout = new QFormLayout(formWidget);
    formLayout->setSizeConstraint(QFormLayout::SetMinimumSize);
    formLayout->setHorizontalSpacing(20);
    formLayout->setVerticalSpacing(10);
    formWidget->setLayout(formLayout);

    partIdLabel = new QLabel(tr("-"));
    formLayout->addRow(tr("Part ID"), partIdLabel);

    partContainersLabel = new QLabel(tr("-"));
    partContainersLabel->setWordWrap(true);
    connect(partContainersLabel, &QLabel::linkActivated, this, &PartDisplayWidget::containerLinkActivated);
    formLayout->addRow(tr("Associated Containers"), partContainersLabel);

    for (AbstractPartProperty* aprop : partCat->getAbstractProperties(sys->getPartLinkTypes())) {
        PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
        PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

        PropertyWidgets pw;
        memset(&pw, 0, sizeof(PropertyWidgets));

        if (prop) {
            PartProperty::Type type = prop->getType();
            flags_t flags = prop->getFlags();

            if ((flags & PartProperty::MultiValue) != 0  &&  (flags & PartProperty::DisplayMultiInSingleField) == 0) {
                pw.mvWidget = new PropertyMultiValueWidget(partCat, prop, tabber, this);
                pw.mvWidget->setObjectName(QString("mvPropertyWidget_%1").arg(prop->getFieldName()));
                connect(pw.mvWidget, SIGNAL(changedByUser()), this, SLOT(changedByUser()));

                tabber->addTab(pw.mvWidget, prop->getUserReadableName());
            } else {
                if (type == PartProperty::Boolean) {
                    QLabel* label = new QLabel(prop->getUserReadableName(), formWidget);

                    pw.boolBox = new QCheckBox(formWidget);
                    pw.boolBox->installEventFilter(this);
                    connect(pw.boolBox, SIGNAL(toggled(bool)), this, SLOT(changedByUser()));

                    formLayout->addRow(label, pw.boolBox);
                } else if (type == PartProperty::File) {
                    pw.fileWidget = new PropertyFileWidget(prop, formWidget, this);
                    connect(pw.fileWidget, SIGNAL(changedByUser()), this, SLOT(changedByUser()));

                    formLayout->addRow(prop->getUserReadableName(), pw.fileWidget);
                } else if ( type == PartProperty::Date
                        ||  type == PartProperty::Time
                        ||  type == PartProperty::DateTime
                ) {
                    pw.dateTimeEdit = new PropertyDateTimeEdit(prop, formWidget, this);
                    connect(pw.dateTimeEdit, SIGNAL(editedByUser()), this, SLOT(changedByUser()));

                    formLayout->addRow(prop->getUserReadableName(), pw.dateTimeEdit);
                } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                    pw.enumBox = new PropertyComboBox(prop, formWidget);
                    pw.enumBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
                    pw.enumBox->installEventFilter(this);
                    connect(pw.enumBox, &PropertyComboBox::changedByUser, this, &PartDisplayWidget::changedByUser);

                    formLayout->addRow(prop->getUserReadableName(), pw.enumBox);
                } else {
                    if ((prop->getFlags() & PartProperty::DisplayTextArea)  !=  0) {
                        QLabel* label = new QLabel(prop->getUserReadableName(), formWidget);

                        pw.textEdit = new PropertyTextEdit(prop, formWidget);
                        QSizePolicy pol(QSizePolicy::Expanding, QSizePolicy::Expanding);
                        pol.setVerticalStretch(1);
                        pw.textEdit->setSizePolicy(pol);
                        pw.textEdit->installEventFilter(this);

                        connect(pw.textEdit, &PropertyTextEdit::editedByUser, this, &PartDisplayWidget::changedByUser);

                        formLayout->addRow(label, pw.textEdit);
                    } else {
                        QLabel* label = new QLabel(prop->getUserReadableName(), formWidget);

                        pw.lineEdit = new PropertyLineEdit(prop, formWidget);
                        pw.lineEdit->installEventFilter(this);
                        connect(pw.lineEdit, &PropertyLineEdit::editedByUser, this, &PartDisplayWidget::changedByUser);

                        formLayout->addRow(label, pw.lineEdit);
                    }
                }
            }
        } else if (ltype) {
            if ((ltype->getFlagsForCategory(partCat) & (PartLinkType::ShowInA | PartLinkType::ShowInB)) != 0) {
                pw.mvWidget = new PropertyMultiValueWidget(partCat, ltype, tabber, this);
                pw.mvWidget->setObjectName(QString("mvPropertyWidgetLtype_%1").arg(ltype->getID()));
                connect(pw.mvWidget, SIGNAL(changedByUser()), this, SLOT(changedByUser()));

                tabber->addTab(pw.mvWidget, ltype->isPartCategoryA(partCat) ? ltype->getNameA() : ltype->getNameB());
            }
        } else {
            assert(false);
        }

        propWidgets[aprop] = pw;
    }


    QWidget* buttonWidget = new QWidget(this);
    topLayout->addWidget(buttonWidget);

    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonWidget->setLayout(buttonLayout);

    buttonLayout->addStretch(1);
    dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Reset, Qt::Horizontal, buttonWidget);
    dialogButtonBox->setEnabled(false);
    buttonLayout->addWidget(dialogButtonBox);
    buttonLayout->addStretch(1);

    connect(dialogButtonBox, &QDialogButtonBox::clicked, this, &PartDisplayWidget::buttonBoxClicked);



    applyState();


    EditStack* editStack = sys->getEditStack();


    connect(sys, &System::appModelAboutToBeReset, this, &PartDisplayWidget::clearDisplayedPart);
    connect(sys, &System::saveAllRequested, this, &PartDisplayWidget::saveChanges);
    connect(editStack, &EditStack::undone, this, &PartDisplayWidget::reload);
    connect(editStack, &EditStack::redone, this, &PartDisplayWidget::reload);


    installEventFilter(this);
}


void PartDisplayWidget::setDisplayedPart(const Part& part)
{
    if (!part) {
        currentPart = Part();

        headerLabel->setText(tr("(No part selected)"));

        partIdLabel->setText(tr("-"));
        partContainersLabel->setText(QString(tr("-")));

        for (AbstractPartProperty* aprop : partCat->getAbstractProperties(System::getInstance()->getPartLinkTypes())) {
            PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
            PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

            PropertyWidgets pw = propWidgets[aprop];

            if (prop) {
                PartProperty::Type type = prop->getType();
                flags_t flags = prop->getFlags();

                if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
                    pw.mvWidget->display(Part(), {});
                } else {
                    if (type == PartProperty::Boolean) {
                        pw.boolBox->blockSignals(true);
                        pw.boolBox->setChecked(false);
                        pw.boolBox->blockSignals(false);
                    } else if (type == PartProperty::File) {
                        pw.fileWidget->displayFile(Part(), QString());
                    } else if ( type == PartProperty::Date
                            ||  type == PartProperty::Time
                            ||  type == PartProperty::DateTime
                    ) {
                        pw.dateTimeEdit->display(Part(), QVariant());
                    } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                        pw.enumBox->displayRecord(Part(), QVariant());
                    } else {
                        if ((flags & PartProperty::DisplayTextArea)  !=  0) {
                            pw.textEdit->setText("");
                        } else {
                            pw.lineEdit->setText("");
                        }
                    }
                }
            } else if (ltype) {
                if (pw.mvWidget) {
                    pw.mvWidget->display(Part(), {});
                }
            } else {
                assert(false);
            }
        }

        applyState();
    } else {
        System* sys = System::getInstance();

        bool needLoading = true;
        if (currentPart == part) {
            needLoading = false;
        }

        // This check is quite important for performance reasons. If they are the same part, we DON'T want to overwrite,
        // because 'currentPart' already has the assocs claimed, while 'part' might not. So if we overwrote, we'd
        // needlessly reload the assocs.
        if (currentPart != part) {
            currentPart = part;
        }

        PartFactory::getInstance().loadItemsSingleCategory(&currentPart, &currentPart+1, PartFactory::LoadAll);

        QList<Part> lparts = currentPart.getLinkedParts();
        PartFactory::getInstance().loadItems(lparts.begin(), lparts.end(), 0, [](PartCategory* pcat) {
            return pcat->getDescriptionProperties();
        });

        headerLabel->setText(currentPart.getDescription());
        partIdLabel->setText(QString::number(currentPart.getID()));

        QList<PartContainer> conts = currentPart.getContainers();

        PartContainerFactory::getInstance().loadItems(conts.begin(), conts.end(), 0);

        QString contStr("");
        for (const PartContainer& cont : conts) {
            if (!contStr.isEmpty()) {
                contStr += ", ";
            }
            contStr += QString("<a href=\"%1\">%2</a>").arg(cont.getID()).arg(cont.getName());
        }
        if (contStr.isEmpty()) {
            contStr = tr("-");
        }

        partContainersLabel->setText(contStr);

        const Part::DataMap& data = currentPart.getData();

        for (AbstractPartProperty* aprop : partCat->getAbstractProperties(sys->getPartLinkTypes())) {
            PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
            PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

            PropertyWidgets pw = propWidgets[aprop];

            if (prop) {
                const QVariant& val = data[prop];

                PartProperty::Type type = prop->getType();
                flags_t flags = prop->getFlags();

                if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
                    pw.mvWidget->display(currentPart, val.toList());
                } else {
                    if (type == PartProperty::Boolean) {
                        bool bval = val.toBool();

                        pw.boolBox->blockSignals(true);
                        pw.boolBox->setChecked(bval);
                        pw.boolBox->blockSignals(false);
                    } else if (type == PartProperty::File) {
                        pw.fileWidget->displayFile(currentPart, val.toString());
                    } else if ( type == PartProperty::Date
                            ||  type == PartProperty::Time
                            ||  type == PartProperty::DateTime
                    ) {
                        pw.dateTimeEdit->display(currentPart, val);
                    } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                        pw.enumBox->displayRecord(currentPart, val);
                    } else {
                        if ((prop->getFlags() & PartProperty::DisplayTextArea)  !=  0) {
                            pw.textEdit->setText(prop->formatValueForEditing(val));
                        } else {
                            pw.lineEdit->setText(prop->formatValueForEditing(val));
                        }
                    }
                }
            } else if (ltype) {
                if (pw.mvWidget) {
                    QList<QVariant> varVals;

                    for (PartLinkAssoc assoc : currentPart.getLinkedPartAssocs()) {
                        if (assoc.hasLinkType(ltype)) {
                            Part lpart = assoc.getOther(currentPart);
                            QVariant v;
                            v.setValue(lpart);
                            varVals << v;
                        }
                    }

                    pw.mvWidget->display(currentPart, varVals);
                }
            } else {
                assert(false);
            }
        }

        applyState();
    }

    setHasChanges(false);
}


void PartDisplayWidget::clearDisplayedPart()
{
    setDisplayedPart(Part());
}


void PartDisplayWidget::setState(DisplayWidgetState state)
{
    this->state = state;
    applyState();
}


void PartDisplayWidget::setFlags(flags_t flags)
{
    this->flags = flags;
    applyState();
}


void PartDisplayWidget::applyState()
{
    DisplayWidgetState state = this->state;

    if (!currentPart) {
        state = Disabled;
    }

    dialogButtonBox->setEnabled(state == Enabled);

    if ((flags & ChoosePart)  !=  0) {
        dialogButtonBox->setStandardButtons(QDialogButtonBox::Open);
    } else {
        dialogButtonBox->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Reset);
    }

    for (AbstractPartProperty* aprop : partCat->getAbstractProperties(System::getInstance()->getPartLinkTypes())) {
        PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
        PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

        PropertyWidgets pw = propWidgets[aprop];

        if (prop) {
            PartProperty::Type type = prop->getType();
            flags_t flags = prop->getFlags();

            if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
                pw.mvWidget->setState(state);
                pw.mvWidget->setFlags(this->flags);
            } else {
                if (type == PartProperty::Boolean) {
                    if (state == Enabled  &&  (this->flags & ChoosePart)  ==  0) {
                        pw.boolBox->setEnabled(true);
                    } else {
                        pw.boolBox->setEnabled(false);
                    }
                } else if (type == PartProperty::File) {
                    pw.fileWidget->setState(state);
                    pw.fileWidget->setFlags(this->flags);
                } else if ( type == PartProperty::Date
                        ||  type == PartProperty::Time
                        ||  type == PartProperty::DateTime
                ) {
                    pw.dateTimeEdit->setState(state);
                    pw.dateTimeEdit->setFlags(this->flags);
                } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                    pw.enumBox->setState(state);
                    pw.enumBox->setFlags(this->flags);
                } else {
                    if ((flags & PartProperty::DisplayTextArea)  !=  0) {
                        pw.textEdit->setState(state);
                        pw.textEdit->setFlags(this->flags);
                    } else {
                        pw.lineEdit->setState(state);
                        pw.lineEdit->setFlags(this->flags);
                    }
                }
            }
        } else if (ltype) {
            if (pw.mvWidget) {
                pw.mvWidget->setState(state);
                pw.mvWidget->setFlags(this->flags);
            }
        } else {
            assert(false);
        }
    }
}


bool PartDisplayWidget::saveChanges()
{
    if (!currentPart) {
        return true;
    }
    if (!hasLocalChanges()) {
        return true;
    }

    QSet<PartLinkType*> ltypesToRebuild;
    for (PartLinkType* ltype : partCat->getPartLinkTypes()) {
        if (    (ltype->getFlagsForCategory(partCat) & (PartLinkType::ShowInA | PartLinkType::ShowInB)) != 0
            &&  (ltype->getFlagsForCategory(partCat) & (PartLinkType::EditInA | PartLinkType::EditInB)) != 0
        ) {
            ltypesToRebuild.insert(ltype);
        }
    }

    currentPart.unlinkPartLinkTypes(ltypesToRebuild);

    Part::DataMap data;

    for (auto it = propWidgets.begin() ; it != propWidgets.end() ; ++it) {
        AbstractPartProperty* aprop = it.key();
        PropertyWidgets pw = it.value();

        PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
        PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

        if (prop) {
            PartProperty::Type type = prop->getType();
            flags_t flags = prop->getFlags();

            try {
                if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
                    data[prop] = pw.mvWidget->getValues(true);
                } else {
                    if (type == PartProperty::Boolean) {
                        data[prop] = pw.boolBox->isChecked();
                    } else if (type == PartProperty::File) {
                        QString fpath = pw.fileWidget->getValue();

                        QString relPath = handleFile(prop, fpath);

                        if (relPath.isNull()) {
                            return false;
                        }

                        data[prop] = relPath;
                    } else if ( type == PartProperty::Date
                            ||  type == PartProperty::Time
                            ||  type == PartProperty::DateTime
                    ) {
                        data[prop] = pw.dateTimeEdit->getValue();
                    } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                        data[prop] = pw.enumBox->getRawValue();
                    } else {
                        if ((flags & PartProperty::DisplayTextArea)  !=  0) {
                            data[prop] = pw.textEdit->getValue();
                        } else {
                            data[prop] = pw.lineEdit->getValue();
                        }
                    }
                }
            } catch (InvalidValueException&) {
                /*QMessageBox::warning(this, "Invalid Value",
                        QString("Invalid value for property '%1':\n\n%2").arg(prop->getUserReadableName()).arg(ex.getMessage()));
                return false;*/
                data[prop] = prop->getDefaultValue();
            }
        } else if (ltype) {
            if (pw.mvWidget  &&  ltypesToRebuild.contains(ltype)) {
                QList<QVariant> varVals = pw.mvWidget->getValues();

                for (const QVariant& v : varVals) {
                    Part lpart = v.value<Part>();
                    if (!lpart.isValid()) {
                        continue;
                    }
                    currentPart.linkPart(lpart, {ltype});
                }
            }
        } else {
            assert(false);
        }
    }

    setHasChanges(false);

    currentPart.setData(data);
    PartFactory::getInstance().updateItem(currentPart);

    reload();

    return true;
}


void PartDisplayWidget::buttonBoxClicked(QAbstractButton* b)
{
    System* sys = System::getInstance();

    if ((flags & ChoosePart)  ==  0) {
        if (dialogButtonBox->standardButton(b) == QDialogButtonBox::Apply) {
            saveChanges();
        } else {
            setDisplayedPart(currentPart);
        }
    } else {
        emit partChosen(currentPart);
    }
}


void PartDisplayWidget::setHasChanges(bool hasChanges)
{
    if (state != Enabled  ||  (flags & ChoosePart)  !=  0)
        return;

    bool changed = localChanges != hasChanges;

    dialogButtonBox->setEnabled(hasChanges);
    localChanges = hasChanges;

    if (changed) {
        emit hasLocalChangesStatusChanged(hasChanges);
    }
}


void PartDisplayWidget::changedByUser(const QString&)
{
    setHasChanges(true);
}


QString PartDisplayWidget::handleFile(PartProperty* prop, const QString& fpath)
{
    System* sys = System::getInstance();

    DatabaseConnection* conn = sys->getCurrentDatabaseConnection();
    assert(conn);

    QFileInfo fpathInfo(fpath);

    QString rootPath = conn->getFileRoot();
    QDir rootDir(rootPath);

    if (!rootDir.exists()  &&  (!fpathInfo.isRelative()  ||  fpath.startsWith("../"))) {
        QMessageBox::critical(this, tr("Invalid File"),
                tr(	"The file path you specified for property '%1' is not relative or tried to reference the parent "
                    "directory, and the current file root directory is invalid. This file can't be saved!")
                    .arg(prop->getUserReadableName()));
        return QString();
    }

    QString relPath = rootDir.relativeFilePath(fpath);

    if (relPath.startsWith("../")  ||  QFileInfo(relPath).isAbsolute()) {
        if (fpathInfo.exists()  &&  fpathInfo.isFile()) {
            QMessageBox::StandardButton b = QMessageBox::question(this, tr("Copy File?"),
                    tr(	"The file you specified for property '%1' does not seem to be inside the current file root directory. "
                        "It must be copied there to be handled correctly by this program.\n\n"
                        "Do you want to copy the file to the file root directory?").arg(prop->getUserReadableName()),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (b == QMessageBox::Yes) {
                QFileInfo relPathInfo(relPath);
                QString suggestedPath(relPathInfo.fileName());

                unsigned int i = 2;
                while (QFileInfo(rootDir, suggestedPath).exists()) {
                    QString sfx = relPathInfo.completeSuffix();
                    suggestedPath = QString("%1_%2%3%4")
                            .arg(relPathInfo.baseName())
                            .arg(i)
                            .arg(sfx.isEmpty() ? "" : ".",
                                 sfx);
                    i++;
                }

                relPath = QString();
                while (relPath.isNull()) {
                    QString savePath = QFileDialog::getSaveFileName(this, tr("Select Destination File"),
                            QFileInfo(rootDir, suggestedPath).filePath());

                    if (savePath.isNull()) {
                        return QString();
                    }

                    relPath = rootDir.relativeFilePath(savePath);

                    if (relPath.startsWith("../")) {
                        QMessageBox::critical(this, tr("Invalid File"),
                                tr("The selected file does not seem to be inside the current file root directory '%1'!")
                                    .arg(rootPath));
                        relPath = QString();
                    }
                }

                if (!QFile::copy(fpath, QFileInfo(rootDir, relPath).filePath())) {
                    QMessageBox::critical(this, tr("Unable To Copy File"),
                            tr("Failed to copy file '%1' to '%2/%3'!").arg(fpath, rootPath, relPath));
                    return QString();
                }
            } else if (b == QMessageBox::No) {
            } else if (b == QMessageBox::Cancel) {
                return QString();
            }
        } else {
            QMessageBox::critical(this, tr("Invalid File"),
                    tr(	"The file path you specified for property '%1' does not seem to point inside the current file root "
                        "directory, nor does a regular file exist under that path. This file can't be saved!")
                        .arg(prop->getUserReadableName()));
            return QString();
        }
    }

    if (relPath.startsWith("../")) {
        QFileInfo info(relPath);

        relPath = info.fileName();
    }

    return relPath;
}


void PartDisplayWidget::focusValueWidget(AbstractPartProperty* aprop)
{
    PropertyWidgets pw = propWidgets[aprop];

    PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
    PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

    if (prop) {
        PartProperty::Type type = prop->getType();
        flags_t flags = prop->getFlags();

        QString tabWidgetObjName;

        if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
            tabWidgetObjName = QString("mvPropertyWidget_%1").arg(prop->getFieldName());
        } else {
            tabWidgetObjName = "generalWidget";
        }

        if (!tabWidgetObjName.isEmpty()  &&  tabber) {
            QWidget* tabWidget = tabber->findChild<QWidget*>(tabWidgetObjName);
            tabber->setCurrentWidget(tabWidget);
        }

        if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
            pw.mvWidget->focusValueWidget();
        } else if (type == PartProperty::Boolean) {
            pw.boolBox->setFocus();
        } else if (type == PartProperty::File) {
            pw.fileWidget->setValueFocus();
        } else if ( type == PartProperty::Date
                ||  type == PartProperty::Time
                ||  type == PartProperty::DateTime
        ) {
            pw.dateTimeEdit->setValueFocus();
        } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
            pw.enumBox->setFocus();
            pw.enumBox->lineEdit()->selectAll();
        } else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
            pw.textEdit->setFocus();
            pw.textEdit->selectAll();
        } else {
            pw.lineEdit->setFocus();
            pw.lineEdit->selectAll();
        }
    } else if (ltype) {
        if (pw.mvWidget) {
            assert(tabber);

            QWidget* tabWidget = tabber->findChild<QWidget*>(QString("mvPropertyWidgetLtype_%1").arg(ltype->getID()));
            tabber->setCurrentWidget(tabWidget);

            pw.mvWidget->focusValueWidget();
        }
    } else {
        assert(false);
    }
}


void PartDisplayWidget::focusAuto()
{
    QWidget* fw = focusWidget();

    if (fw) {
        AbstractPartProperty* aprop = getPropertyForWidget(fw);

        if (aprop) {
            focusValueWidget(aprop);
            return;
        }
    }

    if (tabber) {
        QString objName = tabber->currentWidget()->objectName();

        if (objName == "generalWidget") {
            if (!propWidgets.empty()) {
                focusValueWidget(*partCat->getPropertyBegin());
            }
        } else {
            for (AbstractPartProperty* aprop : partCat->getAbstractProperties(System::getInstance()->getPartLinkTypes())) {
                PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
                PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

                if (prop) {
                    if (objName == QString("mvPropertyWidget_%1").arg(prop->getFieldName())) {
                        propWidgets[prop].mvWidget->focusValueWidget();
                        break;
                    }
                } else if (ltype) {
                    if (objName == QString("mvPropertyWidgetLtype_%1").arg(ltype->getID())) {
                        if (propWidgets[ltype].mvWidget) {
                            propWidgets[ltype].mvWidget->focusValueWidget();
                            break;
                        }
                    }
                } else {
                    assert(false);
                }
            }
        }
    } else {
        if (!propWidgets.empty()) {
            focusValueWidget(propWidgets.begin().key());
        }
    }
}


void PartDisplayWidget::containerLinkActivated(const QString& link)
{
    PartContainer cont = PartContainerFactory::getInstance().findContainerByID(static_cast<dbid_t>(link.toLongLong()));
    System::getInstance()->jumpToContainer(cont);
}


void PartDisplayWidget::keyPressEvent(QKeyEvent* evt)
{
    if (evt->key() == Qt::Key_Escape) {
        evt->accept();
        emit defocusRequested();
        return;
    } else if (evt->key() == Qt::Key_PageDown) {
        emit gotoNextPartRequested();
    } else if (evt->key() == Qt::Key_PageUp) {
        emit gotoPreviousPartRequested();
    }

    QWidget::keyPressEvent(evt);
}


bool PartDisplayWidget::eventFilter(QObject* obj, QEvent* evt)
{
    EditStack* editStack = System::getInstance()->getEditStack();

    if (evt->type() == QEvent::KeyPress) {
        QKeyEvent* kevt = (QKeyEvent*) evt;

        if (kevt->key() == Qt::Key_PageDown) {
            emit gotoNextPartRequested();
            return true;
        } else if (kevt->key() == Qt::Key_PageUp) {
            emit gotoPreviousPartRequested();
            return true;
        } else if (kevt->matches(QKeySequence::Undo)) {
            if (editStack->canUndo()) {
                editStack->undo();
            }
            return true;
        } else if (kevt->matches(QKeySequence::Redo)) {
            if (editStack->canRedo()) {
                editStack->redo();
            }
            return true;
        }
    }

    return QWidget::eventFilter(obj, evt);
}


AbstractPartProperty* PartDisplayWidget::getPropertyForWidget(QWidget* widget)
{
    for (AbstractPartProperty* aprop : partCat->getAbstractProperties(System::getInstance()->getPartLinkTypes())) {
        PartProperty* prop = dynamic_cast<PartProperty*>(aprop);
        PartLinkType* ltype = dynamic_cast<PartLinkType*>(aprop);

        PropertyWidgets pw = propWidgets[aprop];

        if (prop) {
            PartProperty::Type type = prop->getType();
            flags_t flags = prop->getFlags();

            if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
                if (widget == pw.mvWidget  ||  pw.mvWidget->isAncestorOf(widget)) {
                    return prop;
                }
            } else {
                if (type == PartProperty::File) {
                    if (widget == pw.fileWidget  ||  pw.fileWidget->isAncestorOf(widget)) {
                        return prop;
                    }
                } else if (type == PartProperty::Boolean) {
                    if (widget == pw.boolBox) {
                        return prop;
                    }
                } else if ( type == PartProperty::Date
                        ||  type == PartProperty::Time
                        ||  type == PartProperty::DateTime
                ) {
                    if (widget == pw.dateTimeEdit  ||  pw.dateTimeEdit->isAncestorOf(widget)) {
                        return prop;
                    }
                } else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
                    if (widget == pw.enumBox) {
                        return prop;
                    }
                } else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
                    if (widget == pw.textEdit) {
                        return prop;
                    }
                } else {
                    if (widget == pw.lineEdit) {
                        return prop;
                    }
                }
            }
        } else if (ltype) {
            if (pw.mvWidget) {
                if (widget == pw.mvWidget  ||  pw.mvWidget->isAncestorOf(widget)) {
                    return prop;
                }
            }
        } else {
            assert(false);
        }
    }

    return nullptr;
}


}
