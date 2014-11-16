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

#include "PropertyLinkEditWidget.h"
#include "PlainLineEdit.h"
#include <nxcommon/exception/InvalidValueException.h>
#include "ChoosePartDialog.h"
#include "System.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>



PropertyLinkEditWidget::PropertyLinkEditWidget(PartProperty* prop, bool compactMode, QWidget* parent, QObject* keyEventFilter)
		: QWidget(parent), cat(prop->getPartCategory()), prop(prop), state(Enabled), flags(0)
{
	QVBoxLayout* topLayout = new QVBoxLayout(this);
	topLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(topLayout);

	QWidget* valueWidget = new QWidget(this);
	QVBoxLayout* valueLayout = new QVBoxLayout(valueWidget);
	valueLayout->setContentsMargins(0, 0, 0, 0);
	valueWidget->setLayout(valueLayout);

	QWidget* valueLineWidget = new QWidget(valueWidget);
	QHBoxLayout* valueLineLayout = new QHBoxLayout(valueLineWidget);
	valueLineLayout->setContentsMargins(0, 0, 0, 0);
	valueLineWidget->setLayout(valueLineLayout);
	valueLayout->addWidget(valueLineWidget);

	linkIdField = new PlainLineEdit(valueLineWidget);
	linkIdField->installEventFilter(keyEventFilter);
	connect(linkIdField, SIGNAL(textEdited(const QString&)),
			this, SLOT(idFieldEdited(const QString&)));
	valueLineLayout->addWidget(linkIdField);

	linkChooseButton = new QPushButton(tr("..."), valueLineWidget);
	connect(linkChooseButton, SIGNAL(clicked()), this, SLOT(chooseButtonClicked()));
	valueLineLayout->addWidget(linkChooseButton);

	linkDescLabel = new QLabel(tr("(Invalid)"), valueWidget);
	linkDescLabel->setWordWrap(true);
	linkDescLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	valueLayout->addWidget(linkDescLabel);

	if (compactMode) {
		linkGotoButton = new QPushButton(tr("Open"), valueWidget);
		linkGotoButton->setIcon(QIcon::fromTheme("go-jump", QIcon(":/icons/go-jump.png")));
		connect(linkGotoButton, SIGNAL(clicked()), this, SLOT(gotoButtonClicked()));
		valueLayout->addWidget(linkGotoButton, 0, Qt::AlignLeft);
	} else {
		linkGotoButton = new QPushButton(tr("Open"), valueLineWidget);
		linkGotoButton->setIcon(QIcon::fromTheme("go-jump", QIcon(":/icons/go-jump.png")));
		connect(linkGotoButton, SIGNAL(clicked()), this, SLOT(gotoButtonClicked()));
		valueLineLayout->addWidget(linkGotoButton);
	}

	topLayout->addWidget(valueWidget);
}


void PropertyLinkEditWidget::displayLink(unsigned int linkId)
{
	PartCategory* linkCat = prop->getPartLinkCategory();

	linkIdField->setText(QString("%1").arg(linkId));

	if (linkId == INVALID_PART_ID) {
		linkDescLabel->setText(tr("(Invalid)"));
	} else {
		linkDescLabel->setText(linkCat->getDescription(linkId));
	}
}


void PropertyLinkEditWidget::setState(DisplayWidgetState state)
{
	this->state = state;

	applyState();
}


void PropertyLinkEditWidget::setFlags(flags_t flags)
{
	this->flags = flags;

	applyState();
}


unsigned int PropertyLinkEditWidget::getLink() const
{
	return getValue().toUInt();
}


QString PropertyLinkEditWidget::getValue() const
{
	QString userVal = linkIdField->text();

	return prop->parseUserInputValue(userVal)[0];
}


void PropertyLinkEditWidget::applyState()
{
	if (state == Enabled) {
		if ((flags & ChoosePart)  !=  0) {
			linkIdField->setEnabled(true);
			linkIdField->setReadOnly(true);
			linkChooseButton->setEnabled(false);
			linkDescLabel->setEnabled(true);
			linkGotoButton->setEnabled(false);
		} else {
			linkIdField->setEnabled(true);
			linkIdField->setReadOnly(false);
			linkChooseButton->setEnabled(true);
			linkDescLabel->setEnabled(true);
			linkGotoButton->setEnabled(true);
		}
	} else if (state == Disabled) {
		linkIdField->setEnabled(false);
		linkChooseButton->setEnabled(false);
		linkDescLabel->setEnabled(false);
		linkGotoButton->setEnabled(false);
	} else if (state == ReadOnly) {
		if ((flags & ChoosePart)  !=  0) {
			linkIdField->setEnabled(true);
			linkIdField->setReadOnly(true);
			linkChooseButton->setEnabled(false);
			linkDescLabel->setEnabled(true);
			linkGotoButton->setEnabled(false);
		} else {
			linkIdField->setEnabled(true);
			linkIdField->setReadOnly(true);
			linkChooseButton->setEnabled(false);
			linkDescLabel->setEnabled(true);
			linkGotoButton->setEnabled(true);
		}
	}
}


void PropertyLinkEditWidget::idFieldEdited(const QString& text)
{
	unsigned int linkId;

	try {
		linkId = prop->parseUserInputValue(text)[0].toUInt();
	} catch (InvalidValueException& ex) {
		linkId = 0;
	}

	if (linkId == INVALID_PART_ID) {
		linkDescLabel->setText(tr("(Invalid)"));
	} else {
		PartCategory* linkCat = prop->getPartLinkCategory();
		linkDescLabel->setText(linkCat->getDescription(linkId));
	}

	emit changedByUser();
}


void PropertyLinkEditWidget::chooseButtonClicked()
{
	PartCategory* linkCat = prop->getPartLinkCategory();

	ChoosePartDialog dlg(linkCat, this);

	if (dlg.exec() == ChoosePartDialog::Accepted) {
		unsigned int id = dlg.getChosenID();
		linkDescLabel->setText(linkCat->getDescription(id));

		linkIdField->setText(QString("%1").arg(id));

		emit changedByUser();
	}
}


void PropertyLinkEditWidget::gotoButtonClicked()
{
	PartCategory* linkCat = prop->getPartLinkCategory();

	unsigned int id;

	try {
		id = prop->parseUserInputValue(linkIdField->text())[0].toUInt();
	} catch (InvalidValueException& ex) {
		id = INVALID_PART_ID;
	}

	System::getInstance()->jumpToPart(linkCat, id);
}


void PropertyLinkEditWidget::setValueFocus()
{
	linkIdField->setFocus();
	linkIdField->selectAll();
}
