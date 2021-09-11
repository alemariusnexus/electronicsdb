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

#include <QMap>
#include <QWidget>
#include <ui_ConnectionEditWidget.h>
#include "../../db/DatabaseConnection.h"
#include "../../db/DatabaseConnectionWidget.h"
#include "../../db/SQLDatabaseWrapper.h"

namespace electronicsdb
{


class ConnectionEditWidget : public QWidget
{
    Q_OBJECT

private:
    struct WrappedDatabaseType
    {
        SQLDatabaseWrapper* wrapper;
        SQLDatabaseWrapper::DatabaseType type;
        DatabaseConnectionWidget* connWidget;
        bool showDDLWarning;
    };

public:
    ConnectionEditWidget(QWidget* parent = nullptr);
    virtual ~ConnectionEditWidget();

    void setConnectionName(const QString& name);
    void setConnectionDriverName(const QString& driverName);

    void setFileRootPath(const QString& path);

    void display(DatabaseConnection* conn);
    void clear();

    QString getConnectionName() const { return ui.connNameField->text(); }
    QString getConnectionDriverName() const
            { return ui.connTypeBox->itemData(ui.connTypeBox->currentIndex()).toString(); }

    QString getFileRootPath() const { return ui.connFileRootField->text(); }

    DatabaseConnection* createConnection();

protected:
    void changeEvent(QEvent* evt) override;

private:
    void updateState();

private slots:
    void connTypeBoxActivated(int idx);
    void connFileRootPathChooseButtonClicked();

signals:
    void connectionNameChanged(const QString& name);

private:
    Ui_ConnectionEditWidget ui;

    QMap<QString, WrappedDatabaseType> dbTypesByDriver;
};


}
