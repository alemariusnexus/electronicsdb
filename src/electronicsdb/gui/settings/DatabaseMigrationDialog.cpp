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

#include "DatabaseMigrationDialog.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>
#include <nxcommon/log.h>
#include "../../model/DatabaseMigrator.h"
#include "../../System.h"

namespace electronicsdb
{


DatabaseMigrationDialog::DatabaseMigrationDialog(QWidget* parent)
        : QDialog(parent)
{
    ui.setupUi(this);

    System* sys = System::getInstance();

    connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &DatabaseMigrationDialog::buttonBoxClicked);

    ui.warnOverwriteLabel->setStyleSheet(QString("QLabel { color: %1; }")
            .arg(System::getInstance()->getAppPaletteColor(System::PaletteColorWarning).name()));

    QList<DatabaseConnection*> conns = sys->getPermanentDatabaseConnections();

    ui.srcConnCb->addItem(tr("- Select -"), -1);
    ui.destConnCb->addItem(tr("- Select -"), -1);

    for (int i = 0 ; i < conns.size() ; i++) {
        DatabaseConnection* conn = conns[i];

        ui.srcConnCb->addItem(conn->getName(), i);
        ui.destConnCb->addItem(conn->getName(), i);
    }

    ui.typeCb->addItem(tr("Static Model only"), DatabaseMigrator::MigrateStaticModel);
    ui.typeCb->addItem(tr("Everything"), DatabaseMigrator::MigrateStaticModel | DatabaseMigrator::MigrateDynamicModel);

    ui.typeCb->setCurrentIndex(ui.typeCb->count()-1);
}

void DatabaseMigrationDialog::buttonBoxClicked(QAbstractButton* button)
{
    auto role = ui.buttonBox->buttonRole(button);

    if (role == QDialogButtonBox::ApplyRole) {
        apply();
    }
}

void DatabaseMigrationDialog::progressChanged(const QString& statusMsg, int progressCur, int progressMax)
{
    ui.progressBar->setMaximum(progressMax);
    ui.progressBar->setValue(progressCur);

    ui.statusLabel->setText(statusMsg);

    qApp->processEvents();
}

bool DatabaseMigrationDialog::apply()
{
    System* sys = System::getInstance();

    QList<DatabaseConnection*> conns = sys->getPermanentDatabaseConnections();

    int srcConnIdx = ui.srcConnCb->currentData(Qt::UserRole).toInt();
    int destConnIdx = ui.destConnCb->currentData(Qt::UserRole).toInt();

    if (srcConnIdx < 0  ||  srcConnIdx >= conns.size()) {
        QMessageBox::critical(this, tr("Invalid Database"), tr("The selected source database is invalid!"));
        return false;
    }
    if (destConnIdx < 0  ||  destConnIdx >= conns.size()) {
        QMessageBox::critical(this, tr("Invalid Database"), tr("The selected destination database is invalid!"));
        return false;
    }

    DatabaseConnection* srcConn = conns[srcConnIdx];
    DatabaseConnection* destConn = conns[destConnIdx];

    if (srcConn == destConn) {
        QMessageBox::critical(this, tr("Invalid Database"), tr("Source and destination databases must be different!"));
        return false;
    }

    auto res = QMessageBox::question(this, tr("Confirm Migration"),
                          tr("Are you sure you want to <b>overwrite all data in '%1'</b>? This action can not be "
                             "undone.").arg(destConn->getDescription()));
    if (res != QMessageBox::Yes) {
        return false;
    }

    int migFlags = ui.typeCb->currentData(Qt::UserRole).toInt();

    ui.settingsContainerWidget->setEnabled(false);
    ui.buttonBox->setEnabled(false);

    DatabaseMigrator migrator;
    connect(&migrator, &DatabaseMigrator::progressChanged, this, &DatabaseMigrationDialog::progressChanged);

    try {
        migrator.migrateTo(srcConn, destConn, migFlags);
    } catch (std::exception& ex) {
        QMessageBox::critical(this, tr("Migration Error"),
                              tr("An error occurred during database migration:\n\n%1").arg(ex.what()));
        ui.settingsContainerWidget->setEnabled(true);
        return false;
    }

    ui.settingsContainerWidget->setEnabled(true);
    ui.buttonBox->setEnabled(true);

    return true;
}


}
