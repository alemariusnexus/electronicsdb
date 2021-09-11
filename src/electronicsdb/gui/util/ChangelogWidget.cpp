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

#include "ChangelogWidget.h"

#include <QFont>
#include <QHeaderView>
#include <QKeySequence>
#include <QShortcut>
#include <nxcommon/log.h>
#include "../util/RichTextItemDelegate.h"

namespace electronicsdb
{


ChangelogWidget::ChangelogWidget(QWidget* parent)
        : QWidget(parent)
{
    ui.setupUi(this);

    deleteShortcut = new QShortcut(QKeySequence(QKeySequence::Delete), this, SLOT(undoSelectedEntry()));

    connect(ui.logTable, &QTableView::clicked, this, &ChangelogWidget::itemClicked);

    ui.logTable->setItemDelegate(new RichTextItemDelegate);

    model = new ChangelogTableModel(this);
    ui.logTable->setModel(model);

    ui.logTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui.logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui.logTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui.logTable->horizontalHeader()->resizeSection(2, 100);

    ui.logTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void ChangelogWidget::addEntry(ChangelogEntry* entry)
{
    model->addEntry(entry);
}

void ChangelogWidget::removeEntry(ChangelogEntry* entry)
{
    model->removeEntry(entry);
}

void ChangelogWidget::clear()
{
    model->clear();
}

void ChangelogWidget::undoEntry(ChangelogEntry* entry)
{
    if (!entry) {
        return;
    }
    entry->undo();
}

void ChangelogWidget::undoSelectedEntry()
{
    undoEntry(model->getEntry(ui.logTable->currentIndex().row()));
}

void ChangelogWidget::itemClicked(const QModelIndex& idx)
{
    if (idx.column() == 2) {
        undoEntry(model->getEntry(idx.row()));
    }
}


}
