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

#include "FilterWidget.h"

#include <cstdio>
#include <memory>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextEdit>
#include <QTimer>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../exception/NoDatabaseConnectionException.h"
#include "../../exception/SQLException.h"
#include "../../model/part/PartFactory.h"
#include "../../util/elutil.h"
#include "../../System.h"

namespace electronicsdb
{


FilterWidget::FilterWidget(QWidget* parent, PartCategory* partCat)
        : QWidget(parent), partCat(partCat)
{
    System* sys = System::getInstance();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    setLayout(topLayout);


    QScrollArea* listScroller = new QScrollArea(this);
    listScroller->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listScroller->setWidgetResizable(true);
    listScroller->setFrameShadow(QFrame::Plain);
    topLayout->addWidget(listScroller);

    QWidget* listWidget = new QWidget(listScroller);
    QGridLayout* listLayout = new QGridLayout(listWidget);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSizeConstraint(QLayout::SetMinimumSize);
    listWidget->setLayout(listLayout);
    listScroller->setWidget(listWidget);

    unsigned int i = 0;
    for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++, i++) {
        PartProperty* prop = *it;

        if ((prop->getFlags() & PartProperty::DisplaySelectionList)  ==  0) {
            continue;
        }

        QLabel* listLabel = new QLabel(prop->getUserReadableName(), listWidget);
        listLabel->setAlignment(Qt::AlignHCenter);
        listLayout->addWidget(listLabel, 0, i);

        QListWidget* list = new QListWidget(listWidget);
        list->setSelectionMode(QListWidget::ExtendedSelection);
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        listLayout->addWidget(list, 1, i);

        QPushButton* clearSelButton = new QPushButton("Reset", listWidget);
        connect(clearSelButton, &QPushButton::clicked, list, &QListWidget::clearSelection);
        listLayout->addWidget(clearSelButton, 2, i, Qt::AlignHCenter);

        propLists.insert(prop, list);
    }

    listScroller->setMinimumHeight(listScroller->widget()->minimumSizeHint().height()
            + listScroller->horizontalScrollBar()->sizeHint().height());


    QWidget* sqlFtWidget = new QWidget(this);
    topLayout->addWidget(sqlFtWidget);

    QHBoxLayout* sqlFtLayout = new QHBoxLayout(sqlFtWidget);
    sqlFtLayout->setContentsMargins(0, 0, 0, 0);
    sqlFtWidget->setLayout(sqlFtLayout);


    QGroupBox* ftWidget = new QGroupBox(tr("Full Text Filter"), sqlFtWidget);
    sqlFtLayout->addWidget(ftWidget);

    QHBoxLayout* ftLayout = new QHBoxLayout(ftWidget);
    ftWidget->setLayout(ftLayout);

    QLabel* ftLabel = new QLabel(tr("Query:"), ftWidget);
    ftLayout->addWidget(ftLabel);

    ftLayout->addSpacing(20);

    ftField = new QLineEdit(ftWidget);
    connect(ftField, &QLineEdit::returnPressed, this, &FilterWidget::appliedSlot);
    ftLayout->addWidget(ftField);

    ftInsFieldBox = new QComboBox(ftWidget);
    ftInsFieldBox->addItem(tr("- Insert Prefix -"), QVariant());
    ftLayout->addWidget(ftInsFieldBox);

    for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
        PartProperty* prop = *it;

        if ((prop->getFlags() & PartProperty::FullTextIndex) == 0)
            continue;

