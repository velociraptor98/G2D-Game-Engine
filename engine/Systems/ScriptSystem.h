#ifndef SCRIPTSYSTEM_H
#define SCRIPTSYSTEM_H
#include <SDL2/SDL.h>
#include <iostream>
#include <lua.hpp>
#include "../ECS/ECS.h"
#include "../Components/ScriptComponent.h"
#include "../Scripting/LuaBindings.h"
class ScriptSystem : public System
{
public:
    explicit ScriptSystem(lua_State *lua = nullptr) : lua(lua) { RequireComponent<ScriptComponent>(); }

    // Script references belong to one Lua state; switch only once every
    // scripted entity of the old state has been removed.
    void SetLuaState(lua_State *state) { lua = state; }

    void Update(float deltaTime, Uint32 ticks)
    {
        if (!lua)
        {
            return;
        }
        for (auto entity : GetEntities())
        {
            const int functionRef = entity.GetComponent<ScriptComponent>().functionRef;
            if (functionRef == LUA_NOREF || !entity.IsAlive())
            {
                continue;
            }
            lua_rawgeti(lua, LUA_REGISTRYINDEX, functionRef);
            lua_pushinteger(lua, EncodeEntity(entity));
            lua_pushnumber(lua, deltaTime);
            lua_pushinteger(lua, ticks);
            if (lua_pcall(lua, 3, 0, 0) != LUA_OK)
            {
                // Disabled after the first error so a broken script reports once
                // instead of every frame.
                std::cerr << "Script error on entity " << entity.GetId() << ": " << lua_tostring(lua, -1)
                          << std::endl;
                lua_pop(lua, 1);
                Release(entity.GetComponent<ScriptComponent>());
            }
        }
    }

    void OnEntityRemoved(Entity entity) override { Release(entity.GetComponent<ScriptComponent>()); }

private:
    void Release(ScriptComponent &script)
    {
        if (!lua)
        {
            return;
        }
        luaL_unref(lua, LUA_REGISTRYINDEX, script.functionRef);
        script.functionRef = LUA_NOREF;
    }

    lua_State *lua;
};
#endif
