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
#include <mysql/mysql.h>
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


const char* TestRecords[] = {
		"'avr', 'AVR', 'ATmega128', 16e6, 24, 4096, 1024, 512, 75.0, 1.8, 6.0, 'PDIP-32', 'Atmel', 3, 'This is an ATmega128', 15",
		"'avr', 'AVR', 'ATmega2550', 20e6, 40, 16384, 8192, 2048, 85.0, 3.3, 5.0, 'TQFP-54', 'Atmel', 4, 'This is an ATmega2550', 4",
		"'arm', 'Cortex-A8', 'TMS1337', 700e6, 120, 0, 1048576000, 0, 75.0, 3.3, 3.3, 'TBGA-160', 'Texas Instruments', 5, 'This is powerful!', 2"
};



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

		QString lang = s.value("main/lang", QLocale::system().name()).toString();

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


#if 0
		srand(time(NULL));

		unsigned int numMax = 50;
		unsigned int numBlock = 1000;

		for (unsigned int i = 0 ; i < numMax ; i++) {
			printf("Inserting %u+%u / %u...\n", i*numBlock, numBlock, numMax*numBlock);

			QString query = QString(
					"INSERT INTO microcontroller (arch, processor, model, freqmax, iocount, progmemsize, ramsize, eepromsize, "
					"tempmax, vmin, vmax, package, vendor, datasheet, notes, numstock) VALUES "
					);

			for (unsigned int j = 0 ; j < numBlock ; j++) {
				if (j != 0)
					query += ", ";
				query += QString("(%1)").arg(TestRecords[rand() % (sizeof(TestRecords)/sizeof(const char*))]);
			}

			if (mysql_query(mysql, query.toUtf8().constData())  !=  0) {
				printf("ERROR: %s\n", mysql_error(mysql));
				return 1;
			}
		}

		return 0;
#else
		w = new MainWindow;
#endif

		int retval = app.exec();

		System::destroy();

		return retval;
    } catch (Exception& ex) {
    	System::getInstance()->unhandeledException(ex);

    	return 1;
    }
}
