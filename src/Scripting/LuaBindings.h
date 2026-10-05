#ifndef LUABINDINGS_H
#define LUABINDINGS_H
#include <lua.hpp>
#include <optional>
#include "../ECS/ECS.h"

// Entities cross into Lua as one integer holding both id and generation, so a
// script holding on to a killed entity can't reach whatever reuses its id.
lua_Integer EncodeEntity(Entity entity);
std::optional<Entity> DecodeEntity(lua_Integer value, Registry &registry);

// Exposes get_position, set_position, get_velocity, set_velocity, set_rotation,
// set_sprite_row, set_flip, set_projectile_velocity and get_entity_by_tag.
// Calls on dead entities are ignored (getters return nil).
void RegisterLuaBindings(lua_State *lua, Registry &registry);
#endif
