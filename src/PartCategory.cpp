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

#include "PartCategory.h"
#include "System.h"
#include "NoDatabaseConnectionException.h"
#include <nxcommon/exception/InvalidValueException.h>
#include "SQLCommand.h"
#include "SQLUpdateCommand.h"
#include "SQLDeleteCommand.h"
#include "SQLInsertCommand.h"
#include "SQLMultiValueInsertCommand.h"
#include "SQLNewRecordCommand.h"
#include "ContainerRemovePartsCommand.h"
#include "PartEditCommand.h"
#include "elutil.h"
#include "sqlutils.h"
#include <QtCore/QSet>
#include <QtCore/QTextStream>
#include <QtGui/QMessageBox>
#include <nxcommon/sql/sql.h>

using Xapian::TermGenerator;
using Xapian::Document;
using Xapian::QueryParser;
using Xapian::Query;
using Xapian::Enquire;
using Xapian::MSet;
using Xapian::MSetIterator;
using Xapian::Utf8Iterator;


#define MAX_INDEX_REBUILD_FETCH_ROWS 10000
#define FILTER_SEARCH_BLOCK_SIZE 50000
#define FILTER_SEARCH_SQL_BLOCK_SIZE 50000




PartCategory::PartCategory(const QString& id, const QString& userReadableName, const QString& idField)
		: id(id), userReadableName(userReadableName), idField(idField),
		  xapianDb(Xapian::InMemory::open())
{
	System* sys = System::getInstance();

	connect(sys, SIGNAL(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)),
			this, SLOT(databaseConnectionStatusChanged(DatabaseConnection*, DatabaseConnection*)));
}


PartCategory::~PartCategory()
{
	for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
		delete *it;
	}
}


void PartCategory::addProperty(PartProperty* prop)
{
	props.push_back(prop);
	prop->cat = this;
}


PartProperty* PartCategory::getProperty(const QString& fieldName)
{
	for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
		PartProperty* prop = *it;

		if (prop->getFieldName() == fieldName)
			return prop;
	}

	return NULL;
}


