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

#include "GUIStatePersister.h"

#include <nxcommon/log.h>

namespace electronicsdb
{


GUIStatePersister& GUIStatePersister::getInstance()
{
    static GUIStatePersister inst;
    return inst;
}

GUIStatePersister::GUIStatePersister()
{
    settingsSaveTimer.setSingleShot(true);
    settingsSaveTimer.setInterval(100);

    connect(&settingsSaveTimer, &QTimer::timeout, []() {
        QSettings s;
        s.sync();
    });
}

void GUIStatePersister::delayedSaveSettings()
{
    settingsSaveTimer.start();
}

void GUIStatePersister::registerSplitter(const QString& cfgGroup, QSplitter* splitter)
{
    delayedRestore([=]() {
        QSettings s;
        s.beginGroup(cfgGroup);
        restoreState(s, QString("splitter_state_%1").arg(splitter->objectName()), splitter);
        s.endGroup();
    });

    connect(splitter, &QSplitter::splitterMoved, [=]() {
        QSettings s;
        s.beginGroup(cfgGroup);
        saveState(s, QString("splitter_state_%1").arg(splitter->objectName()), splitter);
        s.endGroup();
        delayedSaveSettings();
    });
}

void GUIStatePersister::registerHeaderView(const QString& cfgGroup, QHeaderView* header)
{
    delayedRestore([=]() {
        QSettings s;
        s.beginGroup(cfgGroup);
        restoreState(s, QString("header_state_%1").arg(header->objectName()), header);
        s.endGroup();
    });

    auto saveFunc = [=]() {
        QSettings s;
        s.beginGroup(cfgGroup);
        saveState(s, QString("header_state_%1").arg(header->objectName()), header);
        s.endGroup();
        delayedSaveSettings();
    };
    connect(header, &QHeaderView::geometriesChanged, saveFunc);
    connect(header, &QHeaderView::sectionResized, saveFunc);
}


}
