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

#include <QTimer>
#include <QWidget>
#include <ui_PartLinkEditWidget.h>
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../../model/PartLinkType.h"

namespace electronicsdb
{


class PartLinkEditWidget : public QWidget
{
    Q_OBJECT

public:
    PartLinkEditWidget(PartCategory* pcat, PartLinkType* ltype, QWidget* parent = nullptr,
                       QObject* keyEvtFilter = nullptr);

    void displayLinkedPart(const Part& part);
    void setState(DisplayWidgetState state);
    void setFlags(flags_t flags);
    Part getLinkedPart() const;
    void setValueFocus();

private:
    QList<PartCategory*> getLinkedCategories() const;
    PartCategory* getLinkedCategory() const;

private slots:
    void updateState();
    void linkedPartChangedByUser();
    void linkedPartChangedByUserDelayed();

    void choosePartRequested();
    void jumpRequested();
    void quickOpenRequested();

signals:
    void changedByUser();

private:
    Ui_PartLinkEditWidget ui;

    PartCategory* pcat;
    PartLinkType* ltype;

    DisplayWidgetState state;
    flags_t flags;

    QTimer delayedChangedTimer;
};


}
