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

#include "sqlutils.h"
#include <nxcommon/sql/driver/MySQLDatabaseImpl.h>
#include <nxcommon/sql/driver/SQLiteDatabaseImpl.h>





QList<QMap<QString, QString> > SQLListColumns(SQLDatabase db, const QString& table)
{
	SQLDatabaseImpl* impl = db.getImplementation();

	if (dynamic_cast<MySQLDatabaseImpl*>(impl) != NULL) {
		QList<QMap<QString, QString> >colData;

		SQLResult res = db.sendQuery(QString("SHOW COLUMNS FROM %1").arg(table));

		while (res.nextRecord()) {
			QMap<QString, QString> data;

			data["name"] = res.getString(0);

			colData << data;
		}

		return colData;
	} else if (dynamic_cast<SQLiteDatabaseImpl*>(impl) != NULL) {
		QList<QMap<QString, QString> > colData;

		SQLResult res = db.sendQuery(QString("PRAGMA table_info(%1)").arg(table));

		while (res.nextRecord()) {
			QMap<QString, QString> data;

			data["name"] = res.getString(1);

			colData << data;
		}

		return colData;
	} else {
		throw SQLException("SQLListColumns called with an unsupported database backend!", __FILE__, __LINE__);
	}
}
