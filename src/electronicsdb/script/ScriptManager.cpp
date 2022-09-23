#include "ScriptManager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QVariant>

#include <nxcommon/log.h>

#include "../model/part/PartFactory.h"
#include "../model/PartCategory.h"
#include "../System.h"


namespace electronicsdb
{


int LuaPartGetPropertyValue(lua_State* lua)
{
    auto& sm = ScriptManager::getInstance();

    Part part = sm.partFromLua(1);
    if (!part.isValid()) {
        lua_pushnil(lua);
        return 1;
    }

    const char* propID = luaL_checkstring(lua, 2);
    PartProperty* prop = part.getPartCategory()->getProperty(QString::fromUtf8(propID));
    if (!prop) {
        lua_pushnil(lua);
        return 1;
    }

    const auto& value = part.getData(prop);
    sm.variantToLuaValue(value);
    return 1;
}



void ScriptManager::load()
{
    initLua();

    loadProviderScripts();
}

void ScriptManager::initLua()
{
    lua = luaL_newstate();

    luaL_openlibs(lua);

    const QStringList packagePaths = {
            QString("%1/../lua/common/?.lua").arg(qApp->applicationDirPath()),
            QString("%1/../share/lua/common/?.lua").arg(qApp->applicationDirPath())
    };

    for (auto& path : packagePaths) {
        addLuaPackagePath(path);
    }


    luaL_Reg luaCFuncs[] = {
            {   "PartGetPropertyValue",             &LuaPartGetPropertyValue                },

            {   NULL,                               NULL                                    }
    };

    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    luaL_setfuncs(lua, luaCFuncs, 0);
    lua_pop(lua, 1); // globals table
}

void ScriptManager::addLuaPackagePath(const QString& path)
{
    lua_getglobal(lua, "package");
    lua_getfield(lua, -1, "path");

    QString fullPath = QString::fromUtf8(lua_tostring(lua, -1));
    fullPath.append(";").append(path);
    lua_pop(lua, 1); // package.path

    lua_pushstring(lua, fullPath.toUtf8().constData());
    lua_setfield(lua, -2, "path");

    lua_pop(lua, 1); // package
}

void ScriptManager::loadProviderScripts()
{
    const QStringList scriptPaths = {
            QString("%1/../lua/provider").arg(qApp->applicationDirPath()),
            QString("%1/../share/lua/provider").arg(qApp->applicationDirPath())
    };

    for (auto& scriptPath : scriptPaths) {
        QDir scriptDir(scriptPath);

        for (auto finfo : scriptDir.entryInfoList({"*.lua", "*.luac"}, QDir::Files | QDir::Readable, QDir::Name)) {
            loadProviderScript(finfo.filePath());
        }
    }
}

void ScriptManager::loadProviderScript(const QString& filePath)
{
    QFileInfo finfo(filePath);

    LogInfo("Loading provider script '%s'.", finfo.filePath().toUtf8().constData());

    if (luaL_loadfile(lua, finfo.filePath().toUtf8().constData()) != LUA_OK) {
        const char* errmsg = lua_tostring(lua, -1);
        LogError("Error loading provider script '%s': %s", finfo.filePath().toUtf8().constData(), errmsg);
        lua_pop(lua, 1);
        return;
    }

    if (lua_pcall(lua, 0, 1, 0) != LUA_OK) {
        const char* errmsg = lua_tostring(lua, -1);
        LogError("Error running provider script '%s': %s", finfo.filePath().toUtf8().constData(), errmsg);
        lua_pop(lua, 1);
        return;
    }

    auto& script = providerScripts.emplace_back();
    script.name = finfo.baseName();
    script.filePath = finfo.filePath();
    script.luaRef = luaL_ref(lua, LUA_REGISTRYINDEX);
}

QStringList ScriptManager::queryPartProductURL(const Part& part)
{
    partToLua(part); // part
    lua_newtable(lua); // urls
    int argsTop = lua_gettop(lua);

    for (auto& script : providerScripts) {
        lua_rawgeti(lua, LUA_REGISTRYINDEX, script.luaRef);

        lua_getfield(lua, -1, "queryPartProductURL");
        if (lua_isfunction(lua, -1)) {
            lua_pushvalue(lua, argsTop-1);
            lua_pushvalue(lua, argsTop);

            if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
                const char* errmsg = lua_tostring(lua, -1);
                LogError("Error calling queryPartProductURL() for provider script '%s': %s",
                         script.name.toUtf8().constData(), errmsg);
                lua_pop(lua, 1);
            }
        } else {
            lua_pop(lua, 1); // function queryPartProductURL
        }

        lua_pop(lua, 1); // providerScript
    }

    QStringList urls = luaValueToVariant(-1).toStringList();
    lua_pop(lua, 2); // urls, part

