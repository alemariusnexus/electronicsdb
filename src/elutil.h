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

#ifndef ELUTIL_H_
#define ELUTIL_H_

#include "global.h"
#include <QtCore/QString>
#include <stdint.h>
#include <nxcommon/util.h>


enum SuffixFormat
{
	SuffixFormatSIBase10,
	SuffixFormatSIBase2,
	SuffixFormatPercent,
	SuffixFormatPPM,
	SuffixFormatNone
};



int64_t ResolveIntSuffices(const QString& str, bool* ok = NULL, bool siPrefixesAreBase2 = false);
double ResolveFloatSuffices(const QString& str, bool* ok = NULL, bool siPrefixesAreBase2 = false);

QString FormatFloatSuffices(double val, SuffixFormat format = SuffixFormatSIBase10);
QString FormatIntSuffices(int64_t val, SuffixFormat format = SuffixFormatSIBase10);

#endif /* ELUTIL_H_ */