        ftInsFieldBox->addItem(prop->getUserReadableName(), prop->getFullTextSearchUserPrefix());
    }


    QGroupBox* sqlWidget = new QGroupBox(tr("Additional SQL Filter"), sqlFtWidget);
    sqlFtLayout->addWidget(sqlWidget);

    QHBoxLayout* sqlLayout = new QHBoxLayout(sqlWidget);
    sqlWidget->setLayout(sqlLayout);

    sqlCombFuncBox = new QComboBox(sqlWidget);
    sqlCombFuncBox->addItem(tr("(Disabled)"));
    sqlCombFuncBox->addItem(tr("AND"));
    sqlCombFuncBox->addItem(tr("OR"));
    connect(sqlCombFuncBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &FilterWidget::sqlCombFuncBoxChanged);
    sqlLayout->addWidget(sqlCombFuncBox);

    sqlEditField = new QLineEdit(sqlWidget);
    connect(sqlEditField, &QLineEdit::returnPressed, this, &FilterWidget::appliedSlot);
    sqlLayout->addWidget(sqlEditField);

    sqlInsColBox = new QComboBox(sqlWidget);
    sqlInsColBox->addItem(tr("- Insert Column -"), QVariant());
    sqlLayout->addWidget(sqlInsColBox);

    for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
        PartProperty* prop = *it;
        sqlInsColBox->addItem(prop->getUserReadableName(), prop->getFieldName());
    }

    sqlCombFuncBox->setCurrentIndex(0);
    sqlCombFuncBoxChanged(0);


    topLayout->addSpacing(5);


    QWidget* buttonWidget = new QWidget(this);
    topLayout->addWidget(buttonWidget);

    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    int blcmL, blcmT, blcmR, blcmB;
    buttonLayout->getContentsMargins(&blcmL, &blcmT, &blcmR, &blcmB);
    buttonLayout->setContentsMargins(0, 0, 0, blcmB);
    buttonWidget->setLayout(buttonLayout);

    resetButton = new QPushButton(tr("Reset All"), this);
    resetButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    connect(resetButton, &QPushButton::clicked, this, &FilterWidget::reset);

    applyButton = new QPushButton(tr("Apply Filters"), this);
    applyButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    connect(applyButton, &QPushButton::clicked, this, &FilterWidget::appliedSlot);

    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch(1);


    topLayout->setStretch(0, 1);
    topLayout->setStretch(1, 0);


    connect(sqlInsColBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &FilterWidget::sqlInsertColumnBoxActivated);
    connect(ftInsFieldBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &FilterWidget::ftInsertFieldBoxActivated);
    connect(sys, SIGNAL(appModelReset()), this, SLOT(populateAppModelUI()));

    EditStack* editStack = sys->getEditStack();

    connect(&PartFactory::getInstance(), &PartFactory::partsChanged, this, &FilterWidget::partsChanged);

    populateAppModelUI();
}


void FilterWidget::readFilter(Filter* filter)
{
    System* sys = System::getInstance();

    QSqlDatabase db = QSqlDatabase::database();

    if (!db.isValid()) {
        throw NoDatabaseConnectionException("Database connection is needed for FilterWidget::readFilter()",
                __FILE__, __LINE__);
    }

    std::unique_ptr<SQLDatabaseWrapper> dbw(SQLDatabaseWrapperFactory::getInstance().create(db));

    QList<QVariant> whereBindParamVals;

    QString listWhereCode = "";

    bool firstListQueryPart = true;
    for (QMap<PartProperty*, QListWidget*>::iterator it = propLists.begin() ; it != propLists.end() ; it++) {
        PartProperty* prop = it.key();
        QListWidget* list = it.value();

        QList<QListWidgetItem*> sel = list->selectedItems();

        if (!sel.empty()) {
            if (!firstListQueryPart) {
                listWhereCode += " AND ";
            } else {
                firstListQueryPart = false;
            }

            listWhereCode += "(";
        }

        for (QList<QListWidgetItem*>::iterator sit = sel.begin() ; sit != sel.end() ; sit++) {
            QListWidgetItem* item = *sit;
            QVariant sqlVal = item->data(Qt::UserRole);

            if (sit != sel.begin()) {
                listWhereCode += " OR ";
            }

            if ((prop->getFlags() & PartProperty::MultiValue)  !=  0) {
                whereBindParamVals << sqlVal;
                listWhereCode += QString("%1.%2=?")
                        .arg(dbw->escapeIdentifier(prop->getMultiValueForeignTableName()),
                             dbw->escapeIdentifier(prop->getFieldName()));
            } else {
                whereBindParamVals << sqlVal;
                listWhereCode += QString("%1.%2=?")
                        .arg(dbw->escapeIdentifier(partCat->getTableName()),
                             dbw->escapeIdentifier(prop->getFieldName()));
            }
        }

        if (!sel.empty())
            listWhereCode += ")";
    }


    QString addSqlCode = sqlEditField->text();

    if (sqlCombFuncBox->currentIndex() == 0) {
        addSqlCode = QString("");
    }

    QString whereCode("");

    if (!listWhereCode.isEmpty()  ||  !addSqlCode.isEmpty()) {
        whereCode = "WHERE ";

        if (!listWhereCode.isEmpty()) {
            whereCode += listWhereCode;

            if (!addSqlCode.isEmpty()) {
                if (sqlCombFuncBox->currentIndex() == 1) {
                    whereCode += " AND ";
                } else {
                    whereCode += " OR ";
                }
            }
        }

        if (!addSqlCode.isEmpty()) {
            whereCode += QString("(%1)").arg(addSqlCode);
        }
    }

    whereCode = whereCode.trimmed();

    filter->setSQLWhereCode(whereCode, whereBindParamVals);

    filter->setFullTextQuery(ftField->text());
}


