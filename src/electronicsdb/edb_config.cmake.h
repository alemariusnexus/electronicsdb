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

#include <nxcommon/config.h>


#define EDB_BUILD_VERSION(major,minor,patch) (((major) << 24) | ((minor) << 16) | (patch))
#define EDB_EXTRACT_VERSION_MAJOR(ver) (((ver) >> 24) & 0xFF)
#define EDB_EXTRACT_VERSION_MINOR(ver) (((ver) >> 16) & 0xFF)
#define EDB_EXTRACT_VERSION_PATCH(ver) ((ver) & 0xFFFF)


#cmakedefine EDB_VERSION_MAJOR
#cmakedefine EDB_VERSION_MINOR
#cmakedefine EDB_VERSION_PATCH

#define EDB_GIT_SHA1 "${GIT_SHA1}"
#define EDB_GIT_SHA1_SHORT "${GIT_SHA1_SHORT}"
#define EDB_GIT_REFSPEC "${GIT_REFSPEC}"

#define EDB_VERSION_STRING "${EDB_VERSION_MAJOR}.${EDB_VERSION_MINOR}.${EDB_VERSION_PATCH}-${GIT_SHA1_SHORT} (${GIT_REFSPEC})"
#define EDB_VERSION EDB_BUILD_VERSION(EDB_VERSION_MAJOR, EDB_VERSION_MINOR, EDB_VERSION_PATCH)


#cmakedefine EDB_BUILD_TESTS

#cmakedefine EDB_QT_PDF_AVAILABLE
