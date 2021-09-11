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

#include "ChangelogTableModel.h"

#include <algorithm>
#include <QFont>
#include <QIcon>
#include <nxcommon/log.h>
#include "../../System.h"

namespace electronicsdb
{


ChangelogTableModel::ChangelogTableModel(QObject* parent)
        : QAbstractTableModel(parent)
{
}

ChangelogTableModel::~ChangelogTableModel()
{
    clear();
}

void ChangelogTableModel::addEntry(ChangelogEntry* entry)
{
    beginInsertRows(QModelIndex(), entries.size(), entries.size());

    entries << entry;

    endInsertRows();

    connect(entry, &ChangelogEntry::updated, this, &ChangelogTableModel::entryUpdatedSlot);
}

void ChangelogTableModel::removeEntry(ChangelogEntry* entry)
{
    int idx = entries.indexOf(entry);
    if (idx < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), idx, idx);

    entries.removeAt(idx);
    entry->deleteLater();

    endRemoveRows();
}

void ChangelogTableModel::clear()
{
    if (entries.empty()) {
        return;
    }

    beginRemoveRows(QModelIndex(), 0, entries.size()-1);

    for (ChangelogEntry* entry : entries) {
        entry->deleteLater();
    }
    entries.clear();

    endRemoveRows();
}

int ChangelogTableModel::indexOf(ChangelogEntry* entry) const
{
    return entries.indexOf(entry);
}

ChangelogEntry* ChangelogTableModel::getEntry(int row) const
{
    if (row < 0  ||  row >= entries.size()) {
        return nullptr;
    }
    return entries[row];
}

int ChangelogTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return entries.size();
}

int ChangelogTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return 3;
}

QVariant ChangelogTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (index.row() < 0  ||  index.row() >= entries.size()) {
        return QVariant();
    }
    if (index.column() < 0  ||  index.column() >= columnCount(QModelIndex())) {
        return QVariant();
    }

    System* sys = System::getInstance();

    ChangelogEntry* entry = getEntry(index.row());

    if (index.column() == 0) {
        if (role == Qt::DisplayRole) {
            return QString("%1").arg(entry->getEntryDescription());
        }
    } else if (index.column() == 1) {
        if (role == Qt::DisplayRole) {
            QString desc = entry->getChangeDescription();
            QStringList errmsgs = entry->getErrorMessages();

            QString errDesc;
            if (!errmsgs.empty()) {
                desc += QString("<font color=\"%1\">")
                        .arg(sys->getAppPaletteColor(System::PaletteColorStatusError).name());
                for (QString errmsg : errmsgs) {
                    desc += "<br/>";
                    desc += QString("Error: %1").arg(errmsg);
                }
                desc += "</font>";
            }

            return desc;
        }
    } else if (index.column() == 2) {
        if (role == Qt::DisplayRole) {
            return QString("<img src=\":/icons/edit-delete.png\" width=\"16\" height=\"16\"/>%1").arg(tr("Undo"));
        }
    }

    if (role == Qt::FontRole) {
        QFont font;

        if (index.column() == 1) {
            font.setPointSize(font.pointSize()+2);
        } else {
            font.setPointSize(font.pointSize()+3);
        }

        return font;
    }

    return QVariant();
}

QVariant ChangelogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) {
        return QVariant();
    }
    if (section < 0  ||  section >= columnCount(QModelIndex())) {
        return QVariant();
    }

    if (section == 0) {
        if (role == Qt::DisplayRole) {
            return tr("Entity");
        }
    } else if (section == 1) {
        if (role == Qt::DisplayRole) {
            return tr("Change Description");
        }
    }

    return QVariant();
}

void ChangelogTableModel::entryUpdatedSlot()
{
    ChangelogEntry* entry = static_cast<ChangelogEntry*>(sender());
    int idx = indexOf(entry);
    if (idx < 0) {
        return;
    }

    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columnCount(QModelIndex()), QModelIndex()));

    emit entryUpdated(entry);
}


}
