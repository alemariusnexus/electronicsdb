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

#include "PropertyFileWidget.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>
#include "../../db/DatabaseConnection.h"
#include "../../System.h"

namespace electronicsdb
{


PropertyFileWidget::PropertyFileWidget(PartProperty* prop, QWidget* parent, QObject* keyEventFilter)
        : QWidget(parent), cat(prop->getPartCategory()), prop(prop), displayState(Enabled), displayFlags(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(topLayout);

    QWidget* fileLineWidget = new QWidget(this);
    QHBoxLayout* fileLineLayout = new QHBoxLayout(fileLineWidget);
    fileLineLayout->setContentsMargins(0, 0, 0, 0);
    fileLineWidget->setLayout(fileLineLayout);
    topLayout->addWidget(fileLineWidget);

    fileEdit = new PlainLineEdit(fileLineWidget);
    fileEdit->setMaxLength(MAX_FILE_PROPERTY_PATH_LEN);
    fileEdit->installEventFilter(keyEventFilter);
    connect(fileEdit, &PlainLineEdit::textEdited, this, &PropertyFileWidget::textEdited);
    fileLineLayout->addWidget(fileEdit);

    chooseButton = new QPushButton(tr("..."), fileLineWidget);
    connect(chooseButton, &QPushButton::clicked, this, &PropertyFileWidget::fileChosen);
    fileLineLayout->addWidget(chooseButton);

    QWidget* buttonWidget = new QWidget(this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonWidget->setLayout(buttonLayout);
    topLayout->addWidget(buttonWidget);

    openButton = new QPushButton(QIcon::fromTheme("document-open", QIcon(":/icons/document-open.png")),
            tr("Open"), buttonWidget);
    connect(openButton, &QPushButton::clicked, this, &PropertyFileWidget::openRequested);
    buttonLayout->addWidget(openButton);

    clipboardButton = new QPushButton(QIcon::fromTheme("edit-copy", QIcon(":/icons/edit-copy.png")),
            tr("Copy Path To Clipboard"), buttonWidget);
    connect(clipboardButton, &QPushButton::clicked, this, &PropertyFileWidget::copyClipboardRequested);
    buttonLayout->addWidget(clipboardButton);

    buttonLayout->addStretch(1);
}


void PropertyFileWidget::displayFile(const Part& part, const QString& fileName)
{
    currentPart = part;

    if (!part) {
        fileEdit->setText(tr("(Invalid)"));
    } else {
        fileEdit->setText(fileName);
    }
}


void PropertyFileWidget::setState(DisplayWidgetState state)
{
    displayState = state;

    applyState();
}


void PropertyFileWidget::setFlags(flags_t flags)
{
    displayFlags = flags;

    applyState();
}


void PropertyFileWidget::applyState()
{
    DisplayWidgetState state = displayState;

    if ((displayFlags & ChoosePart)  !=  0) {
        state = ReadOnly;
    }

    if (!currentPart) {
        state = Disabled;
    }

    if (state == Enabled) {
        fileEdit->setEnabled(true);
        fileEdit->setReadOnly(false);

        chooseButton->setEnabled(true);
        openButton->setEnabled(true);
        clipboardButton->setEnabled(true);
    } else if (state == Disabled) {
        fileEdit->setEnabled(false);

        chooseButton->setEnabled(false);
        openButton->setEnabled(false);
        clipboardButton->setEnabled(false);
    } else if (state == ReadOnly) {
        fileEdit->setEnabled(true);
        fileEdit->setReadOnly(true);

        chooseButton->setEnabled(false);
        openButton->setEnabled(true);
        clipboardButton->setEnabled(true);
    }
}


void PropertyFileWidget::setValueFocus()
{
    fileEdit->setFocus();
    fileEdit->selectAll();
}


QString PropertyFileWidget::getValue() const
{
    return fileEdit->text();
}


void PropertyFileWidget::fileChosen()
{
    System* sys = System::getInstance();

    DatabaseConnection* conn = sys->getCurrentDatabaseConnection();

    QString rootPath = conn->getFileRoot();

    QString fpath = QFileDialog::getOpenFileName(this, tr("Choose A File"), rootPath);

    if (fpath.isNull())
        return;

    fileEdit->setText(fpath);

    emit changedByUser();
}


void PropertyFileWidget::textEdited(const QString& text)
{
    emit changedByUser();
}


QString PropertyFileWidget::getCurrentAbsolutePath()
{
    return System::getInstance()->getRelativeFileAbsolutePath(fileEdit->text(), this);
}


void PropertyFileWidget::openRequested()
{
    System::getInstance()->openRelativeFile(fileEdit->text(), this);
}


void PropertyFileWidget::copyClipboardRequested()
{
    QString fpath = getCurrentAbsolutePath();
    qApp->clipboard()->setText(fpath);
}


}