void PartCategory::rebuildFullTextIndex(bool fullRebuild, QList<unsigned int> ids)
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection()) {
		throw NoDatabaseConnectionException("Database connection needed to build full text index",
				__FILE__, __LINE__);
	}

	try {
		SQLDatabase sql = sys->getCurrentSQLDatabase();

		QString sqlQuery = QString("SELECT %1").arg(getIDField());

		PropertyList indexProps;
		PropertyList mvIndexProps;

		for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
			PartProperty* prop = *it;

			flags_t flags = prop->getFlags();

			if ((flags & PartProperty::FullTextIndex)  ==  0)
				continue;

			if ((flags & PartProperty::MultiValue)  !=  0) {
				mvIndexProps.push_back(prop);
			} else {
				indexProps.push_back(prop);
				sqlQuery += QString(", %1").arg(prop->getFieldName());
			}
		}

		QString whereCode = "";

		if (!fullRebuild) {
			whereCode += "WHERE ";

			for (QList<unsigned int>::iterator it = ids.begin() ; it != ids.end() ; it++) {
				if (it != ids.begin()) {
					whereCode += " OR ";
				}

				whereCode += QString("%1=%2").arg(getIDField()).arg(*it);
			}
		}

		MultiDataMap mvValues;

		for (PropertyIterator it = mvIndexProps.begin() ; it != mvIndexProps.end() ; it++) {
			PartProperty* prop = *it;

			QString query = QString("SELECT %1, %2 FROM %3 %4")
					.arg(prop->getMultiValueIDFieldName())
					.arg(prop->getFieldName())
					.arg(prop->getMultiValueForeignTableName())
					.arg(whereCode);

			SQLResult res = sql.sendQuery(query);

			if (!res.isValid())
				throw SQLException("Error storing SQL result", __FILE__, __LINE__);

			while (res.nextRecord()) {
				unsigned int id = res.getUInt32(0);
				QString val = res.getString(1);

				mvValues[id][prop] << val;
			}
		}

		sqlQuery += QString(" FROM %1 %2").arg(getTableName()).arg(whereCode);


		int numFetched;

		TermGenerator termGen;

		termGen.set_stemmer(Xapian::Stem("en"));
		termGen.set_stemming_strategy(TermGenerator::STEM_SOME);

		unsigned int i = 0;
		do {
			QString query = QString("%1 LIMIT %2, %3").arg(sqlQuery).arg(i*MAX_INDEX_REBUILD_FETCH_ROWS)
					.arg(MAX_INDEX_REBUILD_FETCH_ROWS);

			SQLResult res = sql.sendQuery(query);

			if (!res.isValid())
				throw SQLException("Error storing SQL result", __FILE__, __LINE__);

			numFetched = 0;

			while (res.nextRecord()) {
				unsigned int id = res.getUInt32(0);

				Document doc;
				doc.set_data(QString("%1").arg(id).toStdString());

				termGen.set_document(doc);

				unsigned int j = 0;
				for (PropertyIterator it = indexProps.begin() ; it != indexProps.end() ; it++, j++) {
					PartProperty* prop = *it;

					QString text = res.getString(j+1);
					QByteArray utf8Text = text.toUtf8();

					termGen.index_text(utf8Text.constData(), 1, QString("X%1").arg(j).toAscii().constData());

					termGen.increase_termpos();

					termGen.index_text(utf8Text.constData());

					termGen.increase_termpos();
				}

				DataMap idMvValues = mvValues[id];

				for (DataMap::iterator it = idMvValues.begin() ; it != idMvValues.end() ; it++, j++) {
					PartProperty* prop = it.key();
					QList<QString> vals = it.value();

					for (QList<QString>::iterator sit = vals.begin() ; sit != vals.end() ; sit++) {
						QString val = *sit;

						QByteArray utf8Val = val.toUtf8();

						termGen.index_text(utf8Val.constData(), 1, QString("X%1").arg(j).toAscii().constData());

						termGen.increase_termpos();

						termGen.index_text(utf8Val.constData());

						termGen.increase_termpos();
					}
				}

				xapianDb.replace_document(id, doc);

				numFetched++;
			}

			i++;
		} while (numFetched >= MAX_INDEX_REBUILD_FETCH_ROWS);
	} catch (SQLException& ex) {
		SQLException eex("Error caught while rebuilding full-text index", __FILE__, __LINE__, &ex);
		throw eex;
	}
}


void PartCategory::clearFullTextIndex()
{
	xapianDb.close();
	xapianDb = Xapian::InMemory::open();
}


