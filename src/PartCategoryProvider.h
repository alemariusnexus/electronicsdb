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

#ifndef PARTCATEGORYPROVIDER_H_
#define PARTCATEGORYPROVIDER_H_

#include "global.h"
#include "PartCategory.h"
#include <QtCore/QObject>
#include <QtCore/QList>



class PartCategoryProvider : public QObject
{
	Q_OBJECT

public:
	static PartCategoryProvider* getInstance();
	static void destroy();

public:
	~PartCategoryProvider() {}
	QList<PartCategory*> buildCategories();

private:
	void addNotesProperty(PartCategory* cat, flags_t addFlags = 0);
	void addDatasheetsProperty(PartCategory* cat, flags_t addFlags = 0);
	void addVendorProperty(PartCategory* cat, flags_t addFlags = 0);
	void addNumStockProperty(PartCategory* cat, flags_t addFlags = 0);
	void addTempMaxProperty(PartCategory* cat, flags_t addFlags = 0);
	void addPackageProperty(PartCategory* cat, flags_t addFlags = 0);
	void addToleranceProperty(PartCategory* cat, flags_t addFlags = 0);
	void addKeywordsProperty(PartCategory* cat, flags_t addFlags = 0);

private:
	PartCategory* dsheetCat;

private:
	static PartCategoryProvider* instance;
};

#endif /* PARTCATEGORYPROVIDER_H_ */
