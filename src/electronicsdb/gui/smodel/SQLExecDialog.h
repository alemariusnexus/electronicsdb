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

#include <QDialog>
#include <QString>
#include <qsourcehighlite/qsourcehighliter.h>
#include <ui_SQLExecDialog.h>

namespace electronicsdb
{


class SQLExecDialog : public QDialog
{
    Q_OBJECT

public:
    SQLExecDialog(QWidget* parent = nullptr);
    virtual ~SQLExecDialog();

    void setSQLCode(const QString& code);

private slots:
    void buttonBoxClicked(QAbstractButton* button);

    void saveRequested();
    void copyClipboardRequested();

private:
    Ui_SQLExecDialog ui;

    QSourceHighlite::QSourceHighliter* highlighter;
};


}