QList<unsigned int> PartCategory::find(Filter* filter, unsigned int offset, unsigned int numResults, bool countOnly)
{
	if (numResults == 0  &&  !countOnly) {
		return QList<unsigned int>();
	}

	bool usesSqlFiltering = !filter->getSQLWhereCode().isEmpty();
	bool usesFtFiltering = !filter->getFullTextQuery().isEmpty();

	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection()) {
		throw NoDatabaseConnectionException("Database connection needed to find parts", __FILE__, __LINE__);
	}

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	try {
		QList<unsigned int> results;

		QString joinCode("");

		bool first = true;
		for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
			PartProperty* prop = *it;

			if ((prop->getFlags() & PartProperty::MultiValue)  !=  0) {
				if (!first)
					joinCode += " ";

				joinCode += QString("LEFT JOIN %1 ON %1.%2=%3.%4")
						.arg(prop->getMultiValueForeignTableName())
						.arg(prop->getMultiValueIDFieldName())
						.arg(getTableName())
						.arg(idField);

				first = false;
			}
		}

		if (!usesFtFiltering) {
			// We can bypass the whole Xapian search

			QString query;

			if (countOnly) {
				query = QString("SELECT COUNT(DISTINCT %2.%1) FROM %2 %3 %4")
						.arg(getIDField())
						.arg(getTableName())
						.arg(joinCode)
						.arg(filter->getSQLWhereCode());
			} else {
				query = QString("SELECT DISTINCT %2.%1 FROM %2 %3 %4 %5 LIMIT %6, %7")
						.arg(getIDField())
						.arg(getTableName())
						.arg(joinCode)
						.arg(filter->getSQLWhereCode())
						.arg(filter->getSQLOrderCode())
						.arg(offset)
						.arg(numResults);
			}

			SQLResult res = sql.sendQuery(query);

			if (!res.isValid()) {
				throw SQLException("Error storing SQL result", __FILE__, __LINE__);
			}

			if (countOnly) {
				if (!res.nextRecord()) {
					throw SQLException("Filter counting query returned zero records!", __FILE__, __LINE__);
				}

				unsigned int count = res.getUInt32(0);
				results << count;
			} else {
				while (res.nextRecord()) {
					results << res.getUInt32(0);
				}
			}
		} else {
			QueryParser qp;
			qp.set_stemmer(Xapian::Stem("en"));
			qp.set_stemming_strategy(QueryParser::STEM_SOME);
			qp.set_database(xapianDb);

			unsigned int i = 0;
			for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
				PartProperty* prop = *it;

				flags_t flags = prop->getFlags();

				if ((flags & PartProperty::FullTextIndex)  !=  0  &&  (flags & PartProperty::MultiValue)  ==  0) {
					qp.add_prefix(prop->getFullTextSearchUserPrefix().toAscii().constData(),
							QString("X%1").arg(i).toAscii().constData());
					i++;
				}
			}

			for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
				PartProperty* prop = *it;

				flags_t flags = prop->getFlags();

				if ((flags & PartProperty::FullTextIndex)  !=  0  &&  (flags & PartProperty::MultiValue)  !=  0) {
					qp.add_prefix(prop->getFullTextSearchUserPrefix().toAscii().constData(),
							QString("X%1").arg(i).toAscii().constData());
					i++;
				}
			}

			Query q = qp.parse_query(filter->getFullTextQuery().toUtf8().constData(),
					QueryParser::FLAG_BOOLEAN | QueryParser::FLAG_PHRASE | QueryParser::FLAG_LOVEHATE | QueryParser::FLAG_WILDCARD);

			Enquire enq(xapianDb);
			enq.set_query(q);


			if (filter->getSQLOrderCode().isEmpty()  &&  !countOnly) {
				// No SQL ordering, so we order by Xapian rank.

				unsigned int lastFtNumResults = FILTER_SEARCH_BLOCK_SIZE;
				while (numResults != 0  &&  lastFtNumResults == FILTER_SEARCH_BLOCK_SIZE) {
					MSet mset = enq.get_mset(offset, FILTER_SEARCH_BLOCK_SIZE);

					lastFtNumResults = mset.size();

					if (lastFtNumResults == 0)
						break;

					QString idList = "";

					for (MSetIterator it = mset.begin() ; it != mset.end() ; it++) {
						if (it != mset.begin())
							idList += ",";
						unsigned int id = it.get_document().get_docid();
						idList += QString("%1").arg(id);
					}

					QString whereCode = filter->getSQLWhereCode();

					if (whereCode.isEmpty()) {
						whereCode = "WHERE";
					} else {
						whereCode += " AND";
					}

					QString query = QString("SELECT DISTINCT %2.%1 FROM %2 %3 %4 %1 IN(%5) ORDER BY %1 ASC")
							.arg(getIDField())
							.arg(getTableName())
							.arg(joinCode)
							.arg(whereCode)
							.arg(idList);

					SQLResult res = sql.sendQuery(query);

					if (!res.isValid()) {
						throw SQLException("Error storing SQL result", __FILE__, __LINE__);
					}

					QSet<unsigned int> sqlMatchedIds;

					while (res.nextRecord()) {
						sqlMatchedIds.insert(res.getUInt32(0));
					}

					for (MSetIterator it = mset.begin() ; it != mset.end()  &&  numResults != 0 ; it++) {
						unsigned int id = it.get_document().get_docid();

						if (sqlMatchedIds.contains(id)) {
							results << id;
							numResults--;
						}
					}

					offset += lastFtNumResults;
				}
			} else {
				// SQL ordering code given, so we order by that and ignore Xapian rank.

				QSet<unsigned int> ftMatchingIds;

				MSet mset = enq.get_mset(0, xapianDb.get_doccount());

				for (MSetIterator it = mset.begin() ; it != mset.end() ; it++) {
					ftMatchingIds << it.get_document().get_docid();
				}

				if (!countOnly) {
					unsigned int i = 0;

					while (numResults != 0) {
						QString query = QString("SELECT DISTINCT %2.%1 FROM %2 %3 %4 %5 LIMIT %6, %7")
								.arg(getIDField())
								.arg(getTableName())
								.arg(joinCode)
								.arg(filter->getSQLWhereCode())
								.arg(filter->getSQLOrderCode())
								.arg(i*FILTER_SEARCH_SQL_BLOCK_SIZE)
								.arg(FILTER_SEARCH_SQL_BLOCK_SIZE);

						SQLResult res = sql.sendQuery(query);

						if (!res.isValid()) {
							throw SQLException("Error storing SQL result", __FILE__, __LINE__);
						}

						bool hadNextRow;
						while ((hadNextRow = res.nextRecord())  &&  numResults != 0) {
							unsigned int id = res.getUInt32(0);

							if (ftMatchingIds.contains(id)) {
								results << id;
								numResults--;
							}
						}

						if (!hadNextRow)
							break;

						i++;
					}
				} else {
					QString whereCode(filter->getSQLWhereCode());

					if (whereCode.isEmpty()) {
						whereCode = QString("WHERE ");
					} else {
						int conditionStartIdx = whereCode.indexOf("WHERE ") + 6;

						if (conditionStartIdx == -1) {
							throw InvalidValueException("Invalid WHERE code in Filter object!", __FILE__, __LINE__);
						}

						whereCode.insert(conditionStartIdx, '(');
						whereCode.append(") AND ");
					}

					whereCode += QString("%2.%1 IN(")
							.arg(getIDField())
							.arg(getTableName());

					for (QSet<unsigned int>::iterator it = ftMatchingIds.begin() ; it != ftMatchingIds.end() ; it++) {
						if (it != ftMatchingIds.begin())
							whereCode += ',';
						whereCode += QString("%1").arg(*it);
					}

					whereCode += ")";

					QString query = QString("SELECT COUNT(DISTINCT %2.%1) FROM %2 %3 %4")
							.arg(getIDField())
							.arg(getTableName())
							.arg(joinCode)
							.arg(whereCode);

					SQLResult res = sql.sendQuery(query);

					if (!res.isValid()) {
						throw SQLException("Error storing SQL result", __FILE__, __LINE__);
					}

					if (!res.nextRecord()) {
						throw SQLException("Filter counting query returned zero records!", __FILE__, __LINE__);
					}

					unsigned int count = res.getUInt32(0);
					results << count;
				}
			}
		}

		return results;
	} catch (SQLException& ex) {
		SQLException eex("Error caught while finding parts", __FILE__, __LINE__, &ex);
		throw eex;
	}
}