void FilterWidget::sqlInsertColumnBoxActivated(int idx)
{
    if (idx == -1)
        return;

    QVariant fieldData = sqlInsColBox->itemData(idx, Qt::UserRole);

    if (!fieldData.isValid())
        return;

    QString fieldName = fieldData.toString();
    sqlEditField->insert(fieldName);

    sqlInsColBox->setCurrentIndex(0);

    sqlEditField->setFocus();
}


void FilterWidget::ftInsertFieldBoxActivated(int idx)
{
    if (idx == -1)
        return;

    QVariant fieldData = ftInsFieldBox->itemData(idx, Qt::UserRole);

    if (!fieldData.isValid())
        return;

    QString fieldName = fieldData.toString();
    ftField->insert(fieldName + ":");

    ftInsFieldBox->setCurrentIndex(0);

    ftField->setFocus();
}


void FilterWidget::sqlCombFuncBoxChanged(int idx)
{
    if (idx == -1)
        return;

    bool enabled = idx != 0;

    sqlEditField->setEnabled(enabled);
    sqlInsColBox->setEnabled(enabled);
}


void FilterWidget::populateAppModelUI(const QMap<PartProperty*, QList<QVariant>>& selectedData)
{
    System* sys = System::getInstance();

    if (!sys->hasValidDatabaseConnection()) {
        invalidate();
        return;
    }

    try {
        // Property lists
        for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
            PartProperty* prop = *it;

            flags_t flags = prop->getFlags();

            if ((flags & PartProperty::DisplaySelectionList)  ==  0) {
                continue;
            }

            QListWidget* list = propLists[prop];

            QList<QVariant> propValues = prop->getDistinctValues();

            QList<QVariant> selRawVals = selectedData[prop];

            for (const QVariant& val : propValues) {
                if (!val.isNull()) {
                    QListWidgetItem* item = new QListWidgetItem(prop->formatSingleValueForDisplay(val), list);
                    item->setData(Qt::UserRole, val);

                    if (selRawVals.contains(val)) {
                        item->setSelected(true);
                    }
                } else {
                    QListWidgetItem* item = new QListWidgetItem(prop->formatSingleValueForDisplay(QVariant()), list);
                    item->setData(Qt::UserRole, QVariant());

                    QFont font = item->font();
                    font.setItalic(true);
                    item->setFont(font);

                    if (selRawVals.contains(QString())) {
                        item->setSelected(true);
                    }
                }

                list->setMinimumWidth(list->sizeHintForColumn(0) + 15);
            }
        }

        // TODO: Maybe add list widgets for PartLinkTypes

        applyButton->setEnabled(true);
        resetButton->setEnabled(true);
    } catch (SQLException& ex) {
        QMessageBox::critical(this, tr("SQL Error"),
                tr("An SQL error was caught while building the filter widget:\n\n%1").arg(ex.what()));
        invalidate();
        sys->emergencyDisconnect();
    }
}


void FilterWidget::invalidate()
{
    for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
        PartProperty* prop = *it;

        if ((prop->getFlags() & PartProperty::DisplaySelectionList)  ==  0) {
            continue;
        }

        QListWidget* list = propLists[prop];
        list->clear();
    }

    applyButton->setEnabled(false);
    resetButton->setEnabled(false);
}


void FilterWidget::reset()
{
    for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
        PartProperty* prop = *it;

        if ((prop->getFlags() & PartProperty::DisplaySelectionList)  ==  0) {
            continue;
        }

        QListWidget* list = propLists[prop];
        list->clearSelection();
    }

    sqlEditField->clear();
    ftField->clear();

    emit applied();
}


void FilterWidget::appliedSlot()
{
    emit applied();
}


void FilterWidget::partsChanged()
{
    QMap<PartProperty*, QList<QVariant>> selectedData;

    for (QMap<PartProperty*, QListWidget*>::iterator it = propLists.begin() ; it != propLists.end() ; it++) {
        PartProperty* prop = it.key();
        QListWidget* list = it.value();

        QList<QVariant> propSelData;

        for (QListWidgetItem* item : list->selectedItems()) {
            QVariant rawVal = item->data(Qt::UserRole);
            propSelData << item->data(Qt::UserRole);
        }

        list->clear();

        selectedData[prop] = propSelData;
    }

    populateAppModelUI(selectedData);
}


}
