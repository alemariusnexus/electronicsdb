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

#include <QList>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVariant>
#include "../../command/EditCommand.h"

namespace electronicsdb
{


class DragObject : public QObject
{
    Q_OBJECT

public:
    DragObject(const QString& context = QString(), const QVariant& dragPayload = QVariant());
    DragObject(const DragObject& other) = delete;
    DragObject(DragObject&& other) = delete;
    virtual ~DragObject();

    void setDragPayload(const QVariant& payload);
    void setDropPayload(const QVariant& payload);

    QUuid getUuid() const { return uuid; }
    QString getContext() const { return context; }
    QVariant getDragPayload() const { return dragPayload; }
    QVariant getDropPayload() const { return dropPayload; }

    void accept();

    void enqueueAcceptCommand(EditCommand* cmd);

signals:
    void accepted();

private:
    QUuid uuid;
    QString context;
    QVariant dragPayload;
    QVariant dropPayload;
    QList<EditCommand*> acceptCmds;
};


}
