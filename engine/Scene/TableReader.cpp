#include "./TableReader.h"
#include <iostream>
#include <utility>

TableReader::TableReader(lua_State *lua, int index, std::string context)
    : lua(lua), index(lua_absindex(lua, index)), context(std::move(context))
{
}

bool TableReader::Push(const char *key) const
{
    readFields.insert(key);
    lua_getfield(lua, index, key);
    if (lua_isnil(lua, -1))
    {
        lua_pop(lua, 1);
        return false;
    }
    return true;
}

double TableReader::Number(const char *key, double fallback) const
{
    if (!Push(key))
        return fallback;
    double value = fallback;
    if (lua_isnumber(lua, -1))
        value = lua_tonumber(lua, -1);
    else
        WrongType(key, "a number");
    lua_pop(lua, 1);
    return value;
}

bool TableReader::Bool(const char *key, bool fallback) const
{
    if (!Push(key))
        return fallback;
    bool value = fallback;
    if (lua_isboolean(lua, -1))
        value = lua_toboolean(lua, -1);
    else
        WrongType(key, "a boolean");
    lua_pop(lua, 1);
    return value;
}

std::string TableReader::String(const char *key, const std::string &fallback) const
{
    if (!Push(key))
        return fallback;
    std::string value = fallback;
    if (lua_type(lua, -1) == LUA_TSTRING)
        value = lua_tostring(lua, -1);
    else
        WrongType(key, "a string");
    lua_pop(lua, 1);
    return value;
}

bool TableReader::Has(const char *key) const
{
    if (!Push(key))
        return false;
    lua_pop(lua, 1);
    return true;
}

bool TableReader::PushTable(const char *key) const
{
    if (!Push(key))
        return false;
    if (!lua_istable(lua, -1))
    {
        WrongType(key, "a table");
        lua_pop(lua, 1);
        return false;
    }
    return true;
}

bool TableReader::WithTable(const char *key, const std::function<void(const TableReader &)> &read) const
{
    if (!Push(key))
        return false;
    if (!lua_istable(lua, -1))
    {
        WrongType(key, "a table");
        lua_pop(lua, 1);
        return false;
    }
    const TableReader inner(lua, -1, context + "." + key);
    read(inner);
    inner.ReportUnknownFields();
    lua_pop(lua, 1);
    return true;
}

glm::vec2 TableReader::Vec2(const char *key, glm::vec2 fallback) const
{
    glm::vec2 value = fallback;
    WithTable(key, [&](const TableReader &v) { value = glm::vec2(v.Float("x", fallback.x), v.Float("y", fallback.y)); });
    return value;
}

SDL_Color TableReader::Color(const char *key, SDL_Color fallback) const
{
    SDL_Color value = fallback;
    WithTable(key, [&](const TableReader &c) {
        value = SDL_Color{static_cast<Uint8>(c.Int("r", fallback.r)), static_cast<Uint8>(c.Int("g", fallback.g)),
                          static_cast<Uint8>(c.Int("b", fallback.b)), static_cast<Uint8>(c.Int("a", fallback.a))};
    });
    return value;
}

std::vector<std::string> TableReader::StringList(const char *key) const
{
    std::vector<std::string> values;
    if (!Push(key))
        return values;
    if (lua_istable(lua, -1))
    {
        const int list = lua_gettop(lua);
        for (lua_Integer i = 1; i <= static_cast<lua_Integer>(lua_rawlen(lua, list)); ++i)
        {
            lua_rawgeti(lua, list, i);
            if (lua_type(lua, -1) == LUA_TSTRING)
                values.push_back(lua_tostring(lua, -1));
            else
                Warn(std::string(key) + "[" + std::to_string(i) + "] should be a string");
            lua_pop(lua, 1);
        }
    }
    else
    {
        WrongType(key, "a list of strings");
    }
    lua_pop(lua, 1);
    return values;
}

void TableReader::Warn(const std::string &message) const
{
    std::cerr << "Scene warning: " << context << ": " << message << std::endl;
}

void TableReader::WrongType(const char *key, const char *expected) const
{
    Warn(std::string(key) + " should be " + expected);
}

void TableReader::ReportUnknownFields() const
{
    lua_pushnil(lua);
    while (lua_next(lua, index) != 0)
    {
        if (lua_type(lua, -2) == LUA_TSTRING && readFields.count(lua_tostring(lua, -2)) == 0)
        {
            Warn(std::string("unknown field '") + lua_tostring(lua, -2) + "'");
        }
        lua_pop(lua, 1);
    }
}
