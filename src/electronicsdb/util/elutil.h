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

#include <stdint.h>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>
#include <nxcommon/util.h>

namespace electronicsdb
{


enum SuffixFormat
{
    SuffixFormatSIBase10,
    SuffixFormatSIBase2,
    SuffixFormatPercent,
    SuffixFormatPPM,
    SuffixFormatNone
};



int64_t ResolveIntSuffices(const QString& str, bool* ok = nullptr, bool siPrefixesAreBase2 = false);
double ResolveFloatSuffices(const QString& str, bool* ok = nullptr, bool siPrefixesAreBase2 = false);

QString FormatFloatSuffices(double val, SuffixFormat format = SuffixFormatSIBase10, int precision = 3);
QString FormatIntSuffices(int64_t val, SuffixFormat format = SuffixFormatSIBase10, int precision = 3);

inline dbid_t VariantToDBID(const QVariant& v)
{
    if constexpr(sizeof(dbid_t) <= sizeof(int)) {
        return static_cast<dbid_t>(v.toInt());
    } else {
        return static_cast<dbid_t>(v.toLongLong());
    }
}

QString IndentString(const QString& str, const QString& ind);

bool ParseVersionNumber(const QString& versionStr, int* major, int* minor, int* patch);

QString ReplaceStringVariables(const QString& str, const QMap<QString, QString>& vars,
        QSet<QString>* outUnmatchedVars = nullptr);


}
