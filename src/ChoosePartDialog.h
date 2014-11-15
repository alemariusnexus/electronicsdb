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

#ifndef CHOOSEPARTDIALOG_H_
#define CHOOSEPARTDIALOG_H_

#include "global.h"
#include "PartCategory.h"
#include <QtGui/QDialog>



class ChoosePartDialog : public QDialog
{
	Q_OBJECT

public:
	ChoosePartDialog(PartCategory* partCat, QWidget* parent = NULL);
	~ChoosePartDialog();
	unsigned int getChosenID() const { return chosenID; }

private slots:
	void recordChosen(unsigned int id);

private:
	PartCategory* partCat;
	unsigned int chosenID;
};

#endif /* CHOOSEPARTDIALOG_H_ */
