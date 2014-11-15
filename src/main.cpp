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

#include "global.h"
#include <QtGui/QWidget>
#include <QtGui/QApplication>
#include <QtCore/QSettings>
#include <QtCore/QTranslator>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>
#include "MainWindow.h"
#include "FilterWidget.h"
#include "elutil.h"
#include "PartCategory.h"
#include "System.h"
#include <nxcommon/exception/Exception.h>
#include "MainApplication.h"
#include <cstdio>
#include <fstream>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QSplitter>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#include <ctime>

#include "SQLInsertCommand.h"
#include "SQLMultiValueInsertCommand.h"
#include "SQLDeleteCommand.h"
#include "SQLUpdateCommand.h"

using std::ifstream;




int main(int argc, char** argv)
{
    MainApplication app(argc, argv);

    app.setQuitOnLastWindowClosed(false);

    MainWindow* w = NULL;

    try {
		System* sys = System::getInstance();

		app.setOrganizationName("alemariusnexus");
		app.setApplicationName("electronics");

		QSettings s;

		//QString lang = s.value("main/lang", QLocale::system().name()).toString();
		QString lang = sys->getActiveLanguage();

		QTranslator qtTrans;

		if (!qtTrans.load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
			if (lang != "en")
				qtTrans.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
		}

		app.installTranslator(&qtTrans);

		QTranslator trans;

		if (!trans.load(":/electronics_" + lang)) {
			trans.load(":/electronics_" + QLocale::system().name());
		}

		app.installTranslator(&trans);


		w = new MainWindow;

		int retval = app.exec();

		System::destroy();

		return retval;
    } catch (Exception& ex) {
    	System::getInstance()->unhandeledException(ex);

    	return 1;
    }
}
