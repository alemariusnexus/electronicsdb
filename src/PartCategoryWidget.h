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

#ifndef PARTCATEGORYWIDGET_H_
#define PARTCATEGORYWIDGET_H_

#include "global.h"
#include "PartCategory.h"
#include "FilterWidget.h"
#include "PartDisplayWidget.h"
#include "ListingTable.h"
#include "PartTableModel.h"
#include "Filter.h"
#include <QtGui/QWidget>
#include <QtGui/QSplitter>
#include <QtGui/QPushButton>



class PartCategoryWidget : public QWidget
{
	Q_OBJECT

public:
	PartCategoryWidget(PartCategory* partCat, QWidget* parent = NULL);
	PartDisplayWidget* getPartDisplayWidget() { return displayWidget; }
	void jumpToPart(unsigned int id);
	void setDisplayFlags(flags_t flags);

public slots:
	void gotoPreviousPart();
	void gotoNextPart();

private slots:
	void aboutToQuit();
	void listingCurrentPartChanged(unsigned int id);
	void listingPartActivated(unsigned int id);
	void filterApplied();
	void rebuildListTable();
	void databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);
	void recordAddRequested();
	void recordRemoveRequested();
	void recordChosenSlot(unsigned int id);
	void displayWidgetDefocusRequested();

signals:
	void recordChosen(unsigned int id);

private:
	PartCategory* partCat;
	Filter* filter;

	flags_t displayFlags;

	FilterWidget* filterWidget;
	ListingTable* listingTable;
	PartTableModel* listingTableModel;
	PartDisplayWidget* displayWidget;

	QSplitter* mainSplitter;
	QSplitter* displaySplitter;

	QPushButton* recordAddButton;
	QPushButton* recordRemoveButton;
};

#endif /* PARTCATEGORYWIDGET_H_ */
