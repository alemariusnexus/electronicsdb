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

#include <QList>
#include <QString>
#include "sql/SQLCommand.h"
#include "EditCommand.h"

namespace electronicsdb
{


class SQLEditCommand : public EditCommand
{
public:
    SQLEditCommand();
    ~SQLEditCommand();

    void addSQLCommand(SQLCommand* cmd);

    void setDescription(const QString& desc) { this->desc = desc; }
    QString getDescription() const override { return desc; }

    bool wantsSQLTransaction() override { return true; }
    QSqlDatabase getSQLDatabase() const override;

    void commit() override;
    void revert() override;

private:
    QString desc;
    QList<SQLCommand*> cmds;
};


}
