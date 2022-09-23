#pragma once

#include "../global.h"

#include <vector>

#include <QMap>
#include <QString>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "../model/part/Part.h"

namespace electronicsdb
{


class ScriptManager
{
private:
    struct ProviderScript
    {
        QString name;
        QString filePath;
        int luaRef;
    };

public:
    static ScriptManager& getInstance()
    {
        static ScriptManager inst;
        return inst;
    }

public:
    void load();

    QStringList queryPartProductURL(const Part& part);

    QMap<QString, QVariant> queryPartProviderInfo(const Part& part);

public:
    void partToLua(const Part& part);
    Part partFromLua(int idx) const;

    QMap<QString, QVariant> luaTableToMap(int idx) const;

    QVariant luaValueToVariant(int idx) const;
    void variantToLuaValue(const QVariant& v) const;

private:
    void initLua();

    void addLuaPackagePath(const QString& path);

    void loadProviderScripts();

    void loadProviderScript(const QString& filePath);

private:
    lua_State* lua;
    std::vector<ProviderScript> providerScripts;
};


}
