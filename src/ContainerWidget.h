/*
	Copyright 2010-2014 David "Alemarius Nexus" Lerch

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

#ifndef CONTAINERWIDGET_H_
#define CONTAINERWIDGET_H_

#include "global.h"
#include "ContainerTableModel.h"
#include "ContainerPartTableModel.h"
#include "PartCategory.h"
#include "ContainerEditCommand.h"
#include <ui_ContainerWidget.h>
#include <QtGui/QWidget>
#include <cstdio>



class ContainerWidget : public QWidget
{
	Q_OBJECT

private:
	struct PartIdentifier
	{
		PartCategory* cat;
		unsigned int id;
	};

public:
	ContainerWidget(QWidget* parent = NULL);
	void setConfigurationName(const QString& name);
	void jumpToContainer(unsigned int cid);
	void gotoNextContainer();

protected:
	virtual bool eventFilter(QObject* obj, QEvent* evt);

private:
	QString convertSearchPattern(const QString& str);
	void displayContainer(unsigned int id);
	ContainerEditCommand* buildAddPartsCommand(unsigned int cid, ContainerPartTableModel::PartList parts);
	ContainerEditCommand* buildRemovePartsCommand(unsigned int cid, ContainerPartTableModel::PartList parts);
	void removeParts(unsigned int cid, ContainerPartTableModel::PartList parts);

private slots:
	void databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);
	void searchRequested();
	void currentContainerChanged(const QModelIndex& newIdx, const QModelIndex& oldIdx);
	void aboutToQuit();
	void partActivated(const QModelIndex& idx);
	void containerAddRequested();
	void containerRemoveRequested();
	void reload();
	void containerSelectionChanged();
	void partsDropped(ContainerPartTableModel::PartList parts);
	void partsDragged(ContainerPartTableModel::PartList parts);
	void partTableViewContextMenuRequested(const QPoint& pos);
	void removeSelectedPartsRequested();

private:
	Ui_ContainerWidget ui;

	QString configName;
	unsigned int currentID;

	ContainerTableModel* contTableModel;
	ContainerPartTableModel* partTableModel;

	QAction* removePartsAction;

	static ContainerEditCommand* lastDragCommand;
};

#endif /* CONTAINERWIDGET_H_ */
