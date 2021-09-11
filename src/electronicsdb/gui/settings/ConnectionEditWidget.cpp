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

#include "ConnectionEditWidget.h"

#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include "../../db/SQLDatabaseWrapperFactory.h"
#include "../../System.h"

namespace electronicsdb
{


ConnectionEditWidget::ConnectionEditWidget(QWidget* parent)
        : QWidget(parent)
{
    ui.setupUi(this);

    ui.ddlWarnLabel->setStyleSheet(QString("QLabel { color: %1; }")
            .arg(System::getInstance()->getAppPaletteColor(System::PaletteColorWarning).name()));

    QList<SQLDatabaseWrapper*> wrappers = SQLDatabaseWrapperFactory::getInstance().createAll();

    for (SQLDatabaseWrapper* dbw : wrappers) {
        auto dbTypes = dbw->getSupportedDatabaseTypes();
        for (const auto& type : dbTypes) {
            DatabaseConnectionWidget* cw = dbw->createWidget(type, ui.connStackWidget);
            ui.connStackWidget->addWidget(cw);

            WrappedDatabaseType& wdbType = dbTypesByDriver[type.driverName];
            wdbType.wrapper = dbw;
            wdbType.type = type;
            wdbType.connWidget = cw;
            wdbType.showDDLWarning = !dbw->supportsTransactionalDDL();
            ui.connTypeBox->addItem(type.userReadableName, type.driverName);
        }
    }

    connect(ui.connTypeBox, SIGNAL(activated(int)), this, SLOT(connTypeBoxActivated(int)));
    connect(ui.connFileRootChooseButton, &QPushButton::clicked,
            this, &ConnectionEditWidget::connFileRootPathChooseButtonClicked);

    connect(ui.connNameField, &QLineEdit::textEdited, this, &ConnectionEditWidget::connectionNameChanged);

    SQLDatabaseWrapper* defWrapper = SQLDatabaseWrapperFactory::getInstance().createDefault();
    setConnectionDriverName(defWrapper->getSupportedDatabaseTypes()[0].driverName);
    delete defWrapper;

    updateState();
}


ConnectionEditWidget::~ConnectionEditWidget()
{
    for (auto it = dbTypesByDriver.begin() ; it != dbTypesByDriver.end() ; it++) {
        WrappedDatabaseType& wdbType = it.value();
        delete wdbType.wrapper;
    }
}


void ConnectionEditWidget::setConnectionName(const QString& name)
{
    ui.connNameField->setText(name);
}


void ConnectionEditWidget::setConnectionDriverName(const QString& driverName)
{
    int index = ui.connTypeBox->findData(driverName);
    ui.connTypeBox->setCurrentIndex(index);

    WrappedDatabaseType& wdbType = dbTypesByDriver[driverName];
    ui.connStackWidget->setCurrentWidget(wdbType.connWidget);

    updateState();
}


void ConnectionEditWidget::setFileRootPath(const QString& path)
{
    ui.connFileRootField->setText(path);
}


void ConnectionEditWidget::updateState()
{
    int idx = ui.connTypeBox->currentIndex();

    if (idx >= 0) {
        QString driverName = ui.connTypeBox->itemData(idx, Qt::UserRole).toString();
        WrappedDatabaseType& wdbType = dbTypesByDriver[driverName];

        ui.ddlWarnLabel->setVisible(wdbType.showDDLWarning);
    } else {
        ui.ddlWarnLabel->setVisible(false);
    }
}


void ConnectionEditWidget::connTypeBoxActivated(int idx)
{
    if (idx == -1)
        return;

    QString driverName = ui.connTypeBox->itemData(idx, Qt::UserRole).toString();
    WrappedDatabaseType& wdbType = dbTypesByDriver[driverName];
    ui.connStackWidget->setCurrentWidget(wdbType.connWidget);

    updateState();
}


void ConnectionEditWidget::connFileRootPathChooseButtonClicked()
{
    QString fpath = QFileDialog::getExistingDirectory(this, tr("Choose a root directory"), ui.connFileRootField->text());

    if (fpath.isNull())
        return;

    setFileRootPath(fpath);
}


void ConnectionEditWidget::changeEvent(QEvent* evt)
{
    if (evt->type() == QEvent::EnabledChange) {
        bool enabled = isEnabled();

        ui.connNameField->setEnabled(enabled);
        ui.connTypeBox->setEnabled(enabled);

        ui.connFileRootField->setEnabled(enabled);
        ui.connFileRootChooseButton->setEnabled(enabled);
    }
}


void ConnectionEditWidget::display(DatabaseConnection* conn)
{
    if (!conn) {
        clear();
        return;
    }

    WrappedDatabaseType& wdbType = dbTypesByDriver[conn->getDatabaseType().driverName];

    wdbType.connWidget->display(conn);

    setConnectionName(conn->getName());
    setFileRootPath(conn->getFileRoot());
    setConnectionDriverName(conn->getDatabaseType().driverName);

    updateState();
}


void ConnectionEditWidget::clear()
{
    setConnectionName("New connection");
    setFileRootPath("");

    for (auto it = dbTypesByDriver.begin() ; it != dbTypesByDriver.end() ; it++) {
        WrappedDatabaseType& wdbType = it.value();
        wdbType.connWidget->clear();
    }

    updateState();
}

DatabaseConnection* ConnectionEditWidget::createConnection()
{
    WrappedDatabaseType& wdbType = dbTypesByDriver[getConnectionDriverName()];

    DatabaseConnection* conn = wdbType.connWidget->createConnection();
    conn->setName(ui.connNameField->text());
    conn->setFileRoot(ui.connFileRootField->text());

    return conn;
}


}