QList<unsigned int> PartCategory::find(Filter* filter, unsigned int offset, unsigned int numResults)
{
	return find(filter, offset, numResults, false);
}


unsigned int PartCategory::countMatches(Filter* filter)
{
	return find(filter, 0, 0, true)[0];
}


PartCategory::MultiDataMap PartCategory::getValues(QList<unsigned int> ids, PropertyList props)
{
	if (ids.empty())
		return MultiDataMap();

	if (props.empty())
		props = getProperties();

	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection()) {
		throw NoDatabaseConnectionException("Database connection needed to get part values", __FILE__, __LINE__);
	}

	SQLDatabase sql = sys->getCurrentSQLDatabase();

	try {
		MultiDataMap data;

		QString query = QString("SELECT %1.%2").arg(getTableName()).arg(getIDField());

		unsigned int numCols = 0;
		PropertyList multiValueProps;

		for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
			PartProperty* prop = *it;

			flags_t flags = prop->getFlags();

			if ((flags & PartProperty::MultiValue)  !=  0) {
				multiValueProps << prop;
				continue;
			}

			query += QString(", %1").arg(prop->getFieldName());

			numCols++;
		}

		QString idListStr("");

		for (QList<unsigned int>::iterator it = ids.begin() ; it != ids.end() ; it++) {
			unsigned int id = *it;

			if (it != ids.begin())
				idListStr += ",";

			idListStr += QString("%1").arg(id);
		}


		//QMap<unsigned int, QMap<PartProperty*, QList<QString> > > multiValueData;

		for (PartCategory::PropertyIterator it = multiValueProps.begin() ; it != multiValueProps.end() ; it++) {
			PartProperty* prop = *it;

			QString mvQuery = QString("SELECT DISTINCT %1.%2, %3.%5 FROM %1 JOIN %3 ON %3.%4=%1.%2 WHERE %1.%2 IN(%6)")
					.arg(getTableName())
					.arg(getIDField())
					.arg(prop->getMultiValueForeignTableName())
					.arg(prop->getMultiValueIDFieldName())
					.arg(prop->getFieldName())
					.arg(idListStr);

			SQLResult mvRes = sql.sendQuery(mvQuery);

			if (!mvRes.isValid()) {
				throw SQLException("SQL error while storing result", __FILE__, __LINE__);
			}

			while (mvRes.nextRecord()) {
				unsigned int id = mvRes.getUInt32(0);

				QList<QString>& values = data[id][prop];

				QString val = mvRes.getString(1);
				values << val;
			}
		}


		query += QString(" FROM %1 WHERE %2 IN(%3) ORDER BY %2 ASC")
				.arg(getTableName())
				.arg(getIDField())
				.arg(idListStr);

		SQLResult res = sql.sendQuery(query);

		if (!res.isValid()) {
			throw SQLException("SQL error while storing result", __FILE__, __LINE__);
		}

		while (res.nextRecord()) {
			unsigned int id = res.getUInt32(0);

			QMap<PartProperty*, QList<QString> >& idData = data[id];

			unsigned int i = 0;
			for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
				PartProperty* prop = *it;

				if ((prop->getFlags() & PartProperty::MultiValue)  != 0) {
					continue;
				}

				QString val = res.getString(i+1);
				idData[prop] << val;

				i++;
			}
		}

		return data;
	} catch (SQLException& ex) {
		SQLException eex("Error caught while getting part values", __FILE__, __LINE__, &ex);
		throw eex;
	}
}


