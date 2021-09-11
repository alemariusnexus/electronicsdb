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

#include <QAbstractTableModel>
#include <QCache>
#include <QList>
#include <QMimeData>
#include <QStringList>
#include <QUuid>
#include "../../model/container/PartContainer.h"
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../dragndrop/DragObject.h"

namespace electronicsdb
{


class ContainerPartTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ContainerPartTableModel(QObject* parent = nullptr);
    virtual ~ContainerPartTableModel();

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& idx) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int col, const QModelIndex& parent) override;
    QMimeData* mimeData(const QModelIndexList& indices) const override;

    void display(const PartContainer& cont, PartList parts);
    PartContainer getDisplayedContainer() const;
    Part getIndexPart(const QModelIndex& idx) const;
    QModelIndex getIndexFromPart(const Part& part) const;

signals:
    void partsDropped(const QMap<PartContainer, PartList>& parts);

private:
    void doClear();

private:
    PartContainer currentCont;
    PartList parts;
};


}
