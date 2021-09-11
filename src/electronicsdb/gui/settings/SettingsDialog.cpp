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

#include "SettingsDialog.h"

#include <cstdio>
#include <QFileDialog>
#include <QSettings>
#include <QTimer>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../model/PartCategory.h"
#include "../../System.h"

namespace electronicsdb
{


SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent), curEdConn(nullptr)
{
    ui.setupUi(this);

    System* sys = System::getInstance();

    for (DatabaseConnection* conn : sys->getPermanentDatabaseConnections()) {
        LocalPermDbConn* lconn = new LocalPermDbConn;
        lconn->globalConn = conn;
        lconn->localConn = conn->clone();

        localPermDbConns.push_back(lconn);

        ui.connList->addItem(conn->getName());
    }


    connect(ui.connList, &QListWidget::currentRowChanged, this, &SettingsDialog::connListRowChanged);
    connect(ui.connAddButton, &QPushButton::clicked, this, &SettingsDialog::connAddRequested);
    connect(ui.connRemoveButton, &QPushButton::clicked, this, &SettingsDialog::connRemoveRequested);

    connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &SettingsDialog::buttonBoxClicked);

    connect(ui.connEditor, &ConnectionEditWidget::connectionNameChanged, this, &SettingsDialog::connNameChanged);

    ui.connAddButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    ui.connRemoveButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));

    if (localPermDbConns.empty()) {
        ui.connEditor->setEnabled(false);
    } else {
        ui.connList->setCurrentRow(0);
    }

    rebuildStartupConnectionBox();


    QSettings s;

    DatabaseConnection* startupConn = sys->getStartupDatabaseConnection();

    if (startupConn) {
        int i = 0;
        for (LocalPermDbConn* lconn : localPermDbConns) {
            if (lconn->globalConn  &&  lconn->globalConn->getID() == startupConn->getID()) {
                ui.connStartupBox->setCurrentIndex(i+1);
                break;
            }
            i++;
        }
    }

    QString theme = s.value("gui/theme", "default").toString();

    ui.themeCb->addItem(tr("Default"), "default");
    ui.themeCb->addItem(tr("Dark (experimental)"), "dark");

    for (int i = 0 ; i < ui.themeCb->count() ; i++) {
        if (ui.themeCb->itemData(i, Qt::UserRole).toString() == theme) {
            ui.themeCb->setCurrentIndex(i);
            break;
        }
    }

    ui.siBinPrefixDefaultBox->setChecked(s.value("main/si_binary_prefixes_default", true).toBool());

    s.beginGroup("gui/settings_dialog");
    restoreGeometry(s.value("geometry").toByteArray());
    s.endGroup();
}

SettingsDialog::~SettingsDialog()
{
    for (LocalPermDbConn* lconn : localPermDbConns) {
        delete lconn->localConn;
        delete lconn;
    }
}

void SettingsDialog::closeEvent(QCloseEvent* evt)
{
    QSettings s;

    s.beginGroup("gui/settings_dialog");
    s.setValue("geometry", saveGeometry());
    s.endGroup();

    s.sync();
}

void SettingsDialog::applyCurrentConnectionChanges()
{
    if (curEdConn) {
        delete curEdConn->localConn;
        curEdConn->localConn = ui.connEditor->createConnection();

        ui.connList->item(localPermDbConns.indexOf(curEdConn))->setText(curEdConn->localConn->getName());

        rebuildStartupConnectionBox();
    }
}

void SettingsDialog::connListRowChanged(int row)
{
    System* sys = System::getInstance();

    if (curEdConn) {
        applyCurrentConnectionChanges();
    }

    if (row != -1  &&  row < localPermDbConns.size()) {
        LocalPermDbConn* lconn = localPermDbConns[row];
        DatabaseConnection* conn = lconn->localConn;

        curEdConn = lconn;

        ui.connEditor->display(conn);
        ui.connEditor->setEnabled(true);

        ui.connRemoveButton->setEnabled(true);
    } else {
        curEdConn = nullptr;

        ui.connEditor->clear();
        ui.connEditor->setEnabled(false);

        ui.connRemoveButton->setEnabled(false);
    }
}

void SettingsDialog::connAddRequested()
{
    applyCurrentConnectionChanges();

    curEdConn = nullptr;

    ui.connEditor->clear();

    LocalPermDbConn* lconn = new LocalPermDbConn;
    lconn->globalConn = nullptr;
    lconn->localConn = nullptr;

    localPermDbConns.push_back(lconn);

    ui.connList->addItem(ui.connEditor->getConnectionName());
    ui.connList->setCurrentRow(ui.connList->count()-1);

    rebuildStartupConnectionBox();
}

