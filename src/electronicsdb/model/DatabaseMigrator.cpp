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

#include "DatabaseMigrator.h"

#include <memory>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/exception/InvalidValueException.h>
#include <nxcommon/log.h>
#include <zip/zip.h>
#include "../command/CompoundEditCommand.h"
#include "../db/sqlite/SQLiteDatabaseConnection.h"
#include "../db/SQLDatabaseWrapperFactory.h"
#include "../db/SQLTransaction.h"
#include "../exception/InvalidCredentialsException.h"
#include "../exception/IOException.h"
#include "../exception/SQLException.h"
#include "../model/container/PartContainerFactory.h"
#include "../model/part/PartFactory.h"
#include "../System.h"
#include "StaticModelManager.h"

namespace electronicsdb
{


void DatabaseMigrator::migrateTo(DatabaseConnection* srcConn, DatabaseConnection* destConn, int flags)
{
    System* sys = System::getInstance();

    bool ok;
    QString pw;
    int numTries = 0;

    sys->disconnect(false);

    do {
        pw = sys->askConnectDatabasePassword(srcConn, &ok);
        numTries++;

        if (ok) {
            if (!sys->connectDatabase(srcConn, pw)) {
                ok = false;
            }
        }
    } while (!ok  &&  numTries < 3);

    if (!ok) {
        throw InvalidCredentialsException("Unable to connect to source database", __FILE__, __LINE__);
    }

    int progressCur = 0;
    int progressMax = 0;

    doMigrateTo(srcConn, destConn, flags, progressCur, progressMax);

    QSqlDatabase::removeDatabase("migrationDestDb");

    sys->disconnect(false);
}

void DatabaseMigrator::doMigrateTo (
        DatabaseConnection* srcConn, DatabaseConnection* destConn, int flags, int& progressCur, int& progressMax
) {
    // NOTE: This method assumes that we're already connected to srcConn, and that the static model is fully loaded.

    bool staticModel = (flags & MigrateStaticModel) != 0;
    bool dynModel = (flags & MigrateDynamicModel) != 0;

    if (!staticModel  &&  dynModel) {
        return;
    }

    if (dynModel  &&  !staticModel) {
        throw InvalidStateException("Invalid migration flag combination", __FILE__, __LINE__);
    }

    System* sys = System::getInstance();
    StaticModelManager& smMgr = *StaticModelManager::getInstance();

    QSqlDatabase srcDb = QSqlDatabase::database();
    QSqlDatabase destDb;

    bool ok;
    QString pw;
    int numTries = 0;

    do {
        pw = sys->askConnectDatabasePassword(destConn, &ok);
        numTries++;

        if (ok) {
            try {
                destDb = destConn->openDatabase(pw, "migrationDestDb");
            } catch (SQLException&) {
                ok = false;
            }
        }
    } while (!ok  &&  numTries < 3);

    if (!ok) {
        throw InvalidCredentialsException("Unable to connect to source database", __FILE__, __LINE__);
    }


    progressMax += 1; // Destroy old schema

    if ((flags & MigrateStaticModel) != 0) {
        progressMax += 1; // Static model
    }
    if ((flags & MigrateDynamicModel) != 0) {
        const int numDynPasses = 2;

        progressMax += 1 * numDynPasses; // Containers
        progressMax += sys->getPartCategories().size() * numDynPasses; // Parts
    }

    progressMax += 1; // Commit transaction


    std::unique_ptr<SQLDatabaseWrapper> destDbw(SQLDatabaseWrapperFactory::getInstance().create(destDb));

    destDbw->initSession();

    emit progressChanged(tr("Destroying old schema..."), progressCur, progressMax);

    SQLTransaction trans = destDbw->beginDDLTransaction();

    destDbw->destroySchema(false);

    progressCur++;


    emit progressChanged(tr("Migrating static model..."), progressCur, progressMax);

    destDbw->createSchema(false);

    QStringList stmts;

    for (PartPropertyMetaType* mtype : sys->getPartPropertyMetaTypes()) {
        stmts << smMgr.generateMetaTypeCode(mtype, destDb);
    }
    stmts << smMgr.generateCategorySchemaDeltaCode(sys->getPartCategories(), {}, {}, {}, destDb);
    stmts << smMgr.generatePartLinkTypeDeltaCode(sys->getPartLinkTypes(), {}, {}, destDb);

    smMgr.executeSchemaStatements(stmts, false, destDb);

    progressCur++;


    if ((flags & MigrateDynamicModel) != 0) {
        PartContainerFactory& contFact = PartContainerFactory::getInstance();
        PartFactory& partFact = PartFactory::getInstance();


        // ********** PASS 1: INSERT DATA WITHOUT ASSOCS **********

        emit progressChanged(tr("Migrating containers (pass 1/2)..."), progressCur, progressMax);

        PartContainerList conts = contFact.loadItems(QString(), QString(), 0);

        try {
            contFact.setDatabaseConnectionName(destDb.connectionName());

            EditCommand* insCmd = contFact.insertItemsCmd(conts.begin(), conts.end());
            insCmd->commit();
            delete insCmd;

            destDbw->adjustAutoIncrementColumn("container", "id");

            contFact.setDatabaseConnectionName(QString());
        } catch (std::exception&) {
            contFact.setDatabaseConnectionName(QString());
            throw;
        }

        conts.clear();

        progressCur++;

        for (PartCategory* pcat : sys->getPartCategories()) {
            emit progressChanged(tr("Migrating parts for category '%1' (pass 1/2)").arg(pcat->getID()),
                                 progressCur, progressMax);

            PartList parts = partFact.loadItemsSingleCategory(pcat, QString(), QString(), QString(), 0);

            try {
                partFact.setDatabaseConnectionName(destDb.connectionName());

                EditCommand* insCmd = partFact.insertItemsCmd(parts.begin(), parts.end());
                insCmd->commit();
                delete insCmd;

                destDbw->adjustAutoIncrementColumn(pcat->getTableName(), pcat->getIDField());

                partFact.setDatabaseConnectionName(QString());
            } catch (std::exception& ex) {
                partFact.setDatabaseConnectionName(QString());
                throw;
            }

            progressCur++;
        }


        // ********** PASS 2: UPDATE DATA WITH ASSOCS **********

        emit progressChanged(tr("Migrating containers (pass 2/2)..."), progressCur, progressMax);

        conts = contFact.loadItems(QString(), QString(), PartContainerFactory::LoadAll);
        for (PartContainer& cont : conts) {
            PartContainer::foreachAssocTypeIndex([&](auto assocIdx) {
                cont.markAssocDirty<assocIdx>();
            });
        }

        try {
            contFact.setDatabaseConnectionName(destDb.connectionName());

            EditCommand* updCmd = contFact.updateItemsCmd(conts.begin(), conts.end());
            updCmd->commit();
            delete updCmd;

            contFact.setDatabaseConnectionName(QString());
        } catch (std::exception&) {
            contFact.setDatabaseConnectionName(QString());
            throw;
        }

        conts.clear();

        progressCur++;

        for (PartCategory* pcat : sys->getPartCategories()) {
            emit progressChanged(tr("Migrating parts for category '%1' (pass 2/2)").arg(pcat->getID()),
                                 progressCur, progressMax);

            PartList parts = partFact.loadItemsSingleCategory(pcat, QString(), QString(), QString(),
                                                              PartFactory::LoadAll);
            for (Part& part : parts) {
                Part::foreachAssocTypeIndex([&](auto assocIdx) {
                    part.markAssocDirty<assocIdx>();
                });
            }

            try {
                partFact.setDatabaseConnectionName(destDb.connectionName());

                EditCommand* updCmd = partFact.updateItemsCmd(parts.begin(), parts.end());
                updCmd->commit();
                delete updCmd;

                partFact.setDatabaseConnectionName(QString());
            } catch (std::exception& ex) {
                partFact.setDatabaseConnectionName(QString());
                throw;
            }

            progressCur++;
        }
    }

    emit progressChanged(tr("Committing changes..."), progressCur, progressMax);

    trans.commit();

    progressCur++;

    emit progressChanged(tr("Migration done!"), progressCur, progressMax);
}

void DatabaseMigrator::backupCurrentDatabase(const QString& destFile, int flags)
{
    System* sys = System::getInstance();

    DatabaseConnection* curConn = sys->getCurrentDatabaseConnection();
    if (!curConn) {
        throw NoDatabaseConnectionException("Database connection is required for backup", __FILE__, __LINE__);
    }

    zip_t* zip = zip_open(destFile.toUtf8().constData(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    if (!zip) {
        throw IOException("Error creating backup ZIP file", __FILE__, __LINE__);
    }


    int progressCur = 0;
    int progressMax = 0;

    if ((flags & BackupDatabase) != 0) {
        progressMax += 1; // Vacuum database
        progressMax += 1; // Zip database
    }
    if ((flags & BackupFiles) != 0) {
        progressMax += 1; // Package files
    }


    {
        if (zip_entry_open(zip, "edb-backup/README.txt") != 0) {
            throw IOException("Error opening backup ZIP entry", __FILE__, __LINE__);
        }

        QFile readmeFile(":/resources/README.BACKUP");
        if (!readmeFile.open(QIODevice::ReadOnly)) {
            throw IOException("Error opening backup README file", __FILE__, __LINE__);
        }
        QString readmeData = QString::fromUtf8(readmeFile.readAll())
                .arg(EDB_VERSION_STRING,
                     QDateTime::currentDateTime().toString(Qt::ISODate));
        QByteArray readmeDataUtf8 = readmeData.toUtf8();
        readmeFile.close();
        if (zip_entry_write(zip, readmeDataUtf8.constData(), readmeDataUtf8.size()) != 0) {
            throw IOException("Error writing README to backup ZIP file", __FILE__, __LINE__);
        }

        if (zip_entry_close(zip) != 0) {
            throw IOException("Error closing backup ZIP entry", __FILE__, __LINE__);
        }
    }


    if ((flags & BackupDatabase) != 0) {
        QTemporaryFile file("edb-backup-XXXXXX.db");
        if (!file.open()) {
            throw IOException("Unable to create temporary database file", __FILE__, __LINE__);
        }
        file.close();

        QString dbPath = file.fileName();

        SQLDatabaseWrapper::DatabaseType dbType;
        dbType.driverName = "QSQLITE";
        dbType.userReadableName = "SQLite";

        std::unique_ptr<SQLDatabaseWrapper> destDbw(SQLDatabaseWrapperFactory::getInstance().create(dbType.driverName));

        if (!destDbw) {
            throw SQLException("SQLite driver is required to create backups", __FILE__, __LINE__);
        }

        std::unique_ptr<SQLiteDatabaseConnection> destConn(dynamic_cast<SQLiteDatabaseConnection*>(
                destDbw->createConnection(dbType)));
        assert(destConn);

        // Not actually required for now, so anything that exists will do.
        destConn->setFileRoot(".");

        destConn->setDatabasePath(dbPath);

        std::unique_ptr<DatabaseConnection> srcConn(curConn->clone());

        doMigrateTo(srcConn.get(), destConn.get(), MigrateAll, progressCur, progressMax);

        progressCur++;

        destDbw->setDatabase(QSqlDatabase::database("migrationDestDb"));
        assert(destDbw->getDatabase().driverName() == "QSQLITE");

        emit progressChanged(tr("Optimizing backup database size..."), progressCur, progressMax);

        destDbw->execAndCheckQuery("VACUUM", "Error vacuuming backup database");

        destDbw.reset();
        QSqlDatabase::removeDatabase("migrationDestDb");

        emit progressChanged(tr("Packing backup database..."), progressCur, progressMax);

        {
            if (zip_entry_open(zip, "edb-backup/database.db") != 0) {
                throw IOException("Error opening backup ZIP entry", __FILE__, __LINE__);
            }
            if (zip_entry_fwrite(zip, dbPath.toUtf8().constData()) != 0) {
                throw IOException("Error writing database to backup ZIP file", __FILE__, __LINE__);
            }
            if (zip_entry_close(zip) != 0) {
                throw IOException("Error closing backup ZIP entry", __FILE__, __LINE__);
            }
        }

        progressCur++;
    }

    if ((flags & BackupFiles) != 0) {
        // TODO: The zip library currently doesn't support ZIP64, which might be a problem for us if the file root
        //       directory gets too large, as it limits the total ZIP file size to 4GiB. Maybe use another library?

        emit progressChanged(tr("Packaging files (this may take some time)..."), progressCur, progressMax);

        QString fileRoot = curConn->getFileRoot();

        if (QFileInfo(fileRoot).isDir()) {
            QDir fileRootDir(fileRoot);
            QDirIterator dirIt(fileRoot, QDir::Files | QDir::Hidden,
                               QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

            while (dirIt.hasNext()) {
                dirIt.next();
                QFileInfo finfo = dirIt.fileInfo();
                QString fpathRel = fileRootDir.relativeFilePath(finfo.absoluteFilePath());

                QString zipEntryPath = QString("edb-backup/fileroot/%1").arg(fpathRel);

                if (zip_entry_open(zip, zipEntryPath.toUtf8().constData()) != 0) {
                    throw IOException("Error opening backup ZIP entry", __FILE__, __LINE__);
                }
                if (zip_entry_fwrite(zip, finfo.filePath().toUtf8().constData()) != 0) {
                    throw IOException(QString("Error writing file '%1' to backup ZIP file").arg(fpathRel)
                            .toUtf8().constData(), __FILE__, __LINE__);
                }
                if (zip_entry_close(zip) != 0) {
                    throw IOException("Error closing backup ZIP entry", __FILE__, __LINE__);
                }
            }
        }

        progressCur++;
    }

    zip_close(zip);

    emit progressChanged(tr("Backup done!"), progressCur, progressMax);
}


}
