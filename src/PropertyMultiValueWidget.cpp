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

#include "PropertyMultiValueWidget.h"
#include <nxcommon/exception/InvalidValueException.h>
#include <QtGui/QSplitter>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFormLayout>



PropertyMultiValueWidget::PropertyMultiValueWidget(PartProperty* prop, QWidget* parent, QObject* keyEventFilter)
		: QWidget(parent), cat(prop->getPartCategory()), prop(prop), state(Enabled), flags(0)
{
	PartProperty::Type type = prop->getType();
	flags_t flags = prop->getFlags();


	QVBoxLayout* topLayout = new QVBoxLayout(this);
	setLayout(topLayout);

	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	topLayout->addWidget(splitter);

	QWidget* listContWidget = new QWidget(splitter);
	splitter->addWidget(listContWidget);

	QVBoxLayout* listContLayout = new QVBoxLayout(listContWidget);
	listContWidget->setLayout(listContLayout);

	listWidget = new QListWidget(listContWidget);
	listWidget->installEventFilter(keyEventFilter);
	connect(listWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
			this, SLOT(currentItemChanged(QListWidgetItem*, QListWidgetItem*)));
	listContLayout->addWidget(listWidget);


	QWidget* buttonWidget = new QWidget(listContWidget);
	listContLayout->addWidget(buttonWidget);

	QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
	buttonWidget->setLayout(buttonLayout);

	buttonLayout->addStretch(1);

	listAddButton = new QPushButton("", buttonWidget);
	listAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
	connect(listAddButton, SIGNAL(clicked()), this, SLOT(addRequested()));
	buttonLayout->addWidget(listAddButton);

	listRemoveButton = new QPushButton("", buttonWidget);
	listRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));
	connect(listRemoveButton, SIGNAL(clicked()), this, SLOT(removeRequested()));
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

	if (type != PartProperty::PartLink) {
		if (type == PartProperty::Boolean) {
			boolBox = new QCheckBox(prop->getUserReadableName(), detailFormWidget);
			boolBox->installEventFilter(keyEventFilter);
			connect(boolBox, SIGNAL(toggled(bool)), this, SLOT(boolFieldToggled(bool)));
			detailFormLayout->addRow(boolBox);
		} else if (type == PartProperty::File) {
			fileWidget = new PropertyFileWidget(prop, detailFormWidget, keyEventFilter);
			connect(fileWidget, SIGNAL(changedByUser()), this, SLOT(fileChanged()));
			detailFormLayout->addRow(fileWidget);
		} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
			enumBox = new PropertyComboBox(prop, detailFormWidget);
			enumBox->installEventFilter(keyEventFilter);
			connect(enumBox, SIGNAL(changedByUser(const QString&)), this, SLOT(valueFieldEdited(const QString&)));
			detailFormLayout->addRow(tr("Value"), enumBox);
		} else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
			textEdit = new PropertyTextEdit(prop, detailFormWidget);
			textEdit->installEventFilter(keyEventFilter);
			connect(textEdit, SIGNAL(editedByUser(const QString&)), this, SLOT(textEditEdited()));
			detailFormLayout->addRow(tr("Value"), textEdit);
		} else {
			lineEdit = new PropertyLineEdit(prop, detailFormWidget);
			lineEdit->installEventFilter(keyEventFilter);
			connect(lineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(valueFieldEdited(const QString&)));
			detailFormLayout->addRow(tr("Value"), lineEdit);
		}

		detailLayout->addStretch(1);
	} else {
		linkWidget = new PropertyLinkEditWidget(prop, true, detailFormWidget, keyEventFilter);
		connect(linkWidget, SIGNAL(changedByUser()), this, SLOT(linkWidgetChangedByUser()));

		detailFormLayout->addRow(tr("Link"), linkWidget);

		detailLayout->addStretch(1);
	}

	applyState();
}


