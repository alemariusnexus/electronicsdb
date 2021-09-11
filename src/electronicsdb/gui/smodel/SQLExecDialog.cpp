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

#include "SQLExecDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include "../../System.h"

namespace electronicsdb
{


SQLExecDialog::SQLExecDialog(QWidget* parent)
        : QDialog(parent)
{
    ui.setupUi(this);

    ui.saveButton->setIcon(QIcon::fromTheme("document-save", QIcon(":/icons/document-save.png")));
    ui.copyClipboardButton->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/icons/edit-copy.png")));

    highlighter = new QSourceHighlite::QSourceHighliter(ui.codeEdit->document());
    highlighter->setCurrentLanguage(QSourceHighlite::QSourceHighliter::CodeSQL);

    ui.warningLabel->setStyleSheet(QString("QLabel { color: %1 }")
            .arg(System::getInstance()->getAppPaletteColor(System::PaletteColorWarning).name()));

    connect(ui.saveButton, &QPushButton::clicked, this, &SQLExecDialog::saveRequested);
    connect(ui.copyClipboardButton, &QPushButton::clicked, this, &SQLExecDialog::copyClipboardRequested);

    connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &SQLExecDialog::buttonBoxClicked);
}

SQLExecDialog::~SQLExecDialog()
{
    delete highlighter;
}

void SQLExecDialog::setSQLCode(const QString& code)
{
    ui.codeEdit->setText(code);
}

void SQLExecDialog::buttonBoxClicked(QAbstractButton* button)
{
    auto role = ui.buttonBox->buttonRole(button);

    if (    role == QDialogButtonBox::ApplyRole
        ||  role == QDialogButtonBox::AcceptRole
        ||  role == QDialogButtonBox::YesRole
    ) {
        accept();
    } else if ( role == QDialogButtonBox::RejectRole
            ||  role == QDialogButtonBox::NoRole
    ) {
        reject();
    }
}

void SQLExecDialog::saveRequested()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Select Destination File"), QString(),
                                                "SQL Scripts (*.sql)");
    if (path.isNull()) {
        return;
    }

    QFile f(path);

    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error Saving File"), tr("Error saving SQL code to file: %1")
                .arg(f.errorString()));
        return;
    }

    QByteArray u8Code = ui.codeEdit->toPlainText().toUtf8();
    f.write(u8Code);
}

void SQLExecDialog::copyClipboardRequested()
{
    qApp->clipboard()->setText(ui.codeEdit->toPlainText());
}


}
