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
#include <QMap>
#include <QSet>
#include "../../model/part/Part.h"
#include "../../model/PartCategory.h"
#include "../../util/Filter.h"

namespace electronicsdb
{


class PartTableModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    struct PartData
    {
        Part part;
        QList<QString> displayData;
        QString contStr;
    };

public:
    PartTableModel(PartCategory* partCat, Filter* filter, QObject* parent = nullptr);

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orient, int role = Qt::DisplayRole) const;
    virtual void sort(int col, Qt::SortOrder order = Qt::AscendingOrder);
    virtual Qt::ItemFlags flags(const QModelIndex& idx) const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual QStringList mimeTypes() const;
    virtual QMimeData* mimeData(const QModelIndexList& indices) const;

    Part getIndexPart(const QModelIndex& idx) const;
    QModelIndex getIndexWithPart(const Part& part);

public slots:
    void updateData();

private:
    void cacheData(int rowBegin, int size = 1, bool fullData = true);
    PartData* getPartData(int row);
    int partToRow(const Part& part, bool fullSearch = false);

signals:
    void updateRequested();

private:
    PartCategory* partCat;
    Filter* filter;

    int cachedNumRows;

    QCache<int, PartData> dataCache;
    QCache<dbid_t, int> idToRowCache;

    uint8_t dragIdentifierCounter;
};


}
