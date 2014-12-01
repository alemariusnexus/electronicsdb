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

#include "PartDisplayWidget.h"
#include "System.h"
#include "NoDatabaseConnectionException.h"
#include <nxcommon/exception/InvalidValueException.h>
#include "System.h"
#include "PlainLineEdit.h"
#include "PartCategoryWidget.h"
#include "ChoosePartDialog.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QFormLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QTabWidget>
#include <QtGui/QSplitter>
#include <QtGui/QListWidget>
#include <QtGui/QTableWidget>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QScrollArea>
#include <nxcommon/sql/sql.h>



PartDisplayWidget::PartDisplayWidget(PartCategory* partCat, QWidget* parent)
		: QWidget(parent), partCat(partCat), state(Enabled), flags(0), currentId(0), localChanges(false)
{
	System* sys = System::getInstance();


	bool hasMultiListProps = false;

	for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
		PartProperty* prop = *it;

		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
			hasMultiListProps = true;
			break;
		}
	}


	QVBoxLayout* topLayout = new QVBoxLayout(this);
	setLayout(topLayout);

	tabber = NULL;
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
	connect(partContainersLabel, SIGNAL(linkActivated(const QString&)), this, SLOT(containerLinkActivated(const QString&)));
	formLayout->addRow(tr("Associated Containers"), partContainersLabel);

	unsigned int i = 0;
	for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++, i++) {
		PartProperty* prop = *it;

		PartProperty::Type type = prop->getType();
		flags_t flags = prop->getFlags();

		PropertyWidgets pw;
		memset(&pw, 0, sizeof(PropertyWidgets));

		bool isLink = (type == PartProperty::PartLink);

		if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
			pw.mvWidget = new PropertyMultiValueWidget(prop, tabber, this);
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
			} else if (type == PartProperty::PartLink) {
				pw.linkWidget = new PropertyLinkEditWidget(prop, false, formWidget, this);
				connect(pw.linkWidget, SIGNAL(changedByUser()), this, SLOT(changedByUser()));

				formLayout->addRow(prop->getUserReadableName(), pw.linkWidget);
			} else if (type == PartProperty::File) {
				pw.fileWidget = new PropertyFileWidget(prop, formWidget, this);
				connect(pw.fileWidget, SIGNAL(changedByUser()), this, SLOT(changedByUser()));

				formLayout->addRow(prop->getUserReadableName(), pw.fileWidget);
			} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
				pw.enumBox = new PropertyComboBox(prop, formWidget);
				pw.enumBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
				pw.enumBox->installEventFilter(this);
				connect(pw.enumBox, SIGNAL(changedByUser(const QString&)), this, SLOT(changedByUser()));

				formLayout->addRow(prop->getUserReadableName(), pw.enumBox);
			} else {
				if ((prop->getFlags() & PartProperty::DisplayTextArea)  !=  0) {
					QLabel* label = new QLabel(prop->getUserReadableName(), formWidget);

					pw.textEdit = new PropertyTextEdit(prop, formWidget);
					QSizePolicy pol(QSizePolicy::Expanding, QSizePolicy::Expanding);
					pol.setVerticalStretch(1);
					pw.textEdit->setSizePolicy(pol);
					pw.textEdit->installEventFilter(this);

					connect(pw.textEdit, SIGNAL(editedByUser(const QString&)), this, SLOT(changedByUser()));

					formLayout->addRow(label, pw.textEdit);

					//printf("min: %u,   actual: %u\n", pw.textEdit->minimumHeight(), pw.textEdit->height());
				} else {
					QLabel* label = new QLabel(prop->getUserReadableName(), formWidget);

					pw.lineEdit = new PropertyLineEdit(prop, formWidget);
					pw.lineEdit->installEventFilter(this);
					connect(pw.lineEdit, SIGNAL(editedByUser(const QString&)), this, SLOT(changedByUser()));

					formLayout->addRow(label, pw.lineEdit);
				}
			}
		}

		propWidgets[prop] = pw;
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

	connect(dialogButtonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));



	applyState();


	EditStack* editStack = sys->getEditStack();


	connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)));
	connect(sys, SIGNAL(saveAllRequested()), this, SLOT(saveChanges()));
	connect(editStack, SIGNAL(undone(EditCommand*)), this, SLOT(reload()));
	connect(editStack, SIGNAL(redone(EditCommand*)), this, SLOT(reload()));


	installEventFilter(this);
}


