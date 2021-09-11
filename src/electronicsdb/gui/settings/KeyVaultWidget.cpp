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

#include "KeyVaultWidget.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include <nxcommon/log.h>
#include "../../exception/InvalidCredentialsException.h"
#include "../../util/KeyVault.h"
#include "../../System.h"

namespace electronicsdb
{


KeyVaultWidget::KeyVaultWidget(QWidget* parent)
        : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.vaultEnableBox, &QCheckBox::stateChanged, this, &KeyVaultWidget::updateState);
    connect(ui.pwEnableBox, &QCheckBox::stateChanged, this, &KeyVaultWidget::updateState);
    connect(ui.clearButton, &QPushButton::clicked, this, &KeyVaultWidget::clearVaultRequested);

    ui.noPwWarningLabel->setStyleSheet(QString("QLabel { color: %1 }")
            .arg(System::getInstance()->getAppPaletteColor(System::PaletteColorWarning).name()));
    ui.removePwWarningLabel->setStyleSheet(QString("QLabel { color: %1 }")
            .arg(System::getInstance()->getAppPaletteColor(System::PaletteColorWarning).name()));

    ui.noPwWarningLabel->setVisible(false);
    ui.removePwWarningLabel->setVisible(false);

    load();
}

KeyVaultWidget::ApplyState KeyVaultWidget::apply()
{
    return save();
}

void KeyVaultWidget::load()
{
    KeyVault& vault = KeyVault::getInstance();

    if (vault.isSetUp()) {
        if (vault.isEnabled()) {
            ui.vaultEnableBox->setChecked(true);
        } else {
            ui.vaultEnableBox->setChecked(false);
        }
    } else {
        ui.vaultEnableBox->setChecked(true);
    }

    ui.pwEdit->clear();
    ui.pwRepeatEdit->clear();

    updateState();
}

KeyVaultWidget::ApplyState KeyVaultWidget::save()
{
    KeyVault& vault = KeyVault::getInstance();

    if (ui.vaultEnableBox->isChecked()) {
        if (ui.pwEnableBox->isChecked()) {
            if (ui.pwEdit->text() != ui.pwRepeatEdit->text()) {
                QMessageBox::warning(this, tr("Password Mismatch"), tr("The passwords do not match!"));
                return Unfinished;
            }
            if (ui.pwEdit->text().isEmpty()) {
                // These are the widget's default settings. An empty password would be insecure, so we reject it and
                // simply take this to mean "don't change anything". That's also useful to allow this widget to pop
                // up again the first (or next) time the user accesses the vault, as a kind of (initial) setup.
                return Rejected;
            }
            QString newPw = ui.pwEdit->text();
            if (!newPw.isEmpty()) {
                try {
                    if (!vault.setVaultPasswordWithUserPrompt(ui.pwEdit->text(), this)) {
                        return Rejected;
                    }
                } catch (InvalidCredentialsException&) {
                    QThread::msleep(1000);
                    return save();
                }
            }
        } else {
            try {
                if (!vault.setVaultPasswordWithUserPrompt(QString(), this)) {
                    return Rejected;
                }
            } catch (InvalidCredentialsException&) {
                QThread::msleep(1000);
                return save();
            }
        }
    } else {
        vault.clear(true);
    }

    vault.setEnabled(ui.vaultEnableBox->isChecked());

    return Applied;
}

void KeyVaultWidget::updateState()
{
    KeyVault& vault = KeyVault::getInstance();

    if (ui.vaultEnableBox->isChecked()) {
        ui.protGroupBox->setEnabled(true);

        if (vault.isVaultKeyed()) {
            if (vault.isVaultDefaultKeyed()) {
                ui.pwEdit->setPlaceholderText(QString());
                ui.pwRepeatEdit->setPlaceholderText(QString());
            } else {
                QString keepOldPwPlaceholder = tr("********");
                ui.pwEdit->setPlaceholderText(keepOldPwPlaceholder);
                ui.pwRepeatEdit->setPlaceholderText(keepOldPwPlaceholder);
            }
        } else {
            ui.pwEdit->setPlaceholderText(QString());
            ui.pwRepeatEdit->setPlaceholderText(QString());
        }

        ui.noPwWarningLabel->setVisible(!ui.pwEnableBox->isChecked());
        ui.removePwWarningLabel->setVisible(false);
    } else {
        ui.protGroupBox->setEnabled(false);

        ui.noPwWarningLabel->setVisible(false);
        if (vault.isVaultKeyed()  &&  vault.getKeyCount() > 0) {
            ui.removePwWarningLabel->setVisible(true);
        } else {
            ui.removePwWarningLabel->setVisible(false);
        }
    }

    ui.pwEdit->setEnabled(ui.pwEnableBox->isChecked());
    ui.pwRepeatEdit->setEnabled(ui.pwEnableBox->isChecked());
}

void KeyVaultWidget::clearVaultRequested()
{
    auto res = QMessageBox::question(this, tr("Clear Key Vault?"),
            tr("Do you really want to clear the key vault? This will remove all stored passwords and reset "
               "the vault key."));
    if (res != QMessageBox::Yes) {
        return;
    }

    KeyVault& vault = KeyVault::getInstance();

    vault.clear(true);
}


}
