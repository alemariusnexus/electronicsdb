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

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QWidget>

namespace electronicsdb
{


class KeyVault : public QObject
{
    Q_OBJECT

public:
    static KeyVault& getInstance();

public:
    bool isSetUp() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    void setVaultPassword(const QString& newPw, const QString& oldPw = QString());
    bool setVaultPasswordWithUserPrompt(const QString& newPw, QWidget* parent = nullptr, int maxRetries = -1);

    bool isVaultKeyed() const;
    bool isVaultDefaultKeyed() const;
    int getKeyCount() const;

    bool containsKey(const QString& id) const;

    void setKey(const QString& id, const QString& key);
    bool setKeyMaybeWithUserPrompt(const QString& id, const QString& key, QWidget* parent = nullptr);

    QString getKey(const QString& id, const QString& vaultPw);
    QString getKeyMaybeWithUserPrompt(const QString& id, QWidget* parent = nullptr, int maxRetries = -1);

    void clear(bool clearMasterKey = false);

    void wipeKey(QString& key);

private:
    KeyVault();

    QJsonObject load() const;
    void save(const QJsonObject& vault);

    void wipeEntry(QJsonObject& jentry);

    void deriveKey(uint8_t* key, const QString& vaultPassword) const;

    QString bin2hex(const QByteArray& data) const;
    QByteArray hex2bin(const QString& data) const;

    void encrypt(QJsonObject& vault, const QString& id, const QString& key);
    QString decrypt(const QJsonObject& vault, const QString& id, const QString& vaultPw) const;
};


}
