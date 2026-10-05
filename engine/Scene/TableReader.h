#ifndef TABLEREADER_H
#define TABLEREADER_H
#include <SDL2/SDL.h>
#include <functional>
#include <lua.hpp>
#include <set>
#include <string>
#include <vector>
#include "../../lib/glm/glm.hpp"

// Reads fields of the Lua table at a stack index. A missing field gives the
// fallback; a field of the wrong type also warns. Warnings name `context`
// (e.g. "entities[3].sprite") so scene authors can find the problem.
class TableReader
{
public:
    TableReader(lua_State *lua, int index, std::string context);

    double Number(const char *key, double fallback) const;
    int Int(const char *key, int fallback) const { return static_cast<int>(Number(key, fallback)); }
    float Float(const char *key, float fallback) const { return static_cast<float>(Number(key, fallback)); }
    bool Bool(const char *key, bool fallback) const;
    std::string String(const char *key, const std::string &fallback) const;
    glm::vec2 Vec2(const char *key, glm::vec2 fallback) const;
    SDL_Color Color(const char *key, SDL_Color fallback) const;
    std::vector<std::string> StringList(const char *key) const;
    bool Has(const char *key) const;
    // Pushes the nested table at `key` and returns true, if there is one; the
    // caller pops it. Unlike WithTable, its fields are not checked for typos.
    bool PushTable(const char *key) const;
    // Calls `read` with the nested table at `key` and returns true, if there is one.
    bool WithTable(const char *key, const std::function<void(const TableReader &)> &read) const;

    void Warn(const std::string &message) const;
    // Warns about every named field that was never read; almost always a typo.
    void ReportUnknownFields() const;
    const std::string &Context() const { return context; }
    lua_State *Lua() const { return lua; }
    int Index() const { return index; }

private:
    // Pushes the field; returns false (and pushes nothing) when it is nil.
    bool Push(const char *key) const;
    void WrongType(const char *key, const char *expected) const;

    lua_State *lua;
    int index;
    std::string context;
    mutable std::set<std::string> readFields;
};
#endif