PartCategory::DataMap PartCategory::getValues(unsigned int id, PropertyList props)
{
	return getValues(QList<unsigned int>() << id, props)[id];
}


void PartCategory::saveRecord(unsigned int id, DataMap data)
{
	PartEditCommand* partCmd = new PartEditCommand(this);

	partCmd->setDescription(QString(tr("Updated part with ID %1 of category \"%2\"")).arg(id).arg(getUserReadableName()));

	SQLUpdateCommand* baseCmd = new SQLUpdateCommand(getTableName(), idField, id);
	partCmd->addSQLCommand(baseCmd);

	for (DataMap::iterator it = data.begin() ; it != data.end() ; it++) {
		PartProperty* prop = it.key();
		QList<QString> vals = it.value();

		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::MultiValue)  !=  0) {
			SQLDeleteCommand* delCmd = new SQLDeleteCommand(prop->getMultiValueForeignTableName(),
					prop->getMultiValueIDFieldName());
			delCmd->addRecord(id);
			SQLMultiValueInsertCommand* insCmd = new SQLMultiValueInsertCommand(prop->getMultiValueForeignTableName(),
					prop->getMultiValueIDFieldName(), id);
			QList<QMap<QString, QString> > mvData;

			for (QList<QString>::iterator sit = vals.begin() ; sit != vals.end() ; sit++) {
				QMap<QString, QString> entry;
				entry[prop->getFieldName()] = *sit;
				mvData << entry;
			}
			insCmd->addValues(mvData);

			partCmd->addSQLCommand(delCmd);
			partCmd->addSQLCommand(insCmd);
		} else {
			QString val = vals[0];
			baseCmd->addFieldValue(prop->getFieldName(), val);
		}
	}

	System* sys = System::getInstance();
	EditStack* editStack = sys->getEditStack();

	emit recordAboutToBeEdited(id, data);

	editStack->push(partCmd);

	emit recordEdited(id, data);
}


