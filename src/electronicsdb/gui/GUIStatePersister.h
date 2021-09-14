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

#pragma once

#include "../global.h"

#include <QHeaderView>
#include <QObject>
#include <QSettings>
#include <QSplitter>
#include <QTimer>

namespace electronicsdb
{


class GUIStatePersister : public QObject
{
    Q_OBJECT

private:
    static constexpr int RestoreDelay = 100;

public:
    static GUIStatePersister& getInstance();

public:
    void registerSplitter(const QString& cfgGroup, QSplitter* splitter);
    void registerHeaderView(const QString& cfgGroup, QHeaderView* header);

private:
    GUIStatePersister();

    void delayedSaveSettings();

    template <typename Func>
    void delayedRestore(const Func& func)
    {
        QTimer::singleShot(RestoreDelay, func);
    }

    template <typename Object>
    void saveState(QSettings& s, const QString& key, Object* obj)
    {
        s.setValue(key, obj->saveState());
    }

    template <typename Object>
    void saveGeometry(QSettings& s, const QString& key, Object* obj)
    {
        s.setValue(key, obj->saveGeometry());
    }

    template <typename Object>
    void restoreState(const QSettings& s, const QString& key, Object* obj)
    {
        obj->restoreState(s.value(key).toByteArray());
    }

    template <typename Object>
    void restoreGeometry(const QSettings& s, const QString& key, Object* obj)
    {
        obj->restoreGeometry(s.value(key).toByteArray());
    }

private:
    QTimer settingsSaveTimer;
};


}
