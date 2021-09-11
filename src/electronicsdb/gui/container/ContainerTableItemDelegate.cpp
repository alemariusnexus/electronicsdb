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

#include "ContainerTableItemDelegate.h"

namespace electronicsdb
{


ContainerTableItemDelegate::ContainerTableItemDelegate(QObject* parent, QObject* eventFilter)
        : QStyledItemDelegate(parent), evtFilter(eventFilter)
{
}


bool ContainerTableItemDelegate::eventFilter(QObject* editor, QEvent* evt)
{
    if (evtFilter  &&  editor->isWidgetType()) {
        if (evtFilter->eventFilter(editor, evt)) {
            return true;
        }
    }

    return QStyledItemDelegate::eventFilter(editor, evt);
}


}