QList<unsigned int> PartCategory::createRecords(QList<DataMap> data)
{
	PartEditCommand* partCmd = new PartEditCommand(this);
	partCmd->setDescription(QString(tr("Created %1 new parts of category \"%2\"")).arg(data.size()).arg(getUserReadableName()));

	QList<SQLNewRecordCommand*> sqlCmds;

	for (DataMap datum : data) {
		SQLNewRecordCommand* cmd = new SQLNewRecordCommand(this, datum);

		partCmd->addSQLCommand(cmd);
		sqlCmds << cmd;
	}

	System* sys = System::getInstance();
	EditStack* editStack = sys->getEditStack();

	editStack->push(partCmd);

	QList<unsigned int> ids;

	unsigned int i = 0;
	for (DataMap datum : data) {
		SQLNewRecordCommand* cmd = sqlCmds[i];

		unsigned int id = cmd->getLastInsertID();
		emit recordCreated(id, datum);
		ids << id;

		i++;
	}

	return ids;
}


unsigned int PartCategory::createRecord(DataMap data)
{
	return createRecords(QList<DataMap>() << data)[0];
	/*SQLNewRecordCommand* cmd = new SQLNewRecordCommand(this, data);

	PartEditCommand* partCmd = new PartEditCommand(this);
	partCmd->setDescription(QString(tr("Created a new part of category \"%1\"")).arg(getUserReadableName()));
	partCmd->addSQLCommand(cmd);

	System* sys = System::getInstance();
	EditStack* editStack = sys->getEditStack();

	editStack->push(partCmd);

	unsigned int id = cmd->getLastInsertID();

	emit recordCreated(id, data);

	return id;*/
}


unsigned int PartCategory::createRecord()
{
	DataMap data;

	for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
		PartProperty* prop = *it;

		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::MultiValue)  !=  0) {
			data[prop] = QList<QString>();
		} else {
			data[prop] << prop->getInitialValue();
		}
	}

	return createRecord(data);
}


void PartCategory::removeRecords(const QList<unsigned int>& ids)
{
	if (ids.empty())
		return;

	PartEditCommand* partCmd = new PartEditCommand(this);

	if (ids.size() == 1) {
		partCmd->setDescription(QString(tr("Removed part with ID %1 of category \"%2\"")).arg(ids[0]).arg(getUserReadableName()));
	} else {
		partCmd->setDescription(QString(tr("Removed %1 parts of category \"%2\"")).arg(ids.size()).arg(getUserReadableName()));
	}

	// Remove references to removed records in the containers
	ContainerRemovePartsCommand* contCmd = new ContainerRemovePartsCommand;
	contCmd->setAllContainers(true);

	for (unsigned int id : ids) {
		contCmd->addPart(this, id);
	}
	partCmd->addSQLCommand(contCmd);

	SQLDeleteCommand* baseCmd = new SQLDeleteCommand(getTableName(), getIDField());
	baseCmd->addRecords(ids);
	partCmd->addSQLCommand(baseCmd);

	for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
		PartProperty* prop = *it;

		if ((prop->getFlags() & PartProperty::MultiValue)  !=  0) {
			SQLDeleteCommand* delCmd = new SQLDeleteCommand(prop->getMultiValueForeignTableName(),
					prop->getMultiValueIDFieldName());
			delCmd->addRecords(ids);
			partCmd->addSQLCommand(delCmd);
		}
	}

	System* sys = System::getInstance();
	EditStack* editStack = sys->getEditStack();

	emit recordsAboutToBeRemoved(ids);

	editStack->push(partCmd);

	emit recordsRemoved(ids);
}


void PartCategory::removeRecord(unsigned int id)
{
	removeRecords(QList<unsigned int>() << id);
}


void PartCategory::databaseConnectionStatusChanged(DatabaseConnection* oldConn, DatabaseConnection* newConn)
{
	System* sys = System::getInstance();

	if (sys->isEmergencyDisconnecting()) {
		return;
	}

	try {
		if (oldConn  &&  !newConn) {
			clearFullTextIndex();
		} else if (newConn) {
			rebuildFullTextIndex();
		}
	} catch (SQLException& ex) {
		QMessageBox::critical(NULL, tr("SQL Error"),
				tr("An SQL error was caught while rebuilding the full-text index:\n\n%1").arg(ex.what()));
		sys->emergencyDisconnect();
		return;
	}
}


