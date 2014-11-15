/*
	Copyright 2010-2014 David "Alemarius Nexus" Lerch

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
#include <cstdlib>
#include <cstdio>
#include <cmath>





double ResolveFloatSuffices(const QString& str, bool* ok, bool siPrefixesAreBase2)
{
	int suffixIdx = str.length();

	bool eFound = false;
	for (unsigned int i = 0 ; i < str.length() ; i++) {
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

		switch (c.toAscii()) {
		case 'm': factor = 1.0e-3; break;
		case 'u': factor = 1.0e-6; break;
		case 'n': factor = 1.0e-9; break;

		case 'P':
		case 'p':
			if (suffixIdx+2 < str.length()  &&  str[suffixIdx+1].toLower() == 'p'  &&  str[suffixIdx+2].toLower() == 'm') {
				factor = 1.0e-6;
			} else if (c == 'p') {
				factor = 1.0e-12;
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
	for (unsigned int i = 0 ; i < str.length() ; i++) {
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

		switch (c.toAscii()) {
		case 'm': factor = 1e-3; break;
		case 'u': factor = 1e-6; break;
		case 'n': factor = 1e-9; break;

		case 'P':
		case 'p':
			if (suffixIdx+2 < str.length()  &&  str[suffixIdx+1].toLower() == 'p'  &&  str[suffixIdx+2].toLower() == 'm') {
				factor = 1e-6;
			} else if (c == 'p') {
				factor = 1e-12;
			}
			break;

		case 'c': factor = 1e-2; break;
		case 'd': factor = 1e-1; break;

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

		case '%':
			factor = 1.0e-2;
			break;
		}
	}

	QString strCpy = str;
	strCpy.truncate(suffixIdx);

	if (sizeof(long) >= sizeof(int64_t))
		return strCpy.toLong(ok) * factor;
	else
		return strCpy.toLongLong(ok) * factor;
}


QString DetermineSuffix(double val, double& factor, SuffixFormat format)
{
	if (format == SuffixFormatNone  ||  val == 0.0) {
		factor = 1.0;
		return "";
	}

	if (format == SuffixFormatSIBase2) {
		if (val < 1024) {
			factor = 1;
			return "";
		} else if (val < pow(1024, 2)) {
			factor = 1.0 / 1024;
			return "Ki";
		} else if (val < pow(1024, 3)) {
			factor = 1.0 / pow(1024, 2);
			return "Mi";
		} else if (val < pow(1024, 4)) {
			factor = 1.0 / pow(1024, 3);
			return "Gi";
		} else {
			factor = 1.0 / pow(1024, 4);
			return "Ti";
		}
	} else if (format == SuffixFormatSIBase10) {
		int magnitude = (int) floor(log10(val));

		if (magnitude < -9) {
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
		} else {
			factor = 1e-12;
			return "T";
		}
	} else if (format == SuffixFormatPercent) {
		factor = 1e2;
		return "%";
	} else if (format == SuffixFormatPPM) {
		factor = 1e6;
		return "ppm";
	}
}


QString FormatFloatSuffices(double val, SuffixFormat format)
{
	double factor;
	QString suffix = DetermineSuffix(val, factor, format);
	return QString("%1%2").arg(val*factor).arg(suffix);
}


QString FormatIntSuffices(int64_t val, SuffixFormat format)
{
	double factor;
	QString suffix = DetermineSuffix(val, factor, format);
	return QString("%1%2").arg(val * factor).arg(suffix);
}
