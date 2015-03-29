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

#ifndef LISTINGTABLE_H_
#define LISTINGTABLE_H_

#include "global.h"
#include "PartCategory.h"
#include "PartTableModel.h"
#include "Filter.h"
#include <QtGui/QTableView>
#include <QtGui/QResizeEvent>
#include <QtGui/QKeyEvent>


class ListingTable : public QTableView
{
	Q_OBJECT

public:
	ListingTable(PartCategory* partCat, Filter* filter, QWidget* parent = NULL);
	PartTableModel* getModel() { return model; }
	QList<unsigned int> getSelectedPartIDs() const;
	unsigned int getCurrentPart() const;
	void setCurrentPart(unsigned int id);
	void setSelectedParts(const QList<unsigned int>& ids);
	void selectParts(const QList<unsigned int>& ids);

public slots:
	void updateData();

protected:
	virtual void keyPressEvent(QKeyEvent* evt);

private:
	void resetColumnSizeDefaults();
	void buildHeaderSectionMenu(QMenu* menu);
	QByteArray saveState();
	bool restoreState(const QByteArray& state);
	void setSectionHidden(int sect, bool hidden);

private slots:
	void partActivatedSlot(const QModelIndex& idx);
	void currentChanged(const QModelIndex& newIdx, const QModelIndex& oldIdx);
	void contextMenuRequested(const QPoint& pos);
	void headerContextMenuRequested(const QPoint& pos);
	void headerSectionVisibilityActionTriggered();
	void aboutToQuit();
	void scaleSectionsToFit();
	void addPartRequestedSlot();
	void deletePartsRequestedSlot();
	void duplicatePartsRequestedSlot();
	void selectionChangedSlot(const QItemSelection&, const QItemSelection&);

signals:
	void currentPartChanged(unsigned int id);
	void selectedPartsChanged(const QList<unsigned int>& ids);
	void partActivated(unsigned int id);
	void addPartRequested();
	void deletePartsRequested(const QList<unsigned int>& ids);
	void duplicatePartsRequested(const QList<unsigned int>& ids);

private:
	PartCategory* partCat;
	PartTableModel* model;
	QList<float> relColWidths;
	bool ignoreSectionResize;
};

#endif /* LISTINGTABLE_H_ */
