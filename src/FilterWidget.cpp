/*
	Copyright 2010-2014 David "Alemarius Nexus" Lerch

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
#include "elutil.h"
#include "System.h"
#include "NoDatabaseConnectionException.h"
#include <QtGui/QComboBox>
#include <QtGui/QScrollArea>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QSplitter>
#include <QtGui/QTextEdit>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QScrollBar>
#include <QtCore/QTimer>
#include <cstdio>
#include <nxcommon/sql/sql.h>



FilterWidget::FilterWidget(QWidget* parent, PartCategory* partCat)
		: QWidget(parent), partCat(partCat)
{
	System* sys = System::getInstance();

	SQLDatabase sql = sys->getCurrentSQLDatabase();

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
	listLayout->setSizeConstraint(QLayout::SetMinimumSize);
	listWidget->setLayout(listLayout);
	listScroller->setWidget(listWidget);

	unsigned int i = 0;
	for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++, i++) {
		PartProperty* prop = *it;

		if ((prop->getFlags() & PartProperty::DisplayNoSelectionList)  !=  0) {
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
		connect(clearSelButton, SIGNAL(clicked()), list, SLOT(clearSelection()));
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
	connect(ftField, SIGNAL(returnPressed()), this, SLOT(appliedSlot()));
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
	connect(sqlCombFuncBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sqlCombFuncBoxChanged(int)));
	sqlLayout->addWidget(sqlCombFuncBox);

	sqlEditField = new QLineEdit(sqlWidget);
	connect(sqlEditField, SIGNAL(returnPressed()), this, SLOT(appliedSlot()));
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


	topLayout->addSpacing(20);


	QWidget* buttonWidget = new QWidget(this);
	topLayout->addWidget(buttonWidget);

	QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
	int blcmL, blcmT, blcmR, blcmB;
	buttonLayout->getContentsMargins(&blcmL, &blcmT, &blcmR, &blcmB);
	buttonLayout->setContentsMargins(blcmL, 0, blcmR, blcmB);
	buttonWidget->setLayout(buttonLayout);

	resetButton = new QPushButton(tr("Reset All"), this);
	resetButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
	connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));

	applyButton = new QPushButton(tr("Apply Filters"), this);
	applyButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
	connect(applyButton, SIGNAL(clicked()), this, SLOT(appliedSlot()));

	buttonLayout->addWidget(applyButton);
	buttonLayout->addWidget(resetButton);
	buttonLayout->addStretch(1);


	topLayout->setStretch(0, 1);
	topLayout->setStretch(1, 0);


	connect(sqlInsColBox, SIGNAL(activated(int)), this, SLOT(sqlInsertColumnBoxActivated(int)));
	connect(ftInsFieldBox, SIGNAL(activated(int)), this, SLOT(ftInsertFieldBoxActivated(int)));
	connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(populateDatabaseDependentUI()));

	EditStack* editStack = sys->getEditStack();

	connect(partCat, SIGNAL(recordCreated(unsigned int, PartCategory::DataMap)), this, SLOT(partsChanged()));
	connect(partCat, SIGNAL(recordEdited(unsigned int, PartCategory::DataMap)), this, SLOT(partsChanged()));
	connect(partCat, SIGNAL(recordsRemoved(QList<unsigned int>)), this, SLOT(partsChanged()));

	connect(editStack, SIGNAL(undone(EditCommand*)), this, SLOT(partsChanged()));
	connect(editStack, SIGNAL(redone(EditCommand*)), this, SLOT(partsChanged()));

	populateDatabaseDependentUI();
}


void FilterWidget::readFilter(Filter* filter)
{
	System* sys = System::getInstance();

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	if (!sql.isValid()) {
		throw NoDatabaseConnectionException("Database connection is needed for FilterWidget::readFilter()",
				__FILE__, __LINE__);
	}

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
			QString sqlVal = item->data(Qt::UserRole).toString();

			if (sit != sel.begin()) {
				listWhereCode += " OR ";
			}

			if ((prop->getFlags() & PartProperty::MultiValue)  !=  0) {
				listWhereCode += QString("%1.%2='%3'")
						.arg(prop->getMultiValueForeignTableName())
						.arg(prop->getFieldName())
						.arg(sql.escapeString(sqlVal));
			} else {
				listWhereCode += QString("%1.%2='%3'")
						.arg(partCat->getTableName())
						.arg(prop->getFieldName())
						.arg(sql.escapeString(sqlVal));
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

	filter->setSQLWhereCode(whereCode);

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


void FilterWidget::populateDatabaseDependentUI(QMap<PartProperty*, QSet<QString> > selectedData)
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection()) {
		invalidate();
		return;
	}

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	try {
		for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
			PartProperty* prop = *it;

			flags_t flags = prop->getFlags();

			if ((flags & PartProperty::DisplayNoSelectionList)  !=  0) {
				continue;
			}

			QListWidget* list = propLists[prop];

			QSet<QString> selRawVals = selectedData[prop];

			QString query;

			if ((flags & PartProperty::MultiValue)  !=  0) {
				query = QString("SELECT DISTINCT %1 FROM %2 %3")
						.arg(prop->getFieldName())
						.arg(prop->getMultiValueForeignTableName())
						.arg(prop->getSQLNaturalOrderCode());
			} else {
				query = QString("SELECT DISTINCT %1 FROM %2 %3")
						.arg(prop->getFieldName())
						.arg(partCat->getTableName())
						.arg(prop->getSQLNaturalOrderCode());
			}

			SQLResult res = sql.sendQuery(query);


			if (prop->getType() == PartProperty::PartLink) {
				QList<unsigned int> linkIds;

				while (res.nextRecord()) {
					unsigned int linkId = res.getUInt32(0);
					linkIds << linkId;
				}

				PartCategory* linkCat = prop->getPartLinkCategory();

				QMap<unsigned int, QString> descs = linkCat->getDescriptions(linkIds);

				for (QMap<unsigned int, QString>::iterator iit = descs.begin() ; iit != descs.end() ; iit++) {
					unsigned int linkId = iit.key();
					QString desc = iit.value();

					QListWidgetItem* item = new QListWidgetItem(desc, list);
					item->setData(Qt::UserRole, linkId);

					if (selRawVals.contains(QString("%1").arg(linkId))) {
						item->setSelected(true);
					}
				}
			} else {
				while (res.nextRecord()) {
					if (!res.isNull(0)) {
						QString val = res.getString(0);
						QListWidgetItem* item = new QListWidgetItem(prop->formatValueForDisplay(QList<QString>() << val), list);
						item->setData(Qt::UserRole, val);

						if (selRawVals.contains(val)) {
							item->setSelected(true);
						}
					} else {
						QListWidgetItem* item = new QListWidgetItem(prop->formatValueForDisplay(QList<QString>()), list);
						item->setData(Qt::UserRole, QVariant());

						QFont font = item->font();
						font.setItalic(true);
						item->setFont(font);

						if (selRawVals.contains(QString())) {
							item->setSelected(true);
						}
					}
				}

				list->setMinimumWidth(list->sizeHintForColumn(0) + 15);
			}
		}

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

		if ((prop->getFlags() & PartProperty::DisplayNoSelectionList)  !=  0) {
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

		if ((prop->getFlags() & PartProperty::DisplayNoSelectionList)  !=  0) {
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
	QMap<PartProperty*, QSet<QString> > selectedData;

	for (QMap<PartProperty*, QListWidget*>::iterator it = propLists.begin() ; it != propLists.end() ; it++) {
		PartProperty* prop = it.key();
		QListWidget* list = it.value();

		QList<QListWidgetItem*> sel = list->selectedItems();
		QSet<QString> selStrs;

		for (QList<QListWidgetItem*>::iterator sit = sel.begin() ; sit != sel.end() ; sit++) {
			QListWidgetItem* item = *sit;
			QVariant rawVal = item->data(Qt::UserRole);

			if (prop->getType() == PartProperty::PartLink) {
				selStrs << QString("%1").arg(rawVal.toUInt());
			} else {
				if (rawVal.isValid()) {
					selStrs << rawVal.toString();
				} else {
					selStrs << QString();
				}
			}
		}

		list->clear();

		selectedData[prop] = selStrs;
	}

	populateDatabaseDependentUI(selectedData);
}
