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

#ifndef TASK_H_
#define TASK_H_

#include "global.h"
#include <QtCore/QObject>
#include <QtCore/QString>



class Task : public QObject
{
	Q_OBJECT

public:
	Task(const QString& desc, int min = 0, int max = 100);

	void setValue(int val);
	void increaseValue(int inc);
	void update();

	QString getDescription() const { return desc; }
	int getMinimum() const { return min; }
	int getMaximum() const { return max; }
	int getValue() const { return value; }

signals:
	void updated(int val);

private:
	QString desc;
	int min;
	int max;
	int value;
};

#endif /* TASK_H_ */
