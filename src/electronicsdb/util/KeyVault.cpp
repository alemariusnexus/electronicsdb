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

#include "KeyVault.h"

#include <cstring>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QThread>
#include <libhydrogen/hydrogen.h>
#include <nxcommon/exception/InvalidStateException.h>
#include <nxcommon/log.h>
#include "../exception/InvalidCredentialsException.h"
#include "../gui/settings/KeyVaultDialog.h"

namespace electronicsdb
{


static const uint8_t HydroMasterKey[hydro_pwhash_MASTERKEYBYTES] = {
        0x66, 0x4D, 0x91, 0x3E, 0x3D, 0x67, 0xF6, 0xAD, 0x52, 0xB7, 0x20, 0x6F, 0x42, 0x64, 0x1F, 0x08,
        0x18, 0x30, 0x75, 0xBC, 0x6E, 0xE0, 0x88, 0xE8, 0x79, 0x12, 0x12, 0x64, 0x3D, 0xAF, 0x02, 0x44
};

static const uint8_t VaultDefaultKey[hydro_secretbox_KEYBYTES] = {
        0x07, 0x9F, 0xCC, 0xBF, 0xDC, 0x46, 0x01, 0xDE, 0x91, 0x1A, 0xC7, 0xC1, 0x2A, 0xAD, 0x2C, 0x70,
        0x8B, 0x6B, 0xAD, 0x53, 0xA9, 0x4F, 0x94, 0xC3, 0x8F, 0xBD, 0x67, 0x02, 0x39, 0x97, 0xBB, 0x50
};



KeyVault& KeyVault::getInstance()
{
    static KeyVault inst;
    return inst;
}


KeyVault::KeyVault()
{
}

bool KeyVault::isSetUp() const
{
    QJsonObject vault = load();
    return vault.contains("enabled");
}

bool KeyVault::isEnabled() const
{
    QJsonObject vault = load();
    return vault.value("enabled").toBool(false);
}

void KeyVault::setEnabled(bool enabled)
{
    QJsonObject vault = load();
    vault.insert("enabled", enabled);
    save(vault);
}

void KeyVault::setVaultPassword(const QString& newPw, const QString& oldPw)
{
    bool enabledBefore = isEnabled();

    // Generate vault keys
    hydro_kx_keypair vaultKeys;
    hydro_kx_keygen(&vaultKeys);

    // Derive key for the private vault key from user password
    uint8_t secretKeyEncKey[hydro_secretbox_KEYBYTES];
    deriveKey(secretKeyEncKey, newPw);

    // Encrypt private vault key
    const size_t secretKeyEncLen = hydro_kx_SECRETKEYBYTES + hydro_secretbox_HEADERBYTES;
    uint8_t secretKeyEnc[secretKeyEncLen];
    if (hydro_secretbox_encrypt(secretKeyEnc, vaultKeys.sk, sizeof(vaultKeys.sk), 0, "vaultkey", secretKeyEncKey) != 0) {
        hydro_memzero(secretKeyEncKey, sizeof(secretKeyEncKey));
        throw InvalidStateException("Error encrypting vault key", __FILE__, __LINE__);
    }

    // Wipe key for private vault key
    hydro_memzero(secretKeyEncKey, sizeof(secretKeyEncKey));

    // Create new vault
    QJsonObject vault;
    vault.insert("format", "hydro1");
    vault.insert("vkPub", bin2hex(QByteArray(reinterpret_cast<const char*>(vaultKeys.pk),
                                             static_cast<int>(sizeof(vaultKeys.pk)))));
    vault.insert("vkPriv", bin2hex(QByteArray(reinterpret_cast<const char*>(secretKeyEnc),
                                              static_cast<int>(sizeof(secretKeyEnc)))));
    vault.insert("keyType", newPw.isNull() ? "default" : "user");
    vault.insert("entries", QJsonArray());

    // Wipe vault keys. They will be read from the JSON vault from now on.
    hydro_memzero(&vaultKeys, sizeof(vaultKeys));

    // Transfer keys from old to new vault, reencrypting them
    QJsonObject oldVault = load();
    for (const QJsonValueRef& jval : oldVault.value("entries").toArray()) {
        QJsonObject jentry = jval.toObject();

        QString id = jentry.value("id").toString();
        QString key = decrypt(oldVault, id, oldPw);

        encrypt(vault, id, key);

        wipeKey(key);
        wipeEntry(jentry);
    }

    save(vault);

    setEnabled(enabledBefore);
}

bool KeyVault::setVaultPasswordWithUserPrompt(const QString& newPw, QWidget* parent, int maxRetries)
{
    if (!isVaultKeyed()  ||  isVaultDefaultKeyed()) {
        setVaultPassword(newPw);
        return true;
    }

    bool success = false;

    for (int i = 0 ; !success && (maxRetries < 0 || i < maxRetries) ; i++) {
        QString vaultPw = QInputDialog::getText(parent, tr("Unlock Key Vault"),
                                                tr("Please enter your vault master password:"),
                                                QLineEdit::Password);
        if (vaultPw.isNull()) {
            // Rejected by user
            return false;
        }

        try {
            setVaultPassword(newPw, vaultPw);
            success = true;
        } catch (InvalidStateException&) {
            // Nothing to do, just retry
        }

        wipeKey(vaultPw);
    }

    return success;
}

bool KeyVault::isVaultKeyed() const
{
    QJsonObject vault = load();
    return vault.contains("vkPub")  &&  vault.contains("vkPriv");
}

bool KeyVault::isVaultDefaultKeyed() const
{
    QJsonObject vault = load();
    return vault.contains("vkPub")  &&  vault.contains("vkPriv")  &&  vault.value("keyType") != "user";
}

int KeyVault::getKeyCount() const
{
    QJsonObject vault = load();
    return vault.value("entries").toArray(QJsonArray()).size();
}

bool KeyVault::containsKey(const QString& id) const
{
    QJsonObject vault = load();
    for (const QJsonValueRef& jval : vault.value("entries").toArray(QJsonArray())) {
        QJsonObject jentry = jval.toObject();

        if (jentry.value("id").toString() == id) {
            return true;
        }
    }

    return false;
}

void KeyVault::setKey(const QString& id, const QString& key)
{
    QJsonObject vault = load();
    encrypt(vault, id, key);
    save(vault);
}

bool KeyVault::setKeyMaybeWithUserPrompt(const QString& id, const QString& key, QWidget* parent)
{
    if (!isSetUp()  ||  (isEnabled()  &&  !isVaultKeyed())) {
        KeyVaultDialog dlg(parent);
        dlg.exec();
    }

    if (isEnabled()  &&  isVaultKeyed()) {
        setKey(id, key);
        return true;
    }

    return false;
}

QString KeyVault::getKey(const QString& id, const QString& vaultPw)
{
    QJsonObject vault = load();
    return decrypt(vault, id, vaultPw);
}

QString KeyVault::getKeyMaybeWithUserPrompt(const QString& id, QWidget* parent, int maxRetries) {
    if (!isEnabled()  ||  !isVaultKeyed()) {
        return QString();
    }

    if (isVaultDefaultKeyed()) {
        return getKey(id, QString());
    } else {
        for (int i = 0 ; maxRetries < 0 || i < maxRetries ; i++) {
            QString vaultPw = QInputDialog::getText(parent, tr("Unlock Key Vault"),
                                                    tr("Please enter the password to unlock the key vault:"),
                                                    QLineEdit::Password);
            if (vaultPw.isNull()) {
                return QString();
            }

            QString key;

            try {
                key = getKey(id, vaultPw);
            } catch (InvalidCredentialsException&) {
                QThread::msleep(1000);
            }

            wipeKey(vaultPw);

            if (!key.isNull()) {
                return key;
            }
        }
    }

    return QString();
}

void KeyVault::clear(bool clearMasterKey)
{
    QJsonObject vault = load();

    if (clearMasterKey) {
        vault.remove("format");
        vault.remove("keyType");
        vault.remove("vkPub");
        vault.remove("vkPriv");
        vault.remove("entries");
        save(QJsonObject());
    } else {
        if (vault.contains("entries")) {
            vault.remove("entries");
            vault.insert("entries", QJsonArray());
        }
    }

    save(vault);
}

QJsonObject KeyVault::load() const
{
    QSettings s;

    QString vaultStr = s.value("main/key_vault", "{}").toString();

    QJsonDocument jdoc = QJsonDocument::fromJson(vaultStr.toUtf8());
    return jdoc.object();
}

void KeyVault::save(const QJsonObject& vault)
{
    QSettings s;

    QJsonDocument jdoc;
    jdoc.setObject(vault);

    QString vaultStr = QString::fromUtf8(jdoc.toJson(QJsonDocument::Indented));

    s.setValue("main/key_vault", vaultStr);

    s.sync();
}

void KeyVault::wipeKey(QString& key)
{
    QChar* keyData = const_cast<QChar*>(key.constData());
    size_t keyDataSize = key.length()*2;

    hydro_memzero(keyData, keyDataSize);
}

void KeyVault::wipeEntry(QJsonObject& jentry)
{
    QString key = jentry.value("key").toString();
    wipeKey(key);
}

void KeyVault::deriveKey(uint8_t* key, const QString& vaultPassword) const
{
    if (vaultPassword.isNull()) {
        memcpy(key, VaultDefaultKey, hydro_secretbox_KEYBYTES);
        return;
    }

    int res = hydro_pwhash_deterministic(key, hydro_secretbox_KEYBYTES,
                                         reinterpret_cast<const char*>(vaultPassword.constData()),
                                         vaultPassword.length()*2, "keyvault", HydroMasterKey, 10000, 0, 1);
    if (res != 0) {
        throw InvalidStateException("Error deriving vault key from password", __FILE__, __LINE__);
    }
}

QString KeyVault::bin2hex(const QByteArray& data) const
{
    size_t hexBufLen = static_cast<size_t>(data.length()*2 + 1);
    char* hexBuf = new char[hexBufLen];
    char* res = hydro_bin2hex(hexBuf, hexBufLen, reinterpret_cast<const uint8_t*>(data.constData()), data.length());
    assert(res);
    QString hex = QString::fromUtf8(hexBuf, static_cast<int>(hexBufLen-1));
    delete[] hexBuf;
    return hex;
}

QByteArray KeyVault::hex2bin(const QString& data) const
{
    QByteArray hexUtf8 = data.toUtf8();
    size_t binLen = (data.length()+1) / 2;
    uint8_t* bin = new uint8_t[binLen];
    int actualBinLen = hydro_hex2bin(bin, binLen, hexUtf8.constData(), hexUtf8.length(), nullptr, nullptr);
    assert(actualBinLen >= 0);
    QByteArray barr(reinterpret_cast<const char*>(bin), actualBinLen);
    delete[] bin;
    return barr;
}

void KeyVault::encrypt(QJsonObject& vault, const QString& id, const QString& key)
{
    if (!isVaultKeyed()) {
        throw InvalidStateException("Can't encrypt in a non-keyed vault", __FILE__, __LINE__);
    }

    QByteArray vkPub = hex2bin(vault.value("vkPub").toString());
    assert(vkPub.length() == hydro_kx_PUBLICKEYBYTES);

    uint8_t packet[hydro_kx_N_PACKET1BYTES];
    hydro_kx_session_keypair sessKeys;
    if (hydro_kx_n_1(&sessKeys, packet, nullptr, reinterpret_cast<const uint8_t*>(vkPub.constData())) != 0) {
        throw InvalidStateException("Error deriving vault session key from public key", __FILE__, __LINE__);
    }

    size_t plainLen = static_cast<size_t>(key.length()*2);
    size_t cipherLen = plainLen + hydro_secretbox_HEADERBYTES;
    uint8_t* cipher = new uint8_t[cipherLen];
    if (hydro_secretbox_encrypt(cipher, key.constData(), plainLen, 0, "keyvault", sessKeys.tx) != 0) {
        delete[] cipher;
        throw InvalidStateException("Error encrypting key in vault", __FILE__, __LINE__);
    }

    hydro_memzero(&sessKeys, sizeof(sessKeys));

    QJsonObject jentry;
    jentry.insert("id", id);
    jentry.insert("key", bin2hex(QByteArray::fromRawData(reinterpret_cast<const char*>(cipher),
                                                         static_cast<int>(cipherLen))));
    jentry.insert("packet", bin2hex(QByteArray(reinterpret_cast<const char*>(packet),
                                           static_cast<int>(sizeof(packet)))));
    jentry.insert("format", "hydro1");

    delete[] cipher;

    // No! Ownership was taken by QByteArray above!
    // delete[] cipher;

    QJsonArray jentries = vault.value("entries").toArray(QJsonArray());

    bool replaced = false;

    //for (const QJsonValueRef& jval : jentries) {
    for (int i = 0 ; i < jentries.size() ; i++) {
        QJsonObject jOldEntry = jentries[i].toObject();

        if (jOldEntry.value("id").toString() == id) {
            jentries.replace(i, jentry);
            replaced = true;
            wipeEntry(jOldEntry);
            break;
        }
    }

    if (!replaced) {
        jentries.append(jentry);
    }

    vault.insert("entries", jentries);
}

QString KeyVault::decrypt(const QJsonObject& vault, const QString& id, const QString& vaultPw) const
{
    if (!isVaultKeyed()) {
        throw InvalidStateException("Can't decrypt in a non-keyed vault", __FILE__, __LINE__);
    }

    QJsonObject jentry;
    for (const QJsonValueRef& jval : vault.value("entries").toArray(QJsonArray())) {
        QJsonObject jentryCand = jval.toObject();

        if (jentryCand.value("id").toString() == id) {
            jentry = jentryCand;
            break;
        }
    }

    if (jentry.isEmpty()) {
        throw InvalidStateException("Key not found in vault", __FILE__, __LINE__);
    }

    QString format = jentry.value("format").toString();

    if (format != "hydro1") {
        throw InvalidStateException("Unsupported vault entry format", __FILE__, __LINE__);
    }

    QString keyType = vault.value("keyType").toString();

    hydro_kx_keypair vaultKeys;

    QByteArray vkPub = hex2bin(vault.value("vkPub").toString());
    assert(vkPub.length() == hydro_kx_PUBLICKEYBYTES);
    memcpy(vaultKeys.pk, vkPub.constData(), sizeof(vaultKeys.pk));

    QByteArray vkPrivEnc = hex2bin(vault.value("vkPriv").toString());
    size_t vkPrivEncLen = static_cast<size_t>(hydro_kx_SECRETKEYBYTES + hydro_secretbox_HEADERBYTES);
    assert(vkPrivEnc.length() == vkPrivEncLen);

    uint8_t vkPrivEncKey[hydro_secretbox_KEYBYTES];
    deriveKey(vkPrivEncKey, (keyType == "user") ? vaultPw : QString());

    if (hydro_secretbox_decrypt(vaultKeys.sk, reinterpret_cast<const uint8_t*>(vkPrivEnc.constData()), vkPrivEncLen, 0,
                                "vaultkey", vkPrivEncKey) != 0
    ) {
        throw InvalidCredentialsException("Error decrypting vault key", __FILE__, __LINE__);
    }

    hydro_memzero(vkPrivEncKey, sizeof(vkPrivEncKey));

    QByteArray packet = hex2bin(jentry.value("packet").toString());

    hydro_kx_session_keypair sessKeys;
    if (hydro_kx_n_2(&sessKeys, reinterpret_cast<const uint8_t*>(packet.constData()), nullptr, &vaultKeys) != 0) {
        hydro_memzero(&vaultKeys, sizeof(vaultKeys));
        throw InvalidStateException("Error deriving vault session key from packet and private key",
                                    __FILE__, __LINE__);
    }

    hydro_memzero(&vaultKeys, sizeof(vaultKeys));

    QByteArray cipher = hex2bin(jentry.value("key").toString());
    if (cipher.length() < hydro_secretbox_HEADERBYTES) {
        hydro_memzero(&sessKeys, sizeof(sessKeys));
        assert(false);
    }

    size_t keyLen = static_cast<size_t>(cipher.length() - hydro_secretbox_HEADERBYTES);
    uint8_t* keyRaw = new uint8_t[keyLen];

    if (hydro_secretbox_decrypt(keyRaw, reinterpret_cast<const uint8_t*>(cipher.constData()), cipher.length(), 0,
                            "keyvault", sessKeys.rx) != 0
    ) {
        hydro_memzero(keyRaw, keyLen);
        hydro_memzero(&sessKeys, sizeof(sessKeys));
        delete[] keyRaw;
        throw InvalidStateException("Error decrypting key from vault", __FILE__, __LINE__);
    }

    assert(keyLen%2 == 0);

    QString key(reinterpret_cast<const QChar*>(keyRaw), static_cast<int>(keyLen/2));

    hydro_memzero(keyRaw, keyLen);
    delete[] keyRaw;

    hydro_memzero(&sessKeys, sizeof(sessKeys));

    return key;
}


}