void SettingsDialog::connRemoveRequested()
{
    if (curEdConn) {
        LocalPermDbConn* lconn = curEdConn;

        curEdConn = nullptr;

        int crow = ui.connList->currentRow();

        if (crow == ui.connList->count()-1) {
            ui.connList->setCurrentRow(crow-1);
        } else {
            ui.connList->setCurrentRow(crow+1);
        }

        localPermDbConns.removeOne(lconn);

        delete lconn->localConn;
        delete lconn;

        ui.connList->takeItem(crow);

        rebuildStartupConnectionBox();
    }
}

void SettingsDialog::buttonBoxClicked(QAbstractButton* b)
{
    if (ui.buttonBox->standardButton(b) == QDialogButtonBox::Apply) {
        apply();
    } else {
        reject();
    }
}

void SettingsDialog::apply()
{
    QSettings s;

    auto kvApplyState = ui.keyVaultWidget->apply();
    if (kvApplyState == KeyVaultWidget::Unfinished) {
        return;
    }

    if (!applyDatabaseSettings()) {
        return;
    }

    s.setValue("gui/theme", ui.themeCb->currentData(Qt::UserRole).toString());
    s.setValue("main/si_binary_prefixes_default", ui.siBinPrefixDefaultBox->isChecked());

    accept();
}

bool SettingsDialog::applyDatabaseSettings()
{
    System* sys = System::getInstance();

    applyCurrentConnectionChanges();

    QString selConnID = ui.connStartupBox->itemData(ui.connStartupBox->currentIndex(), Qt::UserRole).toString();
    LocalPermDbConn* selConn = nullptr;

    if (!selConnID.isNull()) {
        for (LocalPermDbConn* lconn : localPermDbConns) {
            if (lconn->localConn  &&  lconn->localConn->getID() == selConnID) {
                selConn = lconn;
                break;
            }
        }
    }

    if (!selConn) {
        sys->setStartupDatabaseConnection(nullptr);
    }


    LocalPermDbConnList conns = localPermDbConns;

    //System::DatabaseConnectionIterator it;
    LocalPermDbConnList::iterator lit;

    System::DatabaseConnectionList removedConns;

    for (DatabaseConnection* conn : sys->getPermanentDatabaseConnections()) {
        bool found = false;

        // Check if the connection was registered in the system before
        for (lit = conns.begin() ; lit != conns.end() ; lit++) {
            LocalPermDbConn* lconn = *lit;

            if (lconn->globalConn == conn) {
                // Connection changed (or still the same)

                DatabaseConnection* newPermConn = lconn->localConn->clone();
                sys->replacePermanentDatabaseConnection(conn, newPermConn);

                if (selConn == lconn) {
                    sys->setStartupDatabaseConnection(newPermConn);
                }

                conns.erase(lit);
                found = true;

                break;
            }
        }

        if (!found) {
            // Connection removed

            removedConns << conn;
        }
    }

    for (DatabaseConnection* conn : removedConns) {
        sys->removePermanentDatabaseConnection(conn);
    }

    for (lit = conns.begin() ; lit != conns.end() ; lit++) {
        LocalPermDbConn* lconn = *lit;

        // Connection added
        DatabaseConnection* newConn = lconn->localConn->clone();

        sys->addPermanentDatabaseConnection(newConn);

        if (selConn == lconn) {
            sys->setStartupDatabaseConnection(newConn);
        }
    }

    sys->savePermanentDatabaseConnectionSettings();

    return true;
}

void SettingsDialog::rebuildStartupConnectionBox()
{
    QString selConnID = ui.connStartupBox->itemData(ui.connStartupBox->currentIndex(), Qt::UserRole).toString();
    LocalPermDbConn* selConn = nullptr;

    if (!selConnID.isNull()) {
        for (LocalPermDbConn* lconn : localPermDbConns) {
            if (lconn->localConn  &&  lconn->localConn->getID() == selConnID) {
                selConn = lconn;
                break;
            }
        }
    }

    ui.connStartupBox->clear();

    ui.connStartupBox->addItem(tr("(None)"), QVariant());

    int selIdx = 0;
    for (LocalPermDbConn* lconn : localPermDbConns) {
        if (lconn->localConn) {
            ui.connStartupBox->addItem(lconn->localConn->getName(), lconn->localConn->getID());

            if (lconn == selConn) {
                selIdx = ui.connStartupBox->count()-1;
            }
        }
    }

    ui.connStartupBox->setCurrentIndex(selIdx);
}

void SettingsDialog::connNameChanged(const QString& name)
{
    ui.connList->currentItem()->setText(name);
}


}
