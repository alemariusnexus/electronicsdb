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

#include <QCache>
#include <QMimeData>
#include <QObject>
#include <QUuid>
#include "../../model/container/PartContainer.h"
#include "../../model/part/Part.h"
#include "DragObject.h"

namespace electronicsdb
{


class DragNDropManager : public QObject
{
    Q_OBJECT

public:
    void startDrag(DragObject* dragObj);

    DragObject* findDragObject(const QUuid& uuid);


    QMimeData* startContainerPartsDrag(const QMap<PartContainer, PartList>& partsByCont,
            DragObject** outDragObj = nullptr);
    QMimeData* startContainerPartsDrag(const PartContainer& cont, const PartList& parts,
            DragObject** outDragObj = nullptr);
    PartList acceptContainerPartsDrag(const QMimeData* mimeData, PartContainer& dropCont);

public:
    static DragNDropManager& getInstance();

private:
    DragNDropManager();

private slots:
    void dragObjAccepted();

private:
    QCache<QUuid, DragObject> dragObjs;
};


}
