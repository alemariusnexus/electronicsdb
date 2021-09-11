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

#include "elutil.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <QRegularExpression>
#include <nxcommon/log.h>

namespace electronicsdb
{


double ResolveFloatSuffices(const QString& str, bool* ok, bool siPrefixesAreBase2)
{
    int suffixIdx = str.length();

    bool eFound = false;
    for (int i = 0 ; i < str.length() ; i++) {
        QChar c = str[i];

        if (!c.isDigit()  &&  c != '.'  &&  c != '+'  &&  c != '-'  &&  ((c != 'e'  &&  c != 'E')  ||  eFound)) {
            suffixIdx = i;
            break;
        } else if (c == 'e'  ||  c == 'E') {
            eFound = true;
        }
    }

    double factor = 1.0;

    if (suffixIdx != str.length()) {
        QChar c = str[suffixIdx];

        switch (c.toLatin1()) {
        case 'm': factor = 1.0e-3; break;
        case 'u': factor = 1.0e-6; break;
        case 'n': factor = 1.0e-9; break;
        case 'f': factor = 1.0e-15; break;

        case 'P':
        case 'p':
            if (suffixIdx+2 < str.length()  &&  str[suffixIdx+1].toLower() == 'p'  &&  str[suffixIdx+2].toLower() == 'm') {
                factor = 1.0e-6;
            } else if (c == 'p') {
                factor = 1.0e-12;
            } else if (c == 'P') {
                if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                    factor = pow(1024, 5);
                else
                    factor = 1.0e15;
            }
            break;

        case 'c': factor = 1.0e-2; break;
        case 'd': factor = 1.0e-1; break;

        case 'k': case 'K':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = 1024;
            else
                factor = 1.0e3;
            break;
        case 'M':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = pow(1024, 2);
            else
                factor = 1.0e6;
            break;
        case 'G':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = pow(1024, 3);
            else
                factor = 1.0e9;
            break;
        case 'T':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = pow(1024, 4);
            else
                factor = 1.0e12;
            break;

        case '%':
            factor = 1.0e-2;
            break;
        }
    }

    QString strCpy = str;
    strCpy.truncate(suffixIdx);
    return strCpy.toDouble(ok) * factor;
}

int64_t ResolveIntSuffices(const QString& str, bool* ok, bool siPrefixesAreBase2)
{
    int suffixIdx = str.length();

    bool eFound = false;
    for (int i = 0 ; i < str.length() ; i++) {
        QChar c = str[i];

        if (!c.isDigit()  &&  c != '+'  &&  c != '-'  &&  ((c != 'e'  &&  c != 'E')  ||  eFound)) {
            suffixIdx = i;
            break;
        } else if (c == 'e'  ||  c == 'E') {
            eFound = true;
        }
    }

    int64_t factor = 1;

    if (suffixIdx != str.length()) {
        QChar c = str[suffixIdx];

        switch (c.toLatin1()) {
        case 'k': case 'K':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = 1024;
            else
                factor = 1e3;
            break;
        case 'M':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = pow(1024, 2);
            else
                factor = 1e6;
            break;
        case 'G':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = pow(1024, 3);
            else
                factor = 1e9;
            break;
        case 'T':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = pow(1024, 4);
            else
                factor = 1e12;
            break;

        case 'P':
            if (siPrefixesAreBase2  ||  (suffixIdx+1 != str.length()  &&  str[suffixIdx+1] == 'i'))
                factor = pow(1024, 5);
            else
                factor = 1e15;
            break;;
        }
    }

    QString strCpy = str;
    strCpy.truncate(suffixIdx);

    if constexpr (sizeof(long) >= sizeof(int64_t))
        return strCpy.toLong(ok) * factor;
    else
        return strCpy.toLongLong(ok) * factor;
}

QString DetermineSuffix(double val, double& factor, SuffixFormat format)
{
    if (format == SuffixFormatNone  ||  val == 0.0) {
        factor = 1.0;
        if (format == SuffixFormatPercent) {
            return "%";
        } else if (format == SuffixFormatPPM) {
            return "ppm";
        } else {
            return "";
        }
    }

    double aval = std::abs(val);

    if (format == SuffixFormatSIBase2) {
        if (aval < 1024) {
            factor = 1;
            return "";
        } else if (aval < pow(1024, 2)) {
            factor = 1.0 / 1024;
            return "Ki";
        } else if (aval < pow(1024, 3)) {
            factor = 1.0 / pow(1024, 2);
            return "Mi";
        } else if (aval < pow(1024, 4)) {
            factor = 1.0 / pow(1024, 3);
            return "Gi";
        } else if (aval < pow(1024, 5)) {
            factor = 1.0 / pow(1024, 4);
            return "Ti";
        } else {
            factor = 1.0 / pow(1024, 5);
            return "Pi";
        }
    } else if (format == SuffixFormatSIBase10) {
        int magnitude = (int) floor(log10(aval));

        if (magnitude < -12) {
            factor = 1e15L;
            return "f";
        } else if (magnitude < -9) {
            factor = 1e12L;
            return "p";
        } else if (magnitude < -6) {
            factor = 1e9;
            return "n";
        } else if (magnitude < -3) {
            factor = 1e6;
            return "u";
        } else if (magnitude < 0) {
            factor = 1e3;
            return "m";
        } else if (magnitude < 3) {
            factor = 1;
            return "";
        } else if (magnitude < 6) {
            factor = 1e-3;
            return "k";
        } else if (magnitude < 9) {
            factor = 1e-6;
            return "M";
        } else if (magnitude < 12) {
            factor = 1e-9;
            return "G";
        } else if (magnitude < 15) {
            factor = 1e-12;
            return "T";
        } else {
            factor = 1e-15;
            return "P";
        }
    } else if (format == SuffixFormatPercent) {
        factor = 1e2;
        return "%";
    } else if (format == SuffixFormatPPM) {
        factor = 1e6;
        return "ppm";
    }
    assert(false);
    return "";
}

QString FormatFloatSuffices(double val, SuffixFormat format, int precision)
{
    double factor;
    QString suffix = DetermineSuffix(val, factor, format);
    double pfxVal = val*factor;
    double apfxVal = std::abs(pfxVal);

    if (apfxVal != 0.0  &&  apfxVal < std::pow(10.0, -precision)) {
        // Number is so small that even with suffixes, it would be rounded to 0 -> fallback to scientific notation
        return QString::number(val, 'g', precision+1);
    } else {
        QString pfxStr = QString::number(pfxVal, 'f', precision);

        // Remove trailing zeros if a decimal point is present
        static QRegularExpression trimRegex("0+$");
        if (pfxStr.contains('.')) {
            pfxStr.remove(trimRegex);

            // If there are no decimal places, remove the separator as well
            if (pfxStr.endsWith('.')) {
                pfxStr = pfxStr.left(pfxStr.length()-1);
            }
        }

        return QString("%1%2").arg(pfxStr, suffix);
    }
}

QString FormatIntSuffices(int64_t val, SuffixFormat format, int precision)
{
    // TODO: The conversion from int64_t to double is lossy. We really need arbitrary precision numbers...
    return FormatFloatSuffices(static_cast<double>(val), format, precision);
}

QString IndentString(const QString& str, const QString& ind)
{
    static QRegularExpression re("^", QRegularExpression::MultilineOption);
    return QString(str).replace(re, ind);
}

bool ParseVersionNumber(const QString& versionStr, int* major, int* minor, int* patch)
{
    //static QRegularExpression re("^\\d+\\.\\d+(\\.\\d+)");
    static QRegularExpression re(R"/(^\s*(\d+)\.(\d+)(\.(\d+))?)/");
    QRegularExpressionMatch match = re.match(versionStr);
    if (!match.hasMatch()) {
        return false;
    }

    int lmajor = 0;
    int lminor = 0;
    int lpatch = 0;
    bool ok;

    lmajor = match.captured(1).toInt(&ok);
    if (!ok) {
        return false;
    }

    lminor = match.captured(2).toInt(&ok);
    if (!ok) {
        return false;
    }

    if (!match.captured(4).isNull()) {
        lpatch = match.captured(4).toInt(&ok);
        if (!ok) {
            return false;
        }
    }

    if (major) {
        *major = lmajor;
    }
    if (minor) {
        *minor = lminor;
    }
    if (patch) {
        *patch = lpatch;
    }

    return true;
}

QString ReplaceStringVariables(const QString& str, const QMap<QString, QString>& vars, QSet<QString>* outUnmatchedVars)
{
    static QRegularExpression regex("\\$\\{([a-zA-Z0-9_]+)\\}|\\$([a-zA-Z0-9]+)\\b");

    QString replStr;
    int strOffs = 0;

    auto it = regex.globalMatch(str);
    while (it.hasNext()) {
        auto match = it.next();
        replStr += str.mid(strOffs, match.capturedStart(0)-strOffs);
        QString var = match.captured(1) + match.captured(2);
        auto vit = vars.find(var);
        if (vit != vars.end()) {
            replStr += vit.value();
        } else {
            if (outUnmatchedVars) {
                outUnmatchedVars->insert(var);
            }
            replStr += match.captured(0);
        }
        strOffs = match.capturedEnd(0);
    }

    replStr += str.mid(strOffs);

    return replStr;
}


}
