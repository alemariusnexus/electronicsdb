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
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include "../../model/container/PartContainer.h"
#include "../../model/AbstractPartProperty.h"
#include "../../model/PartCategory.h"
#include "PropertyComboBox.h"
#include "PropertyDateTimeEdit.h"
#include "PropertyFileWidget.h"
#include "PropertyLineEdit.h"
#include "PropertyMultiValueWidget.h"
#include "PropertyTextEdit.h"

namespace electronicsdb
{


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
        PropertyComboBox* enumBox;
        PropertyFileWidget* fileWidget;
        PropertyDateTimeEdit* dateTimeEdit;
    };

public:
    PartDisplayWidget(PartCategory* partCat, QWidget* parent = nullptr);
    void setDisplayedPart(const Part& part);
    void setState(DisplayWidgetState state);
    void setFlags(flags_t flags);
    bool hasLocalChanges() const { return localChanges; }
    void focusValueWidget(AbstractPartProperty* aprop);
    void focusAuto();

    virtual bool eventFilter(QObject* obj, QEvent* evt);

public slots:
    void clearDisplayedPart();
    bool saveChanges();
    void reload() { setDisplayedPart(currentPart); }

protected:
    virtual void keyPressEvent(QKeyEvent* evt);

private:
    void applyState();
    void applyMultiListPropertyChange(AbstractPartProperty* prop);
    void setHasChanges(bool hasChanges);
    QString handleFile(PartProperty* prop, const QString& fpath, Part::DataMap& suggestedValues);
    AbstractPartProperty* getPropertyForWidget(QWidget* widget);

private slots:
    void buttonBoxClicked(QAbstractButton* b);
    void openProductURLClicked();
    void changedByUser(const QString& = QString());
    void containerLinkActivated(const QString& link);

signals:
    void hasLocalChangesStatusChanged(bool hasLocalChanges);
    void partChosen(const Part& part);
    void defocusRequested();
    void gotoPreviousPartRequested();
    void gotoNextPartRequested();

private:
    PartCategory* partCat;
    DisplayWidgetState state;
    flags_t flags;
    Part currentPart;
    bool localChanges;

    QLabel* headerLabel;
    QLabel* partIdLabel;
    QLabel* partContainersLabel;

    QTabWidget* tabber;
    QDialogButtonBox* dialogButtonBox;
    QPushButton* openProductURLButton;

    QMap<AbstractPartProperty*, PropertyWidgets> propWidgets;
};


}
