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
#include <QSqlQuery>
#include <QString>
#include <QVariant>

namespace electronicsdb
{


class Filter
{
public:
    Filter();

    void setSQLWhereCode(const QString& whereCode, const QList<QVariant>& bindParamVals = {});
    void setSQLOrderCode(const QString& orderCode);
    void setFullTextQuery(const QString& query);

    QString getSQLWhereCode() const { return sqlWhereCode; }
    const QList<QVariant>& getSQLWhereCodeBindParamValues() const { return sqlWhereCodeBindParamValues; }
    QString getSQLOrderCode() const { return sqlOrderCode; }
    QString getFullTextQuery() const { return ftQuery; }

    int fillSQLWhereCodeBindParams(QSqlQuery& query, int paramOffs = 0);

private:
    QString sqlWhereCode;
    QString sqlOrderCode;
    QString ftQuery;

    QList<QVariant> sqlWhereCodeBindParamValues;
};


}
