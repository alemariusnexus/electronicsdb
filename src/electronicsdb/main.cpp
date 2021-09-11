/*
    Copyright 2010-2021 David "Alemarius Nexus" Lerch

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

#include <ctime>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMap>
#include <QSettings>
#include <QStandardPaths>
#include <QStyle>
#include <QTranslator>
#include <gtest/gtest.h>
#include <libhydrogen/hydrogen.h>
#include <nxcommon/exception/Exception.h>
#include <nxcommon/file/File.h>
#include <nxcommon/log.h>
#include "gui/MainWindow.h"
#include "util/metatypes.h"
#include "MainApplication.h"
#include "System.h"

#ifdef EDB_BUILD_TESTS
#include "test/TestManager.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

using namespace electronicsdb;

using std::ifstream;



int main(int argc, char** argv)
{
    try {
#ifdef _WIN32
        // This is required to see console output on Windows even though we compile with the WIN32 CMake option. Without
        // this, things like cmd.exe or IDE consoles won't show anything. Without the FILE_TYPE_UNKNOWN check, the
        // Cygwin terminal won't show anything (presumably because it uses a pipe for the output). This is the only
        // code that seems to work (for now).
        // Fuck Windows.
        DWORD ftype = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
        if (ftype == FILE_TYPE_UNKNOWN) {
            if (AttachConsole(ATTACH_PARENT_PROCESS)) {
                freopen("CONOUT$", "w", stdout);
                freopen("CONOUT$", "w", stderr);
            }
        }
#endif

        hydro_init();

#ifdef EDB_BUILD_TESTS
        testing::InitGoogleTest(&argc, argv);
#endif

        QString appDir = File::getExecutableFile().getParent().getPath().toString();
        QCoreApplication::addLibraryPath(QFileInfo(QDir(appDir), "plugins").absoluteFilePath());

        QString orgName = "alemariusnexus";
        QString appName = "electronicsdb";

        // We need to specify orgName and appName explicitly, because the QApplication isn't setup yet. We also need
        // the settings before setting it up, because some command line options for Qt depend on settings.
        QSettings s(orgName, appName);

        QStringList qtArgs;
        for (int i = 0 ; i < argc ; i++) {
            qtArgs << QString::fromUtf8(argv[i]);
        }

#if defined(_WIN32)  &&  QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (s.value("gui/theme") == "dark") {
            // This causes Qt to honor the system-wide dark mode setting on Windows (including the title bar). It seems
            // to work quite well if we apply our own palette style in System::initTheme() as well. Without our own
            // palette, it looks pretty bad.
            // See also:
            //
            //      https://doc.qt.io/qt-5/qguiapplication.html#platform-specific-arguments
            qtArgs << "-platform" << "windows:darkmode=2";
        }
#endif

        int qtArgc = qtArgs.size();
        char** qtArgv = new char*[qtArgc];

        for (int i = 0 ; i < qtArgc ; i++) {
            QByteArray u8Arg = qtArgs[i].toUtf8();
            qtArgv[i] = new char[u8Arg.size()+1];
            qtArgv[i][u8Arg.size()] = '\0';
            memcpy(qtArgv[i], u8Arg.constData(), u8Arg.size());
        }

        MainApplication app(qtArgc, qtArgv);
        app.setQuitOnLastWindowClosed(false);
        app.setOrganizationName(orgName);
        app.setOrganizationDomain("alemariusnexus.com");
        app.setApplicationName(appName);
        app.setApplicationDisplayName("ElectronicsDB");
        app.setApplicationVersion(EDB_VERSION_STRING);
        app.setWindowIcon(QIcon(":/icons/appicon.png"));

        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir appDataDir(appDataPath);
        if (!appDataDir.exists()) {
            appDataDir.mkpath(".");
        }
        if (!appDataDir.exists()) {
            LogWarning("App data path does not exist: %s", appDataPath.toUtf8().constData());
        } else {
            QFileInfo logFile(appDataPath, "output.log");
            LogDebug("Opening log file '%s'...", logFile.absoluteFilePath().toUtf8().constData());
            QFile(logFile.absoluteFilePath()).remove();
            OpenLogFile(File(logFile.absoluteFilePath()));
        }

        RegisterQtMetaTypes();

#if defined(_WIN32)  &&  QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        // If we didn't use darkmode=2 above, we have to disable Qt's vista theme in dark mode, because that theme seems
        // to ignore some of our palette color, making it look bad. Qt seems to do that automatically with darkmode=2.
        if (s.value("gui/theme") == "dark") {
            if (app.style()->name() == "windowsvista") {
                app.setStyle("Windows");
            }
        }
#endif

        QCommandLineParser cli;
        cli.setApplicationDescription("An electronic components management program");
        cli.addHelpOption();
        cli.addVersionOption();

        QMap<QString, int> logLevels;
        logLevels["error"] = LOG_LEVEL_ERROR;
        logLevels["warning"] = LOG_LEVEL_WARNING;
        logLevels["info"] = LOG_LEVEL_INFO;
        logLevels["debug"] = LOG_LEVEL_DEBUG;
        logLevels["verbose"] = LOG_LEVEL_VERBOSE;

        QCommandLineOption logLevelOpt(QStringList{"L", "loglevel"},
                                       QString("Set the logging level. One of: %1").arg(logLevels.keys().join(", ")),
                                       "LEVEL");
        cli.addOption(logLevelOpt);

#ifdef EDB_BUILD_TESTS
        QCommandLineOption runTestsOpt(QStringList{"tests"}, "Run automatic tests");
        cli.addOption(runTestsOpt);

        QCommandLineOption testCfgOpt(QStringList{"tests-config"}, "Config file for automatic tests", "CFGFILE");
        cli.addOption(testCfgOpt);
#endif

        cli.process(app);


        bool runRegularGUI = true;
        int exitCode = 0;

        if (cli.isSet(logLevelOpt)) {
            if (logLevels.contains(cli.value(logLevelOpt))) {
                SetLogLevel(logLevels[cli.value(logLevelOpt)]);
            } else {
                LogWarning("Invalid log level: %s", cli.value(logLevelOpt).toUtf8().constData());
            }
        }

        System* sys = System::getInstance();

#ifdef EDB_BUILD_TESTS
        if (cli.isSet(runTestsOpt)) {
            runRegularGUI = false;

            // So that QSettings doesn't overwrite the regular app settings for the unit tests
            app.setApplicationName("electronicsdb-test");

            if (!cli.isSet(testCfgOpt)) {
                fprintf(stderr, "ERROR: Missing test config file. Did you forget --tests-config?\n");
                return 1;
            }

            QString cfgPath = cli.value(testCfgOpt);
            if (!QFileInfo(cfgPath).isFile()) {
                fprintf(stderr, "ERROR: Invalid file: %s\n", cfgPath.toUtf8().constData());
                return 1;
            }

            TestManager& testMgr = TestManager::getInstance();

            testMgr.setTestConfigPath(cfgPath);

            exitCode = testMgr.runTests();
        }
#endif

        if (runRegularGUI) {
            QString lang = sys->getActiveLanguage();

            QTranslator qtTrans;

            if (!qtTrans.load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
                if (lang != "en") {
                    (void) qtTrans.load("qt_" + QLocale::system().name(),
                                        QLibraryInfo::location(QLibraryInfo::TranslationsPath));
                }
            }

            app.installTranslator(&qtTrans);

            QTranslator trans;

            if (!trans.load(":/electronics_" + lang)) {
                (void) trans.load(":/electronics_" + QLocale::system().name());
            }

            app.installTranslator(&trans);


            MainWindow* w = new MainWindow;

            exitCode = app.exec();
        }

        LogInfo("Shutting down...");
        System::destroy();

        LogInfo("Shutdown successful.");
        return exitCode;
    } catch (Exception& ex) {
        System::getInstance()->unhandledException(ex);

        return 1;
    }
}