/*QString PartCategory::generateSQLColumnTypeDefinition(const PartProperty* prop, DatabaseConnection::Type dbType) const
{
	QString query("");

	PartProperty::Type type = prop->getType();
	flags_t flags = prop->getFlags();

	if (type == PartProperty::Integer) {
		if (dbType == DatabaseConnection::MySQL) {
			PartProperty::IntegerStorageType stype = prop->getIntegerStorageType();
			bool isSigned = prop->isIntegerSigned();

			switch (stype) {
			case PartProperty::Int8:
				query += "TINYINT";
				break;
			case PartProperty::Int16:
				query += "SMALLINT";
				break;
			case PartProperty::Int24:
				query += "MEDIUMINT";
				break;
			case PartProperty::Int32:
				query += "INT";
				break;
			case PartProperty::Int64:
				query += "BIGINT";
				break;
			}

			if (!isSigned)
				query += " UNSIGNED";
		} else {
			query += "INTEGER";
		}
	} else if (type == PartProperty::Float) {
		query += "FLOAT";
	} else if (type == PartProperty::String) {
		unsigned int strMaxLen = prop->getStringMaximumLength();

		if (strMaxLen == 0  ||  strMaxLen > 65535) {
			query += "TEXT";
		} else {
			query += QString("VARCHAR(%1)").arg(strMaxLen);
		}

		if (dbType == DatabaseConnection::MySQL) {
			query += " CHARACTER SET utf8";
		}
	} else if (type == PartProperty::PartLink) {
		query += "INTEGER";
	} else if (type == PartProperty::Decimal) {
		query += QString("DECIMAL(%1, %2)").arg(prop->getDecimalPrecision()).arg(prop->getDecimalScale());
	} else if (type == PartProperty::Boolean) {
		if (dbType == DatabaseConnection::MySQL) {
			query += "BOOLEAN";
		} else {
			query += "INTEGER";
		}
	} else if (type == PartProperty::File) {
		query += QString("VARCHAR(%1)").arg(MAX_FILE_PROPERTY_PATH_LEN);
	} else {
		throw InvalidValueException("Invalid PartProperty type in PartCategory::generateSQLColumnTypeDefinition()!",
				__FILE__, __LINE__);
	}

	if ((flags & PartProperty::NullAllowed)  ==  0) {
		query += " NOT NULL";
	}

	query += QString(" DEFAULT '%1'").arg(prop->getInitialValue());

	return query;
}


void PartCategory::createTables()
{
	System* sys = System::getInstance();

	if (!sys->hasValidDatabaseConnection()) {
		throw NoDatabaseConnectionException("Database connection needed to create category tables!",
				__FILE__, __LINE__);
	}

	SQLDatabase sql = sys->getCurrentSQLDatabase();
	DatabaseConnection* conn = sys->getCurrentDatabaseConnection();

	QList<QString> queries = generateCreateTablesCode(conn->getType());

	for (QList<QString>::iterator it = queries.begin() ; it != queries.end() ; it++) {
		QString query = *it;

		sql.sendQuery(query);
	}

	QList<QMap<QString, QString> > colData = SQLListColumns(sql, getTableName());
	QSet<QString> colNames;

	for (QList<QMap<QString, QString> >::iterator it = colData.begin() ; it != colData.end() ; it++) {
		QString name = (*it)["name"];
		colNames.insert(name);
	}

	for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
		PartProperty* prop = *it;

		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::MultiValue)  ==  0) {
			if (!colNames.contains(prop->getFieldName())) {
				printf("INFO: Adding SQL column for part property %s of category %s that does not exist.\n",
						prop->getFieldName().toUtf8().constData(),
						getTableName().toUtf8().constData());

				QString query = QString("ALTER TABLE %1 ADD %2 %3")
						.arg(getTableName())
						.arg(prop->getFieldName())
						.arg(generateSQLColumnTypeDefinition(prop, conn->getType()));

				sql.sendQuery(query);
			}
		}
	}
}


QList<QString> PartCategory::generateCreateTablesCode(DatabaseConnection::Type dbType) const
{
	QList<QString> queries;

	QString query = QString("CREATE TABLE IF NOT EXISTS %1 (\n")
			.arg(getTableName());

	query += QString("    %1 INTEGER NOT NULL").arg(idField);

	if (dbType == DatabaseConnection::MySQL) {
		query += " AUTO_INCREMENT";
	}

	QList<const PartProperty*> mvProps;

	for (ConstPropertyIterator it = props.begin() ; it != props.end() ; it++) {
		const PartProperty* prop = *it;

		flags_t flags = prop->getFlags();

		if ((flags & PartProperty::MultiValue)  !=  0) {
			mvProps << prop;
			continue;
		}

		query += ",\n    ";

		query += prop->getFieldName() + " ";

		query += generateSQLColumnTypeDefinition(prop, dbType);
	}

	query += ",\n    ";

	query += QString("PRIMARY KEY(%1)").arg(idField);

	query += "\n)";

	queries << query;



	for (QList<const PartProperty*>::iterator it = mvProps.begin() ; it != mvProps.end() ; it++) {
		const PartProperty* prop = *it;

		query = QString("CREATE TABLE IF NOT EXISTS %1 (\n").arg(prop->getMultiValueForeignTableName());
		query += QString("    %1 INTEGER NOT NULL,\n").arg(prop->getMultiValueIDFieldName());
		query += QString("    %1 %2").arg(prop->getFieldName()).arg(generateSQLColumnTypeDefinition(prop, dbType));

		if (dbType == DatabaseConnection::SQLite) {
			query += "\n";
		} else {
			query += ",\n";
			query += QString("    INDEX %1_%2_idx (%2)\n")
					.arg(prop->getMultiValueForeignTableName())
					.arg(prop->getMultiValueIDFieldName());
		}

		query += ")";

		queries << query;

		if (dbType == DatabaseConnection::SQLite) {
			query = QString("CREATE INDEX IF NOT EXISTS %1_%2_idx ON %1 (%2)")
					.arg(prop->getMultiValueForeignTableName())
					.arg(prop->getMultiValueIDFieldName());

			queries << query;
		}
	}

	return queries;
}*/


