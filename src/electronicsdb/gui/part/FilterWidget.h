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
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QMap>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>
#include "../../model/PartCategory.h"
#include "../../util/Filter.h"

namespace electronicsdb
{


class FilterWidget : public QWidget
{
    Q_OBJECT

public:
    FilterWidget(QWidget* parent, PartCategory* partCat);

    void readFilter(Filter* filter);

private:
    void invalidate();

private slots:
    void populateAppModelUI(const QMap<PartProperty*, QList<QVariant>>& selectedData = QMap<PartProperty*, QList<QVariant>>());
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


}
