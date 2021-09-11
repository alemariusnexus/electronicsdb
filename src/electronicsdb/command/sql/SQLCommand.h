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

#include "../../global.h"

#include <functional>
#include <unordered_set>
#include <vector>
#include <QList>
#include <QMap>
#include <QSqlDatabase>
#include <QVariant>

namespace electronicsdb
{


class SQLCommand
{
public:
    enum Operation
    {
        OpCommit,
        OpRevert,

        OpInvalid
    };

    using Listener = std::function<void (SQLCommand*, Operation)>;

public:
    SQLCommand();
    virtual ~SQLCommand() {}
    void commit();
    void revert();

    void setListener(const Listener& listener) { this->listener = listener; }

    bool hasBeenCommitted() const { return committed; }
    bool hasBeenReverted() const { return reverted; }

protected:
    virtual QSqlDatabase getSQLConnection();

    virtual void doCommit() = 0;
    virtual void doRevert() = 0;

private:
    Listener listener;
    bool committed;
    bool reverted;
};


}
