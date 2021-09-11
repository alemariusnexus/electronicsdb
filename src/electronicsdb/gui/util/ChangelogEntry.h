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

#include "../../global.h"

#include <QList>
#include <QObject>
#include <QString>

namespace electronicsdb
{


class ChangelogEntry : public QObject
{
    Q_OBJECT

public:
    ChangelogEntry();
    virtual ~ChangelogEntry();

    virtual void update();

    virtual QStringList getErrorMessages() const;
    void addErrorMessage(const QString& errmsg);
    void setErrorMessages(const QStringList& errmsgs);
    void clearErrorMessages();

    virtual void undo() = 0;
    virtual QString getEntryDescription() const = 0;
    virtual QString getChangeDescription() const = 0;

signals:
    void updated();

private:
    QStringList errmsgs;
};


}
