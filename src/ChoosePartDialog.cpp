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

#include "ChoosePartDialog.h"
#include "PartCategoryWidget.h"
#include <QtCore/QSettings>
#include <QtGui/QVBoxLayout>
#include <QtGui/QApplication>




ChoosePartDialog::ChoosePartDialog(PartCategory* partCat, QWidget* parent)
		: QDialog(parent), partCat(partCat), chosenID(INVALID_PART_ID)
{
	setModal(true);

	QVBoxLayout* chooseLayout = new QVBoxLayout(this);
	setLayout(chooseLayout);

	PartCategoryWidget* chooseWidget = new PartCategoryWidget(partCat, this);
	chooseWidget->setDisplayFlags(ChoosePart);
	connect(chooseWidget, SIGNAL(recordChosen(unsigned int)), this, SLOT(recordChosen(unsigned int)));
	chooseLayout->addWidget(chooseWidget);


	QSettings s;

	s.beginGroup(QString("gui_geometry_%1").arg(partCat->getTableName()));
    restoreGeometry(s.value("cpd_dialog_geometry").toByteArray());
    s.endGroup();
}


ChoosePartDialog::~ChoosePartDialog()
{
	QSettings s;

	s.beginGroup(QString("gui_geometry_%1").arg(partCat->getTableName()));
	s.setValue("cpd_dialog_geometry", saveGeometry());
	s.endGroup();
}


void ChoosePartDialog::recordChosen(unsigned int id)
{
	chosenID = id;
	accept();
}
