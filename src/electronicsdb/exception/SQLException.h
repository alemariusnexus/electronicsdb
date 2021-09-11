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

#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <nxcommon/exception/Exception.h>

namespace electronicsdb
{


class SQLException : public Exception
{
public:
    SQLException(const char* msg, const char* srcFile = nullptr, int srcLine = -1,
            const Exception* nestedException = nullptr)
            : Exception(msg, srcFile, srcLine, nestedException, "SQLException") {}
    SQLException(const QSqlQuery& query, const char* msg = "Error executing query",
            const char* srcFile = nullptr, int srcLine = -1, const Exception* nestedException = nullptr)
            : Exception(CString(msg).append(" - ").append(query.lastError().text()),
                    srcFile, srcLine, nestedException, "SQLException") {}
};


}
