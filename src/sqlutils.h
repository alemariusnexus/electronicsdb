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

#ifndef SQLUTILS_H_
#define SQLUTILS_H_

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <nxcommon/sql/sql.h>


QList<QMap<QString, QString> > SQLListColumns(SQLDatabase db, const QString& table);

#endif /* SQLUTILS_H_ */