void PartDisplayWidget::setDisplayedPart(unsigned int id)
{
	if (id == INVALID_PART_ID) {
		currentId = INVALID_PART_ID;

		partIdLabel->setText(tr("-"));
		partContainersLabel->setText(QString(tr("-")));

		for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
			PartProperty* prop = *it;

			PartProperty::Type type = prop->getType();
			flags_t flags = prop->getFlags();

			PropertyWidgets pw = propWidgets[prop];

			if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
				pw.mvWidget->display(INVALID_PART_ID, QList<QString>());
			} else {
				if (type == PartProperty::Boolean) {
					pw.boolBox->blockSignals(true);
					pw.boolBox->setChecked(false);
					pw.boolBox->blockSignals(false);
				} else if (type == PartProperty::PartLink) {
					pw.linkWidget->displayLink(INVALID_PART_ID);
				} else if (type == PartProperty::File) {
					pw.fileWidget->displayFile(INVALID_PART_ID, QString());
				} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
					pw.enumBox->displayRecord(INVALID_PART_ID, QString());
				} else {
					if ((flags & PartProperty::DisplayTextArea)  !=  0) {
						pw.textEdit->setText("");
					} else {
						pw.lineEdit->setText("");
					}
				}
			}
		}

		applyState();
	} else {
		System* sys = System::getInstance();

		SQLDatabase sql = sys->getCurrentSQLDatabase();

		if (!sql.isValid()) {
			throw NoDatabaseConnectionException("Database connection needed to display part!",
					__FILE__, __LINE__);
		}

		currentId = id;

		partIdLabel->setText(QString("%1").arg(id));

		QString query = QString (
				"SELECT cid, name "
				"FROM container_part "
				"JOIN container "
				"    ON container.id = container_part.cid "
				"WHERE ptype='%1' AND pid='%2'"
				)
				.arg(partCat->getID())
				.arg(id);

		SQLResult res = sql.sendQuery(query);

		QString contStr("");

		while (res.nextRecord()) {
			unsigned int cid = res.getUInt32(0);
			QString cname = res.getString(1);

			if (!contStr.isEmpty())
				contStr += ", ";

			contStr += QString("<a href=\"%1\">%2</a>").arg(cid).arg(cname);
		}

		if (contStr.isEmpty()) {
			contStr = tr("-");
		}

		partContainersLabel->setText(contStr);

		PartCategory::DataMap data = partCat->getValues(id);

		if (data.empty()) {
			setDisplayedPart(INVALID_PART_ID);
			return;
		}

		for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
			PartProperty* prop = *it;

			QList<QString> vals = data[prop];
			PropertyWidgets pw = propWidgets[prop];

			PartProperty::Type type = prop->getType();
			flags_t flags = prop->getFlags();

			if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
				pw.mvWidget->display(id, vals);
			} else {
				if (type == PartProperty::Boolean) {
					bool val = vals[0].toLongLong() != 0;

					pw.boolBox->blockSignals(true);
					pw.boolBox->setChecked(val);
					pw.boolBox->blockSignals(false);
				} else if (type == PartProperty::PartLink) {
					unsigned int linkId = vals[0].toUInt();

					pw.linkWidget->displayLink(linkId);
				} else if (type == PartProperty::File) {
					pw.fileWidget->displayFile(currentId, vals[0]);
				} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
					pw.enumBox->displayRecord(id, vals[0]);
				} else {
					if ((prop->getFlags() & PartProperty::DisplayTextArea)  !=  0) {
						pw.textEdit->setText(prop->formatValueForEditing(vals));
					} else {
						pw.lineEdit->setText(prop->formatValueForEditing(vals));
					}
				}
			}
		}

		applyState();
	}

	setHasChanges(false);
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

	if (currentId == INVALID_PART_ID) {
		state = Disabled;
	}

	dialogButtonBox->setEnabled(state == Enabled);

	if ((flags & ChoosePart)  !=  0) {
		dialogButtonBox->setStandardButtons(QDialogButtonBox::Open);
	} else {
		dialogButtonBox->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Reset);
	}

	for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
		PartProperty* prop = *it;

		PartProperty::Type type = prop->getType();
		flags_t flags = prop->getFlags();

		PropertyWidgets pw = propWidgets[prop];

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
			} else if (type == PartProperty::PartLink) {
				pw.linkWidget->setState(state);
				pw.linkWidget->setFlags(this->flags);
			} else if (type == PartProperty::File) {
				pw.fileWidget->setState(state);
				pw.fileWidget->setFlags(this->flags);
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
	}
}


void PartDisplayWidget::databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
	if (oldConn  &&  !newConn) {
		setDisplayedPart(INVALID_PART_ID);
	}
}


