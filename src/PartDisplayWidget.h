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

#ifndef PARTDISPLAYWIDGET_H_
#define PARTDISPLAYWIDGET_H_

#include "global.h"
#include "PartCategory.h"
#include "PropertyLineEdit.h"
#include "PropertyTextEdit.h"
#include "PropertyComboBox.h"
#include "PropertyLinkEditWidget.h"
#include "PropertyMultiValueWidget.h"
#include "PropertyFileWidget.h"
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QTabWidget>
#include <QtGui/QLineEdit>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QCheckBox>
#include <QtGui/QKeyEvent>


class PartDisplayWidget : public QWidget
{
	Q_OBJECT

private:
	struct PropertyWidgets
	{
		PropertyMultiValueWidget* mvWidget;
		PropertyLineEdit* lineEdit;
		PropertyTextEdit* textEdit;
		QCheckBox* boolBox;
		PropertyLinkEditWidget* linkWidget;
		PropertyComboBox* enumBox;
		PropertyFileWidget* fileWidget;
	};

public:
	PartDisplayWidget(PartCategory* partCat, QWidget* parent = NULL);
	void setDisplayedPart(unsigned int id = INVALID_PART_ID);
	void setState(DisplayWidgetState state);
	void setFlags(flags_t flags);
	bool hasLocalChanges() const { return localChanges; }
	void focusValueWidget(PartProperty* prop);
	void focusAuto();

	virtual bool eventFilter(QObject* obj, QEvent* evt);

public slots:
	bool saveChanges();
	void reload() { setDisplayedPart(currentId); }

protected:
	virtual void keyPressEvent(QKeyEvent* evt);

private:
	void applyState();
	void applyMultiListPropertyChange(PartProperty* prop);
	void setHasChanges(bool hasChanges);
	QString handleFile(PartProperty* prop, const QString& fpath);
	PartProperty* getPropertyForWidget(QWidget* widget);

private slots:
	void databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);
	void buttonBoxClicked(QAbstractButton* b);
	void changedByUser(const QString& = QString());
	void containerLinkActivated(const QString& link);

signals:
	void hasLocalChangesStatusChanged(bool hasLocalChanges);
	void recordChosen(unsigned int id);
	void defocusRequested();
	void gotoPreviousPartRequested();
	void gotoNextPartRequested();

private:
	PartCategory* partCat;
	DisplayWidgetState state;
	flags_t flags;
	unsigned int currentId;
	bool localChanges;

	QLabel* headerLabel;
	QLabel* partIdLabel;
	QLabel* partContainersLabel;

	QTabWidget* tabber;
	QDialogButtonBox* dialogButtonBox;

	QMap<PartProperty*, PropertyWidgets> propWidgets;
};

#endif /* PARTDISPLAYWIDGET_H_ */
