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

#include "DragNDropManager.h"

#include "../../command/EditStack.h"
#include "../../model/container/PartContainerFactory.h"
#include "../../model/part/PartFactory.h"
#include "../../System.h"

namespace electronicsdb
{


DragNDropManager& DragNDropManager::getInstance()
{
    static DragNDropManager inst;
    return inst;
}

DragNDropManager::DragNDropManager()
        : dragObjs(10)
{
}

void DragNDropManager::startDrag(DragObject* dragObj)
{
    assert(!dragObj->getUuid().isNull());

    connect(dragObj, &DragObject::accepted, this, &DragNDropManager::dragObjAccepted);

    dragObjs.insert(dragObj->getUuid(), dragObj);
}

DragObject* DragNDropManager::findDragObject(const QUuid& uuid)
{
    if (uuid.isNull()) {
        return nullptr;
    }
    return dragObjs.object(uuid);
}

QMimeData* DragNDropManager::startContainerPartsDrag (
        const QMap<PartContainer, PartList>& partsByCont, DragObject** outDragObj
) {
    PartList parts;
    QMap<QString, QVariant> partQIDsByContID;

    for (auto it = partsByCont.begin() ; it != partsByCont.end() ; ++it) {
        const PartContainer& cont = it.key();
        const PartList& contParts = it.value();

        QList<QVariant> partQIDs;
        for (const Part& part : contParts) {
            QVariant qidVar;
            qidVar.setValue(part.getQualifiedID());
            partQIDs << qidVar;
            parts << part;
        }

        partQIDsByContID[QString::number(cont.getID())] = partQIDs;
    }

    DragObject* dragObj = new DragObject("container_parts", partQIDsByContID);
    startDrag(dragObj);

    if (outDragObj) {
        *outDragObj = dragObj;
    }

    QMimeData* mimeData = PartFactory::getInstance().encodePartListToMimeData(parts, dragObj->getUuid());
    return mimeData;
}

QMimeData* DragNDropManager::startContainerPartsDrag (
        const PartContainer& cont, const PartList& parts, DragObject** outDragObj
) {
    QMap<PartContainer, PartList> partsByCont;
    partsByCont[cont] = parts;
    return startContainerPartsDrag(partsByCont, outDragObj);
}

PartList DragNDropManager::acceptContainerPartsDrag(const QMimeData* mimeData, PartContainer& dropCont)
{
    bool valid;
    QUuid uuid;
    PartList parts = PartFactory::getInstance().decodePartListFromMimeData(mimeData, &valid, &uuid);

    if (!valid) {
        return PartList();
    }

    PartContainerFactory& contFact = PartContainerFactory::getInstance();

    DragObject* dragObj = DragNDropManager::getInstance().findDragObject(uuid);

    if (dragObj  &&  dragObj->getContext() == "container_parts") {
        PartContainerList updatedConts;
        QMap<QString, QVariant> partQIDsByContID = dragObj->getDragPayload().toMap();

        for (auto it = partQIDsByContID.begin() ; it != partQIDsByContID.end() ; ++it) {
            PartContainer dragCont = contFact.findContainerByID(static_cast<dbid_t>(it.key().toLongLong()));
            QList<QVariant> partQIDs = it.value().toList();

            if (dragCont == dropCont) {
                // Dropped onto itself -> ignore these parts
                continue;
            }
            if (partQIDs.empty()) {
                continue;
            }

            PartList contParts;
            for (const QVariant& qidVar : partQIDs) {
                contParts << qidVar.value<Part::QualifiedID>().findPart();
            }

            dragCont.removeParts(contParts);
            dropCont.addParts(contParts);

            if (!updatedConts.contains(dragCont)) {
                updatedConts << dragCont;
            }
        }

        if (!updatedConts.empty()) {
            updatedConts << dropCont;
        }

        EditCommand* updCmd = contFact.updateItemsCmd(updatedConts.begin(), updatedConts.end());

        dragObj->enqueueAcceptCommand(updCmd);

        dragObj->accept();
    } else {
        EditCommand* updCmd = nullptr;

        if (!parts.empty()) {
            dropCont.addParts(parts);

            updCmd = contFact.updateItemCmd(dropCont);
        }

        if (dragObj) {
            dragObj->enqueueAcceptCommand(updCmd);

            dragObj->accept();
        } else {
            System::getInstance()->getEditStack()->push(updCmd);
        }
    }

    return parts;
}

void DragNDropManager::dragObjAccepted()
{
    DragObject* dragObj = static_cast<DragObject*>(sender());
    DragObject* takenObj = dragObjs.take(dragObj->getUuid());
    assert(takenObj == dragObj);
    dragObj->deleteLater();
}


}