bool PartDisplayWidget::saveChanges()
{
	if (currentId == INVALID_PART_ID)
		return true;
	if (!hasLocalChanges())
		return true;

	PartCategory::DataMap data;

	for (QMap<PartProperty*, PropertyWidgets>::iterator it = propWidgets.begin() ; it != propWidgets.end() ; it++) {
		PartProperty* prop = it.key();
		PropertyWidgets pw = it.value();

		PartProperty::Type type = prop->getType();
		flags_t flags = prop->getFlags();

		QList<QString>& propData = data[prop];

		try {
			if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
				propData << pw.mvWidget->getValues(true);
			} else {
				if (type == PartProperty::Boolean) {
					propData << (pw.boolBox->isChecked() ? "1" : "0");
				} else if (type == PartProperty::PartLink) {
					propData << pw.linkWidget->getValue();
				} else if (type == PartProperty::File) {
					QString fpath = pw.fileWidget->getValue();

					QString relPath = handleFile(prop, fpath);

					if (relPath.isNull()) {
						return false;
					}

					propData << relPath;
				} else if ((flags & PartProperty::DisplayDynamicEnum)  !=  0) {
					propData << pw.enumBox->getRawValue();
				} else {
					if ((flags & PartProperty::DisplayTextArea)  !=  0) {
						propData << pw.textEdit->getValues();
					} else {
						propData << pw.lineEdit->getValues();
					}
				}
			}
		} catch (InvalidValueException& ex) {
			/*QMessageBox::warning(this, "Invalid Value",
					QString("Invalid value for property '%1':\n\n%2").arg(prop->getUserReadableName()).arg(ex.getMessage()));
			return false;*/
			propData << prop->getInitialValue();
		}
	}

	setHasChanges(false);

	partCat->saveRecord(currentId, data);

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
			setDisplayedPart(currentId);
		}
	} else {
		emit recordChosen(currentId);
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

	if (relPath.startsWith("../")) {
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
							.arg(sfx.isEmpty() ? "" : ".")
							.arg(sfx);
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
							tr("Failed to copy file '%1' to '%2/%3'!").arg(fpath).arg(rootPath).arg(relPath));
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


void PartDisplayWidget::focusValueWidget(PartProperty* prop)
{
	PropertyWidgets pw = propWidgets[prop];

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
	} else if (type == PartProperty::PartLink) {
		pw.linkWidget->setValueFocus();
	} else if (type == PartProperty::File) {
		pw.fileWidget->setValueFocus();
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
}


void PartDisplayWidget::focusAuto()
{
	QWidget* fw = focusWidget();

	if (fw) {
		PartProperty* prop = getPropertyForWidget(fw);

		if (prop) {
			focusValueWidget(prop);
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
			for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
				PartProperty* prop = *it;

				if (objName == QString("mvPropertyWidget_%1").arg(prop->getFieldName())) {
					propWidgets[prop].mvWidget->focusValueWidget();
					break;
				}
			}
		}
	} else {
		if (!propWidgets.empty())
			focusValueWidget(propWidgets.begin().key());
	}
}


void PartDisplayWidget::containerLinkActivated(const QString& link)
{
	unsigned int cid = link.toUInt();

	System::getInstance()->jumpToContainer(cid);
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
	if (evt->type() == QEvent::KeyPress) {
		QKeyEvent* kevt = (QKeyEvent*) evt;

		if (kevt->key() == Qt::Key_PageDown) {
			emit gotoNextPartRequested();
			return true;
		} else if (kevt->key() == Qt::Key_PageUp) {
			emit gotoPreviousPartRequested();
			return true;
		}
	}

	return QWidget::eventFilter(obj, evt);
}


PartProperty* PartDisplayWidget::getPropertyForWidget(QWidget* widget)
{
	for (PartCategory::PropertyIterator it = partCat->getPropertyBegin() ; it != partCat->getPropertyEnd() ; it++) {
		PartProperty* prop = *it;

		PropertyWidgets pw = propWidgets[prop];

		PartProperty::Type type = prop->getType();
		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::MultiValue)  !=  0  &&  (flags & PartProperty::DisplayMultiInSingleField)  ==  0) {
			if (widget == pw.mvWidget  ||  pw.mvWidget->isAncestorOf(widget)) {
				return prop;
			}
		} else {
			if (type == PartProperty::PartLink) {
				if (widget == pw.linkWidget  ||  pw.linkWidget->isAncestorOf(widget)) {
					return prop;
				}
			} else if (type == PartProperty::File) {
				if (widget == pw.fileWidget  ||  pw.fileWidget->isAncestorOf(widget)) {
					return prop;
				}
			} else if (type == PartProperty::Boolean) {
				if (widget == pw.boolBox) {
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
	}

	return NULL;
}

