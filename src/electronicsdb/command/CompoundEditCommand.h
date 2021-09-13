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
#include "EditCommand.h"

namespace electronicsdb
{


class CompoundEditCommand : public EditCommand
{
public:
    virtual ~CompoundEditCommand();

    virtual QString getDescription() const;

    bool wantsSQLTransaction() override;
    QSqlDatabase getSQLDatabase() const override;

    virtual void commit();
    virtual void revert();

    void setDescription(const QString& desc);
    void addCommand(EditCommand* cmd);

private:
    QString desc;
    QList<EditCommand*> cmds;
};


}
