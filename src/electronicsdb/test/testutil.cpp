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

#include "testutil.h"

#include <QSqlRecord>
#include <nxcommon/log.h>

namespace electronicsdb
{


QMap<QString, QVariant> QueryRowToMap(const QSqlQuery& query)
{
    QMap<QString, QVariant> rowData;
    QSqlRecord rec = query.record();
    for (int i = 0 ; i < rec.count() ; i++) {
        rowData[rec.fieldName(i)] = query.value(rec.fieldName(i));
    }
    return rowData;
}

int ExpectQueryRowResult (
        QSqlQuery& query,
        const QList<QMap<QString, QVariant>>& expRowCands,
        bool allowMoreFields
) {
    QSqlRecord rec = query.record();

    bool match;

    int candIdx = 0;
    for (const auto& expRow : expRowCands) {
        match = true;

        for (auto it = expRow.begin() ; it != expRow.end() ; ++it) {
            const QString& key = it.key();
            const QVariant& val = it.value();

            if (!rec.contains(key)) {
                match = false;
                break;
            }
            if (query.value(key) != val) {
                match = false;
                break;
            }
        }

        if (match) {
            if (!allowMoreFields) {
                for (int i = 0 ; i < rec.count() ; i++) {
                    const QString& key = rec.fieldName(i);

                    if (!expRow.contains(key)) {
                        match = false;
                        break;
                    }
                }
            }
        }

        if (match) {
            break;
        }

        candIdx++;
    }

    if (!match) {
        QMap<QString, QVariant> rowData = QueryRowToMap(query);
        ADD_FAILURE() << "Row doesn't match any expected row: " << rowData;

        return -1;
    }

    return candIdx;
}

void ExpectQueryResult (
        QSqlQuery& query,
        const QList<QMap<QString, QVariant>>& expRows,
        bool sameOrder,
        bool allowMoreFields
) {
    auto expRowsLeft = expRows;

    int rowIdx = 0;
    while (query.next()) {
        if (expRowsLeft.empty()) {
            ADD_FAILURE() << "Unexpected result row: " << QueryRowToMap(query);
            continue;
        }

        if (sameOrder) {
            int matchIdx = ExpectQueryRowResult(query, QList<QMap<QString, QVariant>>() << expRowsLeft[0], allowMoreFields);
            expRowsLeft.removeAt(0);
        } else {
            int matchIdx = ExpectQueryRowResult(query, expRowsLeft, allowMoreFields);
            if (matchIdx >= 0) {
                expRowsLeft.removeAt(matchIdx);
            }
        }

        rowIdx++;
    }

    if (!expRowsLeft.empty()) {
        for (const auto& data : expRowsLeft) {
            ADD_FAILURE() << "Expected row not found: " << data;
        }
    }
}

void ExpectValidIDs(const QList<dbid_t>& ids, bool allDifferent)
{
    for (auto it = ids.begin() ; it != ids.end() ; ++it) {
        EXPECT_GE(*it, 0);

        if (allDifferent) {
            for (auto iit = ids.begin() ; iit != ids.end() ; ++iit) {
                if (it != iit) {
                    EXPECT_NE(*it, *iit);
                }
            }
        }
    }
}


}