PartCategory::PropertyList PartCategory::getDescriptionProperties()
{
	PropertyList descProps;

	for (PropertyIterator it = props.begin() ; it != props.end() ; it++) {
		PartProperty* prop = *it;

		if ((prop->getFlags() & PartProperty::DescriptionProperty)  !=  0) {
			descProps << prop;
		}
	}

	return descProps;
}


QString PartCategory::getDescription(unsigned int id, const DataMap& data)
{
	QString desc = descPattern;

	desc.replace(QString("$id"), QString("%1").arg(id));

	for (DataMap::const_iterator it = data.begin() ; it != data.end() ; it++) {
		const PartProperty* prop = it.key();
		const QList<QString> vals = it.value();

		QString descStr = prop->formatValueForDisplay(vals);

		desc.replace(QString("$%1").arg(prop->getFieldName()), descStr);
	}

	return desc;
}


QMap<unsigned int, QString> PartCategory::getDescriptions(QList<unsigned int> ids)
{
	QMap<unsigned int, QString> descs;

	MultiDataMap data = getValues(ids, getDescriptionProperties());

	for (QList<unsigned int>::iterator it = ids.begin() ; it != ids.end() ; it++) {
		unsigned int id = *it;

		MultiDataMap::iterator dit = data.find(id);

		if (dit == data.end()) {
			descs[id] = tr("(Invalid: %1#%2)").arg(getUserReadableName()).arg(id);
			continue;
		}

		DataMap data = dit.value();

		if (id == INVALID_PART_ID) {
			descs[INVALID_PART_ID] = tr("(Invalid)");
		} else {
			QString desc = getDescription(id, data);
			descs[id] = desc;
		}
	}

	return descs;
}


QString PartCategory::getDescription(unsigned int id)
{
	QMap<unsigned int, QString> descs = getDescriptions(QList<unsigned int>() << id);

	if (descs.empty())
		return tr("(Invalid)");

	return descs[id];
}


void PartCategory::setDescriptionPattern(const QString& descPat)
{
	descPattern = descPat;
}


QList<unsigned int> PartCategory::duplicateRecords(const QList<unsigned int>& ids)
{
	return createRecords(getValues(ids).values());
}


unsigned int PartCategory::duplicateRecord(unsigned int id)
{
	return createRecord(getValues(id));
}
