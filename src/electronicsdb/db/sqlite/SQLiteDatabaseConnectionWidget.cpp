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

#include "SQLiteDatabaseConnectionWidget.h"

#include <QFileDialog>
#include "SQLiteDatabaseConnection.h"

namespace electronicsdb
{


SQLiteDatabaseConnectionWidget::SQLiteDatabaseConnectionWidget (
        const SQLDatabaseWrapper::DatabaseType& dbType, QWidget* parent
)       : DatabaseConnectionWidget(parent), dbType(dbType)
{
    ui.setupUi(this);

    connect(ui.dbFileChooseButton, &QPushButton::clicked, this, &SQLiteDatabaseConnectionWidget::onDbFileChoose);

    SQLiteDatabaseConnection* conn = new SQLiteDatabaseConnection(dbType);

    setDatabaseFile(conn->getDatabasePath());

    delete conn;
}


void SQLiteDatabaseConnectionWidget::onDbFileChoose()
{
    QString fpath = QFileDialog::getOpenFileName(this, tr("Choose a database file"), ui.dbFileEdit->text(),
            tr("SQLite Databases (*.db *.sqlite)"));
    if (fpath.isNull())
        return;
    setDatabaseFile(fpath);
}

void SQLiteDatabaseConnectionWidget::display(DatabaseConnection* conn)
{
    SQLiteDatabaseConnection* sconn = dynamic_cast<SQLiteDatabaseConnection*>(conn);

    if (!sconn) {
        clear();
        return;
    }

    connID = conn->getID();
    setDatabaseFile(sconn->getDatabasePath());
}

void SQLiteDatabaseConnectionWidget::clear()
{
    connID = QString();
    setDatabaseFile(QString());
}

DatabaseConnection* SQLiteDatabaseConnectionWidget::createConnection()
{
    SQLiteDatabaseConnection* conn = new SQLiteDatabaseConnection(dbType, connID);
    conn->setDatabasePath(getDatabaseFile());
    return conn;
}


}