void PropertyMultiValueWidget::display(unsigned int id, QList<QString> rawVals)
{
	currentID = id;

	listWidget->clear();

	for (QList<QString>::iterator it = rawVals.begin() ; it != rawVals.end() ; it++) {
		QString val = *it;
		QString desc = prop->formatValueForEditing(QList<QString>() << val);

		QListWidgetItem* item = new QListWidgetItem(desc, listWidget);
		item->setData(Qt::UserRole, val);
	}

	if (listWidget->count() != 0)
		listWidget->setCurrentRow(0);
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


QList<QString> PropertyMultiValueWidget::getValues(bool initialValueOnError) const
{
	QList<QString> rawVals;

	for (int i = 0 ; i < listWidget->count() ; i++) {
		QVariant vdata = listWidget->item(i)->data(Qt::UserRole);

		if (vdata.isNull()) {
			if (initialValueOnError) {
				rawVals << prop->getInitialValue();
			} else {
				throw InvalidValueException(tr("Value #%1 is invalid!").arg(i+1).toUtf8().constData(),
						__FILE__, __LINE__);
			}
		} else {
			rawVals << vdata.toString();
		}
	}

	return rawVals;
}


void PropertyMultiValueWidget::applyState()
{
	PartProperty::Type type = prop->getType();
	flags_t flags = prop->getFlags();

	DisplayWidgetState state = this->state;

	if ((this->flags & ChoosePart)  !=  0) {
		state = ReadOnly;

		listAddButton->hide();
		listRemoveButton->hide();
	} else {
		listAddButton->show();
		listRemoveButton->show();
	}

	bool hasCurrentItem = (listWidget->currentItem()  !=  NULL);

	DisplayWidgetState stateBeforeHasCurrentItem = state;

	if (!hasCurrentItem) {
		state = Disabled;
	}

	if (type == PartProperty::PartLink) {
		linkWidget->setState(state);
		linkWidget->setFlags(this->flags);
	} else if (type == PartProperty::File) {
		fileWidget->setState(state);
		fileWidget->setFlags(this->flags);
	} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
		enumBox->setState(state);
		enumBox->setFlags(this->flags);
	}

	if (state == Enabled) {
		listWidget->setEnabled(true);
		listAddButton->setEnabled(true);
		listRemoveButton->setEnabled(true);

		if (type == PartProperty::Boolean) {
			boolBox->setEnabled(true);
		} else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
			textEdit->setEnabled(true);
			textEdit->setReadOnly(false);
		} else if (type != PartProperty::PartLink  &&  type != PartProperty::File
				&&  (flags & PartProperty::DisplayDynamicEnum)  ==  0)
		{
			lineEdit->setEnabled(true);
			lineEdit->setReadOnly(false);
		}
	} else if (state == Disabled) {
		listWidget->setEnabled(false);
		listAddButton->setEnabled(false);
		listRemoveButton->setEnabled(false);

		if (type == PartProperty::Boolean) {
			boolBox->setEnabled(false);
		} else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
			textEdit->setEnabled(false);
		} else if (type != PartProperty::PartLink  &&  type != PartProperty::File
				&&  (flags & PartProperty::DisplayDynamicEnum)  ==  0)
		{
			lineEdit->setEnabled(false);
		}
	} else if (state == ReadOnly) {
		listWidget->setEnabled(true);
		listAddButton->setEnabled(false);
		listRemoveButton->setEnabled(false);

		if (type == PartProperty::Boolean) {
			boolBox->setEnabled(false);
		} else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
			textEdit->setEnabled(true);
			textEdit->setReadOnly(true);
		} else if (type != PartProperty::PartLink  &&  type != PartProperty::File
				&&  (flags & PartProperty::DisplayDynamicEnum)  ==  0)
		{
			lineEdit->setEnabled(true);
			lineEdit->setReadOnly(true);
		}
	}

	if (!hasCurrentItem  &&  stateBeforeHasCurrentItem  ==  Enabled) {
		listWidget->setEnabled(true);
		listAddButton->setEnabled(true);
	}
}


