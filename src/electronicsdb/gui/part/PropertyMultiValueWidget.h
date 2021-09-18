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

#pragma once

#include "../../global.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QWidget>
#include "../../model/part/Part.h"
#include "../../model/AbstractPartProperty.h"
#include "../../model/PartCategory.h"
#include "../../model/PartProperty.h"
#include "PartLinkEditWidget.h"
#include "PropertyComboBox.h"
#include "PropertyDateTimeEdit.h"
#include "PropertyFileWidget.h"
#include "PropertyLineEdit.h"
#include "PropertyTextEdit.h"

namespace electronicsdb
{


class PropertyMultiValueWidget : public QWidget
{
    Q_OBJECT

public:
    PropertyMultiValueWidget(PartCategory* pcat, AbstractPartProperty* aprop, QWidget* parent = nullptr,
                             QObject* keyEventFilter = nullptr);

    void display(const Part& part, QList<QVariant> rawVals);
    void setState(DisplayWidgetState state);
    void setFlags(flags_t flags);
    QList<QVariant> getValues(bool initialValueOnError = false) const;
    void focusValueWidget();

private:
    void applyState();
    void updateLinkListItem(QListWidgetItem* item);

private slots:
    void currentItemChanged(QListWidgetItem* newItem, QListWidgetItem* oldItem);
    void addRequested();
    void removeRequested();
    void boolFieldToggled(bool val);
    void valueFieldEdited(const QString&);
    void textEditEdited();
    void linkWidgetChangedByUser();
    void fileChanged();
    void dateTimeEditedByUser();

signals:
    void changedByUser();

private:
    PartCategory* cat;
    AbstractPartProperty* aprop;

    DisplayWidgetState state;
    flags_t flags;

    Part currentPart;

    QListWidget* listWidget;
    QPushButton* listAddButton;
    QPushButton* listRemoveButton;
    QCheckBox* boolBox;
    PropertyTextEdit* textEdit;
    PropertyLineEdit* lineEdit;
    PartLinkEditWidget* linkWidget;
    PropertyComboBox* enumBox;
    PropertyFileWidget* fileWidget;
    PropertyDateTimeEdit* dateTimeEdit;
};


}
