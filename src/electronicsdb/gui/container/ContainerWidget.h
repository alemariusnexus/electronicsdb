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

#include <cstdio>
#include <QMap>
#include <QWidget>
#include <ui_ContainerWidget.h>
#include "../../model/container/PartContainer.h"
#include "../../model/PartCategory.h"
#include "ContainerTableModel.h"
#include "ContainerPartTableModel.h"

namespace electronicsdb
{


class ContainerWidget : public QWidget
{
    Q_OBJECT

public:
    ContainerWidget(QWidget* parent = nullptr);
    void setConfigurationName(const QString& name);
    void jumpToContainer(const PartContainer& cont);
    void gotoNextContainer();

protected:
    virtual bool eventFilter(QObject* obj, QEvent* evt);

private:
    QString convertSearchPattern(const QString& str);
    void displayContainer(const PartContainer& cont);
    void removeParts(const PartContainer& cont, const PartList& parts);

private slots:
    void appModelReset();
    void searchRequested();
    void currentContainerChanged(const QModelIndex& newIdx, const QModelIndex& oldIdx);
    void aboutToQuit();
    void partActivated(const QModelIndex& idx);
    void containerAddRequested();
    void containerRemoveRequested();
    void reload();
    void containerSelectionChanged();
    void partsDropped(const QMap<PartContainer, PartList>& parts);
    void partTableViewContextMenuRequested(const QPoint& pos);
    void removeSelectedPartsRequested();

private:
    Ui_ContainerWidget ui;

    QString configName;
    PartContainer currentCont;

    ContainerTableModel* contTableModel;
    ContainerPartTableModel* partTableModel;

    QAction* removePartsAction;
};


}
