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
#include <QList>
#include "../../model/container/PartContainer.h"
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../dragndrop/DragObject.h"

namespace electronicsdb
{


class ContainerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ContainerTableModel(QObject* parent = nullptr);
    virtual ~ContainerTableModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role = Qt::DisplayRole) const override;
    void sort(int col, Qt::SortOrder order = Qt::AscendingOrder) override;
    Qt::ItemFlags flags(const QModelIndex& idx) const override;
    bool setData(const QModelIndex& idx, const QVariant& val, int role = Qt::EditRole) override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int col, const QModelIndex& parent) override;
    QMimeData* mimeData(const QModelIndexList& indices) const override;

    void setFilter(const QString& sqlLikeFilter);
    void reload();
    PartContainer getIndexContainer(const QModelIndex& idx) const;
    QModelIndex getIndexFromContainer(const PartContainer& cont) const;

signals:
    void partsDropped(const QMap<PartContainer, PartList>& parts);

private:
    void clear();

private:
    QString currentFilter;
    PartContainerList containers;
    bool orderAscending;
};


}
