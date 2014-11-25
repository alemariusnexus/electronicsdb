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

#ifndef PARTCATEGORY_H_
#define PARTCATEGORY_H_

#include "global.h"
#include "PartProperty.h"
#include "Filter.h"
#include "DatabaseConnection.h"
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QIODevice>

using Xapian::WritableDatabase;



class PartCategory : public QObject
{
	Q_OBJECT

public:
	typedef QList<PartProperty*> PropertyList;
	typedef PropertyList::iterator PropertyIterator;
	typedef PropertyList::const_iterator ConstPropertyIterator;

	typedef QMap<PartProperty*, QList<QString> > DataMap;
	typedef QMap<unsigned int, DataMap> MultiDataMap;

public:
	PartCategory(const QString& id, const QString& userReadableName, const QString& idField = "id");
	~PartCategory();

	void addProperty(PartProperty* prop);
	void setDescriptionPattern(const QString& descPat);

	QString getID() const { return id; }
	QString getTableName() const { return QString("pcat_") + id; }
	QString getUserReadableName() const { return userReadableName; }
	QString getIDField() const { return idField; }
	PropertyIterator getPropertyBegin() { return props.begin(); }
	PropertyIterator getPropertyEnd() { return props.end(); }
	ConstPropertyIterator getPropertyBegin() const { return props.begin(); }
	ConstPropertyIterator getPropertyEnd() const { return props.end(); }
	PropertyList getProperties() const { return props; }
	size_t getPropertyCount() const { return props.size(); }
	PartProperty* getProperty(const QString& fieldName);
	PropertyList getDescriptionProperties();

	//QList<QString> generateCreateTablesCode(DatabaseConnection::Type dbType) const;
	//void createTables();

	QString getDescription(unsigned int id, const DataMap& data);
	QMap<unsigned int, QString> getDescriptions(QList<unsigned int> ids);
	QString getDescription(unsigned int id);

	void rebuildFullTextIndex() { rebuildFullTextIndex(true, QList<unsigned int>()); }
	void updateFullTextIndex(QList<unsigned int> ids) { rebuildFullTextIndex(false, ids); }
	void updateFullTextIndex(unsigned int id) { updateFullTextIndex(QList<unsigned int>() << id); }
	void clearFullTextIndex();

	QList<unsigned int> find(Filter* filter, unsigned int offset, unsigned int numResults);
	unsigned int countMatches(Filter* filter);

	MultiDataMap getValues(QList<unsigned int> ids, PropertyList props = PropertyList());
	DataMap getValues(unsigned int id, PropertyList props = PropertyList());

	void saveRecord(unsigned int id, DataMap data);
	unsigned int createRecord(DataMap data);
	unsigned int createRecord();
	void removeRecords(QList<unsigned int> ids);
	void removeRecord(unsigned int id);

private:
	void rebuildFullTextIndex(bool fullRebuild, QList<unsigned int> ids);
	QList<unsigned int> find(Filter* filter, unsigned int offset, unsigned int numResults, bool countOnly);

	//QString generateSQLColumnTypeDefinition(const PartProperty* prop, DatabaseConnection::Type dbType) const;

private slots:
	void databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn);

signals:
	// The PartCategory:: prefix is crucial for Qt's signals and slots system!
	void recordCreated(unsigned int id, PartCategory::DataMap data);
	void recordAboutToBeEdited(unsigned int id, PartCategory::DataMap newData);
	void recordEdited(unsigned int id, PartCategory::DataMap newData);
	void recordsAboutToBeRemoved(QList<unsigned int> ids);
	void recordsRemoved(QList<unsigned int> ids);

private:
	QString id;
	QString userReadableName;
	QString idField;
	QString descPattern;

	WritableDatabase xapianDb;

	PropertyList props;
};

#endif /* PARTCATEGORY_H_ */
