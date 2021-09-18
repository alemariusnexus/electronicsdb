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
#include <QList>
#include <ui_SettingsDialog.h>
#include "../../db/DatabaseConnection.h"
#include "../../System.h"

namespace electronicsdb
{


class SettingsDialog : public QDialog
{
    Q_OBJECT

private:
    struct LocalPermDbConn
    {
        DatabaseConnection* globalConn;
        DatabaseConnection* localConn;
    };

    typedef	QList<LocalPermDbConn*> LocalPermDbConnList;

public:
    SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

protected:
    void closeEvent(QCloseEvent* evt) override;

private:
    void applyCurrentConnectionChanges();
    void rebuildStartupConnectionBox();
    void apply();

    bool applyDatabaseSettings();

private slots:
    void fontDefaultBoxStateChanged();

    void connListRowChanged(int row);
    void buttonBoxClicked(QAbstractButton* b);
    void connAddRequested();
    void connRemoveRequested();
    void connNameChanged(const QString& name);

private:
    Ui_SettingsDialog ui;
    LocalPermDbConnList localPermDbConns;
    LocalPermDbConn* curEdConn;

};


}
