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

#include <QPushButton>
#include <QSplitter>
#include <QWidget>
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../../util/Filter.h"
#include "FilterWidget.h"
#include "ListingTable.h"
#include "PartDisplayWidget.h"
#include "PartTableModel.h"

namespace electronicsdb
{


class PartCategoryWidget : public QWidget
{
    Q_OBJECT

public:
    PartCategoryWidget(PartCategory* partCat, QWidget* parent = nullptr);
    PartDisplayWidget* getPartDisplayWidget() { return displayWidget; }
    void jumpToPart(const Part& part);
    void setDisplayFlags(flags_t flags);

public slots:
    void gotoPreviousPart();
    void gotoNextPart();

private:
    void updateButtonStates();

private slots:
    void aboutToQuit();
    void listingCurrentPartChanged(const Part& part);
    void listingPartActivated(const Part& part);
    void listingAddPartRequested();
    void listingDeletePartsRequested(const PartList& parts);
    void listingDuplicatePartsRequested(const PartList& parts);
    void filterApplied();
    void rebuildListTable();
    void recordAddRequested();
    void recordRemoveRequested();
    void recordDuplicateRequested();
    void partChosenSlot(const Part& part);
    void displayWidgetDefocusRequested();
    void populateDatabaseDependentUI();
    void listingTableSelectionChanged(const PartList& parts);

signals:
    void partChosen(const Part& part);

private:
    PartCategory* partCat;
    Filter* filter;

    flags_t displayFlags;

    FilterWidget* filterWidget;
    ListingTable* listingTable;
    PartTableModel* listingTableModel;
    PartDisplayWidget* displayWidget;

    QSplitter* mainSplitter;
    QSplitter* displaySplitter;

    QPushButton* recordAddButton;
    QPushButton* recordRemoveButton;
    QPushButton* recordDuplicateButton;
};


}
