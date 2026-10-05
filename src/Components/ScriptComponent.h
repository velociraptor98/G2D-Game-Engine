#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H
#include <lua.hpp>
// functionRef is a reference into the Lua registry of the level's lua_State,
// to a function called every frame as f(entity, delta_time, elapsed_ms).
struct ScriptComponent
{
    int functionRef;
    ScriptComponent(int functionRef = LUA_NOREF) : functionRef(functionRef) {}
};
#endif
