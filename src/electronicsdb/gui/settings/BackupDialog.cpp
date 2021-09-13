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

#include "BackupDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include "../../model/DatabaseMigrator.h"
#include "../../System.h"

namespace electronicsdb
{


BackupDialog::BackupDialog(QWidget* parent)
        : QDialog(parent)
{
    ui.setupUi(this);

    connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &BackupDialog::buttonBoxClicked);

    ui.warningLabel->setStyleSheet(QString("QLabel { color: %1; }")
            .arg(System::getInstance()->getAppPaletteColor(System::PaletteColorWarning).name()));
}

void BackupDialog::buttonBoxClicked(QAbstractButton* button)
{
    auto role = ui.buttonBox->buttonRole(button);

    if (    role == QDialogButtonBox::YesRole
        ||  role == QDialogButtonBox::AcceptRole
        ||  role == QDialogButtonBox::ApplyRole
    ) {
        apply();
    }
}

void BackupDialog::progressChanged(const QString& statusMsg, int progressCur, int progressMax)
{
    ui.progressBar->setMaximum(progressMax);
    ui.progressBar->setValue(progressCur);

    ui.statusLabel->setText(statusMsg);

    qApp->processEvents();
}

bool BackupDialog::apply()
{
    int flags = 0;

    if (ui.dbBox->isChecked()) {
        flags |= DatabaseMigrator::BackupDatabase;
    }
    if (ui.filesBox->isChecked()) {
        flags |= DatabaseMigrator::BackupFiles;
    }

    if (flags == 0) {
        return true;
    }

    QString filePath = QFileDialog::getSaveFileName(this, tr("Choose backup file"), QString(), tr("ZIP files (*.zip)"));
    if (filePath.isEmpty()) {
        return false;
    }

    DatabaseMigrator migrator;
    connect(&migrator, &DatabaseMigrator::progressChanged, this, &BackupDialog::progressChanged);

    try {
        migrator.backupCurrentDatabase(filePath, flags);
    } catch (std::exception& ex) {
        QMessageBox::critical(this, tr("Backup Error"),
                              tr("An error occurred during backup:\n\n%1").arg(ex.what()));
        return false;
    }

    return true;
}


}
