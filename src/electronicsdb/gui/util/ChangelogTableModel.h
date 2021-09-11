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

#include <set>
#include <QAbstractTableModel>
#include <QList>
#include "ChangelogEntry.h"

namespace electronicsdb
{


class ChangelogTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ChangelogTableModel(QObject* parent = nullptr);
    ~ChangelogTableModel();

    void addEntry(ChangelogEntry* entry);
    void removeEntry(ChangelogEntry* entry);
    void clear();

    const QList<ChangelogEntry*>& getEntries() const { return entries; }

    template <typename EntryType>
    QList<EntryType*> getEntries()
    {
        QList<EntryType*> matches;
        for (ChangelogEntry* e : entries) {
            EntryType* castE = dynamic_cast<EntryType*>(e);
            if (castE) {
                matches << castE;
            }
        }
        return matches;
    }

    template <typename EntryType, typename Predicate>
    EntryType* getEntry(const Predicate& pred)
    {
        for (ChangelogEntry* e : entries) {
            EntryType* castE = dynamic_cast<EntryType*>(e);
            if (castE  &&  pred(castE)) {
                return castE;
            }
        }
        return nullptr;
    }

    template <typename EntryType>
    EntryType* getEntry()
    {
        for (ChangelogEntry* e : entries) {
            EntryType* castE = dynamic_cast<EntryType*>(e);
            if (castE) {
                return castE;
            }
        }
        return nullptr;
    }

    int indexOf(ChangelogEntry* entry) const;
    ChangelogEntry* getEntry(int row) const;

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private slots:
    void entryUpdatedSlot();

signals:
    void entryUpdated(ChangelogEntry* entry);

private:
    QList<ChangelogEntry*> entries;
};


}
