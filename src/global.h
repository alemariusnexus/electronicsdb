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

#ifndef GLOBAL_H_
#define GLOBAL_H_

// Always include xapian.h before any Qt headers, because Xapian uses identifiers such as "slots", which
// clashes with Qt's signals and slots system #defines.
#include <xapian.h>
#include <stdint.h>


#define INVALID_PART_ID 0
#define INVALID_CONTAINER_ID 0

#define MAX_PART_CATEGORY_TABLE_NAME 64
#define MAX_CONTAINER_NAME 128
#define MAX_FILE_PROPERTY_PATH_LEN 512

typedef uint32_t flags_t;


enum DisplayWidgetState
{
	Enabled,
	Disabled,
	ReadOnly
};

enum DisplayWidgetFlags
{
	ChoosePart = 1 << 0
};


#endif /* GLOBAL_H_ */