void PropertyMultiValueWidget::currentItemChanged(QListWidgetItem* newItem, QListWidgetItem* oldItem)
{
	PartProperty::Type type = prop->getType();
	flags_t flags = prop->getFlags();

	if (!newItem) {
		if (type == PartProperty::PartLink) {
			linkWidget->displayLink(0);
		} else if (type == PartProperty::Boolean) {
			boolBox->blockSignals(true);
			boolBox->setChecked(false);
			boolBox->blockSignals(false);
		} else if (type == PartProperty::File) {
			fileWidget->displayFile(INVALID_PART_ID, QString());
		} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
			enumBox->displayRecord(INVALID_PART_ID, QString());
		} else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
			textEdit->setText("");
		} else {
			lineEdit->clear();
		}

		applyState();

		return;
	}

	if (type == PartProperty::PartLink) {
		QVariant vdata = newItem->data(Qt::UserRole);
		unsigned int linkId;

		if (vdata.isNull()) {
			linkId = INVALID_PART_ID;
		} else {
			linkId = vdata.toString().toUInt();
		}

		linkWidget->displayLink(linkId);
	} else {
		QVariant vdata = newItem->data(Qt::UserRole);
		QString rawVal;

		if (vdata.isNull()) {
			rawVal = QString();
		} else {
			rawVal = vdata.toString();
		}

		if (type == PartProperty::Boolean) {
			bool val = rawVal.toLongLong() != 0;
			boolBox->blockSignals(true);
			boolBox->setChecked(val);
			boolBox->blockSignals(false);
		} else if (type == PartProperty::File) {
			fileWidget->displayFile(currentID, rawVal);
		} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
			enumBox->displayRecord(currentID, rawVal);
		} else if ((flags & PartProperty::DisplayTextArea)  !=  0) {
			QString descVal = prop->formatValueForEditing(QList<QString>() << rawVal);
			textEdit->setText(descVal);
		} else {
			QString descVal = prop->formatValueForEditing(QList<QString>() << rawVal);
			lineEdit->setText(descVal);
		}
	}

	applyState();
}


void PropertyMultiValueWidget::addRequested()
{
	PartProperty::Type type = prop->getType();
	flags_t flags = prop->getFlags();

	if (type == PartProperty::PartLink) {
		QString val = prop->getInitialValue();
		unsigned int id = val.toUInt();

		PartCategory* linkCat = prop->getPartLinkCategory();

		linkWidget->displayLink(id);
		linkWidget->setFocus();

		QString displayStr = prop->formatValueForDisplay(QList<QString>() << val);

		QListWidgetItem* item = new QListWidgetItem(displayStr, listWidget);
		item->setData(Qt::UserRole, QVariant(QString("%1").arg(id)));
		listWidget->setCurrentItem(item);
	} else {
		QString rawVal = prop->getInitialValue();
		QListWidgetItem* item = new QListWidgetItem(prop->formatValueForDisplay(QList<QString>() << rawVal),
				listWidget);
		item->setData(Qt::UserRole, QVariant(rawVal));
		listWidget->setCurrentItem(item);
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
	QString rawVal = val ? "1" : "0";

	QListWidgetItem* item = listWidget->currentItem();
	item->setText(prop->formatValueForDisplay(QList<QString>() << rawVal));
	item->setData(Qt::UserRole, QVariant(rawVal));

	emit changedByUser();
}


void PropertyMultiValueWidget::valueFieldEdited(const QString&)
{
	// This is also called for the QPlainTextEdit, with an invalid parameter, and for
	// DisplayDynamicEnum types, with a valid parameter!

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
		rawVal = prop->parseUserInputValue(text)[0];
	} catch (InvalidValueException& ex) {
		//rawVal = prop->getInitialValue();
		rawVal = QVariant();
	}

	if (rawVal.isNull()) {
		item->setText(prop->formatValueForDisplay(QList<QString>() << QString()));
	} else {
		item->setText(prop->formatValueForDisplay(QList<QString>() << rawVal.toString()));
	}

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

	QString rawVal;

	try {
		rawVal = linkWidget->getValue();
	} catch (InvalidValueException& ex) {
		rawVal = QString();
	}

	QString displayStr = prop->formatValueForDisplay(QList<QString>() << rawVal);
	item->setText(displayStr);

	item->setData(Qt::UserRole, rawVal);

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


void PropertyMultiValueWidget::focusValueWidget()
{
	PartProperty::Type type = prop->getType();
	flags_t flags = prop->getFlags();

	if (listWidget->currentRow() == -1  &&  listWidget->count() > 0) {
		listWidget->setCurrentRow(0);
	}

	if (listWidget->currentRow() == -1) {
		listWidget->setFocus();
	} else {
		if (type == PartProperty::Boolean) {
			boolBox->setFocus();
		} else if (type == PartProperty::PartLink) {
			linkWidget->setValueFocus();
		} else if (type == PartProperty::File) {
			fileWidget->setValueFocus();
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
	}
}


