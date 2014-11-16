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

#ifndef CONNECTIONEDITWIDGET_H_
#define CONNECTIONEDITWIDGET_H_

#include "global.h"
#include <QtGui/QWidget>
#include <ui_ConnectionEditWidget.h>
#include "DatabaseConnection.h"


class ConnectionEditWidget : public QWidget
{
	Q_OBJECT

public:
	ConnectionEditWidget(QWidget* parent = NULL);
	virtual ~ConnectionEditWidget();

	void setConnectionName(const QString& name);
	void setConnectionType(DatabaseConnection::Type type);

	void setMySQLHost(const QString& host);
	void setMySQLPort(unsigned int port);
	void setMySQLDatabaseName(const QString& db);
	void setMySQLUser(const QString& user);

	void setSQLiteFilePath(const QString& path);

	void setFileRootPath(const QString& path);

	void clear();

	QString getConnectionName() const { return ui.connNameField->text(); }
	DatabaseConnection::Type getConnectionType() const
			{ return (DatabaseConnection::Type) ui.connTypeBox->itemData(ui.connTypeBox->currentIndex()).toUInt(); }

	QString getMySQLHost() const { return ui.connServerField->text(); }
	unsigned int getMySQLPort() const { return ui.connPortField->value(); }
	QString getMySQLDatabaseName() const { return ui.connDbField->text(); }
	QString getMySQLUser() const { return ui.connUserField->text(); }

	QString getSQLiteFilePath() const { return ui.connSqliteFileField->text(); }

	QString getFileRootPath() const { return ui.connFileRootField->text(); }

	void setPasswordEntryEnabled(bool enabled);

protected:
	void changeEvent(QEvent* evt);

private slots:
	void connTypeBoxActivated(int idx);
	void connSqliteFileChooseButtonClicked();
	void connFileRootPathChooseButtonClicked();

signals:
	void connectionNameChanged(const QString& name);

private:
	Ui_ConnectionEditWidget ui;
};

#endif /* CONNECTIONEDITWIDGET_H_ */
