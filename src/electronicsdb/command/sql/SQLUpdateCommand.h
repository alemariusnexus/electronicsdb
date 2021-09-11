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

#include <QMap>
#include <QString>
#include <QVariant>
#include "SQLCommand.h"

namespace electronicsdb
{


class SQLUpdateCommand : public SQLCommand
{
public:
    using DataMap = QMap<QString, QVariant>;

public:
    SQLUpdateCommand(const QString& tableName, const QString& idField, dbid_t id);
    void setValues(const DataMap& data);
    void addFieldValue(const QString& fieldName, const QVariant& newValue);

protected:
    void doCommit() override;
    void doRevert() override;

private:
    QString tableName;
    QString idField;
    dbid_t id;
    DataMap values;

    QString revertQuery;
    QList<QVariant> revertBindParamValues;
    DataMap revertListenData;
};


}
