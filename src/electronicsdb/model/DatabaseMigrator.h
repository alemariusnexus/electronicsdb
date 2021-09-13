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

#include "../global.h"

#include <QSqlDatabase>
#include "../db/DatabaseConnection.h"

namespace electronicsdb
{


class DatabaseMigrator : public QObject
{
    Q_OBJECT

public:
    enum MigrationFlags
    {
        MigrateStaticModel      = 0x01,
        MigrateDynamicModel     = 0x02,

        MigrateAll              = MigrateStaticModel | MigrateDynamicModel
    };

    enum BackupFlags
    {
        BackupDatabase          = 0x01,
        BackupFiles             = 0x02,

        BackupAll               = BackupDatabase | BackupFiles
    };

public:
    void migrateTo(DatabaseConnection* srcConn, DatabaseConnection* destConn, int flags);

    void backupCurrentDatabase(const QString& destFile, int flags);

private:
    void doMigrateTo(DatabaseConnection* srcConn, DatabaseConnection* destConn, int flags,
            int& progressCur, int& progressMax);


signals:
    void progressChanged(const QString& statusMsg, int progressCur, int progressMax);
};


}
