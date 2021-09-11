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

#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../../model/PartProperty.h"
#include "../util/PlainLineEdit.h"

namespace electronicsdb
{


class PropertyFileWidget : public QWidget
{
    Q_OBJECT

public:
    PropertyFileWidget(PartProperty* prop, QWidget* parent = nullptr, QObject* keyEventFilter = nullptr);
    void displayFile(const Part& part, const QString& fileName);
    void setState(DisplayWidgetState state);
    void setFlags(flags_t flags);
    void setValueFocus();
    QString getValue() const;

private:
    void applyState();
    QString getCurrentAbsolutePath();

private slots:
    void fileChosen();
    void textEdited(const QString& text);
    void openRequested();
    void copyClipboardRequested();

signals:
    void changedByUser();

private:
    PartCategory* cat;
    PartProperty* prop;

    Part currentPart;

    DisplayWidgetState displayState;
    flags_t displayFlags;

    PlainLineEdit* fileEdit;
    QPushButton* chooseButton;
    QPushButton* openButton;
    QPushButton* clipboardButton;
};


}
