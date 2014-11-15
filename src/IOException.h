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

#ifndef IOEXCEPTION_H_
#define IOEXCEPTION_H_

#include "Exception.h"


class IOException : public Exception
{
public:
	IOException(const char* message, const char* srcFile = NULL, int srcLine = -1,
			const Exception* nestedException = NULL)
			: Exception(message, srcFile, srcLine, nestedException, "IOException") {}
};

#endif /* IOEXCEPTION_H_ */