    return urls;
}

QMap<QString, QVariant> ScriptManager::queryPartProviderInfo(const Part& part)
{
    partToLua(part); // part
    lua_newtable(lua); // info
    int argsTop = lua_gettop(lua);

    for (auto& script : providerScripts) {
        lua_rawgeti(lua, LUA_REGISTRYINDEX, script.luaRef);

        lua_getfield(lua, -1, "queryPartInfo");
        if (lua_isfunction(lua, -1)) {
            lua_pushvalue(lua, argsTop-1);
            lua_pushvalue(lua, argsTop);

            if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
                const char* errmsg = lua_tostring(lua, -1);
                LogError("Error calling queryPartInfo() for provider script '%s': %s",
                         script.name.toUtf8().constData(), errmsg);
                lua_pop(lua, 1);
            }
        } else {
            lua_pop(lua, 1); // function queryPartInfo
        }

        lua_pop(lua, 1); // providerScript
    }

    auto info = luaTableToMap(argsTop);

    lua_pop(lua, 2); // info, part

    return info;
}

void ScriptManager::partToLua(const Part& part)
{
    lua_createtable(lua, 0, 2);

    lua_pushstring(lua, part.getPartCategory()->getID().toUtf8().constData());
    lua_setfield(lua, -2, "pcat");

    lua_pushnumber(lua, static_cast<lua_Number>(part.getID()));
    lua_setfield(lua, -2, "pid");
}

Part ScriptManager::partFromLua(int idx) const
{
    lua_getfield(lua, idx, "pcat");
    const char* pcatID = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    if (!pcatID) {
        return Part();
    }
    PartCategory* pcat = System::getInstance()->getPartCategory(QString::fromUtf8(pcatID));
    if (!pcat) {
        return Part();
    }

    lua_getfield(lua, idx, "pid");
    dbid_t pid = lua_tonumber(lua, -1);
    lua_pop(lua, 1);

    return PartFactory::getInstance().findPartByID(pcat, pid);
}

QMap<QString, QVariant> ScriptManager::luaTableToMap(int idx) const
{
    QVariant varVal = luaValueToVariant(idx);

    if (varVal.canConvert<QMap<QString, QVariant>>()) {
        return varVal.toMap();
    }

    return {};
}

QVariant ScriptManager::luaValueToVariant(int idx) const
{
    int type = lua_type(lua, idx);

    switch (type) {
    case LUA_TNIL:
        return {};
    case LUA_TNUMBER:
        return lua_tonumber(lua, idx);
    case LUA_TBOOLEAN:
        return lua_toboolean(lua, idx) ? true : false;
    case LUA_TSTRING:
        return QString::fromUtf8(lua_tostring(lua, idx));
    case LUA_TTABLE:
        {
            int tblIdx = lua_absindex(lua, idx);

            // Check if it's an array or a map
            bool arr = true;
            int arrIdx = 0;
            lua_pushnil(lua); // first key
            while (lua_next(lua, tblIdx) != 0) {
                lua_pop(lua, 1); // value

                lua_pushinteger(lua, ++arrIdx); // array index
                lua_gettable(lua, tblIdx); // array index
                if (lua_isnil(lua, -1)) {
                    arr = false;
                    lua_pop(lua, 2); // array index, key
                    break;
                }
                lua_pop(lua, 1); // array index
            }

            if (arr) {
                QList<QVariant> list;

                lua_pushnil(lua); // first key
                while (lua_next(lua, tblIdx) != 0) {
                    list << luaValueToVariant(-1);
                    lua_pop(lua, 1); // value
                }

                return list;
            } else {
                QMap<QString, QVariant> map;

                lua_pushnil(lua); // first key
                while (lua_next(lua, tblIdx) != 0) {
                    map.insert(QString::fromUtf8(lua_tostring(lua, -2)), luaValueToVariant(-1));
                    lua_pop(lua, 1); // value
                }

                return map;
            }
        }
        break;
    default:
        return {};
    }
}

void ScriptManager::variantToLuaValue(const QVariant& v) const
{
    int mtype;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mtype = (int) v.type();
#else
    mtype = v.metaType().id();
#endif

    switch (mtype) {
    case QMetaType::QString:
    {
        QByteArray str = v.toString().toUtf8();
        lua_pushlstring(lua, str.constData(), str.size());
        break;
    }
    case QMetaType::Bool:
        lua_pushboolean(lua, v.toBool());
        break;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Double:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::Short:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
    case QMetaType::UShort:
    case QMetaType::Float:
        lua_pushnumber(lua, v.toDouble(nullptr));
        break;
    default:
        // TODO: Implement more types, e.g. lists, maps etc.
        lua_pushnil(lua);
    }
}


}
