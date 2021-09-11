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

#include <QKeyEvent>
#include <QResizeEvent>
#include <QTableView>
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../../util/Filter.h"
#include "PartTableModel.h"

namespace electronicsdb
{


class ListingTable : public QTableView
{
    Q_OBJECT

public:
    ListingTable(PartCategory* partCat, Filter* filter, QWidget* parent = nullptr);
    PartTableModel* getModel() { return model; }
    PartList getSelectedParts() const;
    Part getCurrentPart() const;
    void setCurrentPart(const Part& part);
    void setSelectedParts(const PartList& parts);
    void selectParts(const PartList& parts);

public slots:
    void updateData();

protected:
    virtual void keyPressEvent(QKeyEvent* evt);

private:
    void resetColumnSizeDefaults();
    void buildHeaderSectionMenu(QMenu* menu);
    QByteArray saveState();
    bool restoreState(const QByteArray& state);
    void setSectionHidden(int sect, bool hidden);

private slots:
    void partActivatedSlot(const QModelIndex& idx);
    void currentChanged(const QModelIndex& newIdx, const QModelIndex& oldIdx);
    void contextMenuRequested(const QPoint& pos);
    void headerContextMenuRequested(const QPoint& pos);
    void headerSectionVisibilityActionTriggered();
    void aboutToQuit();
    void scaleSectionsToFit();
    void addPartRequestedSlot();
    void deletePartsRequestedSlot();
    void duplicatePartsRequestedSlot();
    void selectionChangedSlot(const QItemSelection&, const QItemSelection&);

signals:
    void currentPartChanged(const Part& part);
    void selectedPartsChanged(const PartList& parts);
    void partActivated(const Part& part);
    void addPartRequested();
    void deletePartsRequested(const PartList& parts);
    void duplicatePartsRequested(const PartList& parts);

private:
    PartCategory* partCat;
    PartTableModel* model;
    QList<float> relColWidths;
    bool ignoreSectionResize;
};


}
