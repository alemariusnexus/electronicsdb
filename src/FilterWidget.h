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

#ifndef FILTERWIDGET_H_
#define FILTERWIDGET_H_

#include "global.h"
#include "PartCategory.h"
#include "Filter.h"
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGroupBox>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QDialogButtonBox>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <cstdio>


class FilterWidget : public QWidget
{
	Q_OBJECT

public:
	FilterWidget(QWidget* parent, PartCategory* partCat);

	void readFilter(Filter* filter);

private:
	void invalidate();

private slots:
	void populateDatabaseDependentUI(QMap<PartProperty*, QSet<QString> > selectedData = QMap<PartProperty*, QSet<QString> >());
	void sqlInsertColumnBoxActivated(int idx);
	void ftInsertFieldBoxActivated(int idx);
	void sqlCombFuncBoxChanged(int idx);
	void reset();
	void appliedSlot();
	void partsChanged();

signals:
	void applied();

private:
	PartCategory* partCat;

	QMap<PartProperty*, QListWidget*> propLists;

	QLineEdit* ftField;
	QComboBox* ftInsFieldBox;

	QLineEdit* sqlEditField;
	QComboBox* sqlInsColBox;
	QComboBox* sqlCombFuncBox;

	QPushButton* resetButton;
	QPushButton* applyButton;
};

#endif /* FILTERWIDGET_H_ */
