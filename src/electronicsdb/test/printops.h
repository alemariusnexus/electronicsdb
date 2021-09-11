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

#include "test_global.h"

#include <ostream>
#include <QMap>
#include <QString>
#include <QVariant>
#include <gtest/gtest.h>
#include "../model/container/PartContainer.h"
#include "../model/part/Part.h"
#include "../model/PartCategory.h"


inline void PrintTo(const QString& str, std::ostream* os) { *os << str.toUtf8().constData(); }
inline void PrintTo(const QVariant& v, std::ostream* os) { *os << v.toString().toUtf8().constData(); }
void PrintTo(const QMap<QString, QVariant>& d, std::ostream* os);
inline void PrintTo(const electronicsdb::Part& p, std::ostream* os)
        { *os << p.getPartCategory()->getID() << "." << p.getID(); }
inline void PrintTo(const electronicsdb::PartContainer& c, std::ostream* os)
        { *os << c.getID(); }

inline std::ostream& operator<<(std::ostream& os, const QString& str) { return os << str.toUtf8().constData(); }
inline std::ostream& operator<<(std::ostream& os, const QVariant& v) { return os << v.toString().toUtf8().constData(); }
inline std::ostream& operator<<(std::ostream& os, const QMap<QString, QVariant>& d) { PrintTo(d, &os); return os; }
inline std::ostream& operator<<(std::ostream& os, const electronicsdb::Part& p) { PrintTo(p, &os); return os; }
inline std::ostream& operator<<(std::ostream& os, const electronicsdb::PartContainer& c) { PrintTo(c, &os); return os; }
