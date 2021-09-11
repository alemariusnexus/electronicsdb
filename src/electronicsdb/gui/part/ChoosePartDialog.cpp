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

#include "ChoosePartDialog.h"

#include <QApplication>
#include <QSettings>
#include <QVBoxLayout>
#include "PartCategoryWidget.h"

namespace electronicsdb
{


ChoosePartDialog::ChoosePartDialog(PartCategory* partCat, QWidget* parent)
        : QDialog(parent), partCat(partCat)
{
    // TODO: Allow selecting in multiple categories, not just one. Would be useful for part links.

    setModal(true);

    QVBoxLayout* chooseLayout = new QVBoxLayout(this);
    setLayout(chooseLayout);

    PartCategoryWidget* chooseWidget = new PartCategoryWidget(partCat, this);
    chooseWidget->setDisplayFlags(ChoosePart);
    connect(chooseWidget, &PartCategoryWidget::partChosen, this, &ChoosePartDialog::partChosen);
    chooseLayout->addWidget(chooseWidget);


    QSettings s;

    s.beginGroup(QString("gui/choose_part_dialog/%1").arg(partCat->getTableName()));
    restoreGeometry(s.value("geometry").toByteArray());
    s.endGroup();
}


ChoosePartDialog::~ChoosePartDialog()
{
    QSettings s;

    s.beginGroup(QString("gui/choose_part_dialog/%1").arg(partCat->getTableName()));
    s.setValue("geometry", saveGeometry());
    s.endGroup();
}


void ChoosePartDialog::partChosen(const Part& part)
{
    chosenPart = part;
    accept();
}


}
