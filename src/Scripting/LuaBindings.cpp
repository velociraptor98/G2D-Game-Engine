#include "./LuaBindings.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/ProjectileEmitterComponent.h"

lua_Integer EncodeEntity(Entity entity)
{
    return static_cast<lua_Integer>((static_cast<std::uint64_t>(entity.GetGeneration()) << 32) | entity.GetId());
}

std::optional<Entity> DecodeEntity(lua_Integer value, Registry &registry)
{
    const auto bits = static_cast<std::uint64_t>(value);
    Entity entity(static_cast<EntityId>(bits & 0xffffffffu), static_cast<std::uint32_t>(bits >> 32), &registry);
    if (!entity.IsAlive())
    {
        return std::nullopt;
    }
    return entity;
}

namespace
{
    Registry &RegistryOf(lua_State *lua)
    {
        return *static_cast<Registry *>(lua_touserdata(lua, lua_upvalueindex(1)));
    }

    template <typename T>
    T *ComponentArg(lua_State *lua)
    {
        auto entity = DecodeEntity(luaL_checkinteger(lua, 1), RegistryOf(lua));
        if (!entity || !entity->HasComponent<T>())
        {
            return nullptr;
        }
        return &entity->GetComponent<T>();
    }

    int PushVec2(lua_State *lua, const glm::vec2 *value)
    {
        if (!value)
        {
            lua_pushnil(lua);
            return 1;
        }
        lua_pushnumber(lua, value->x);
        lua_pushnumber(lua, value->y);
        return 2;
    }

    glm::vec2 Vec2Args(lua_State *lua)
    {
        return glm::vec2(static_cast<float>(luaL_checknumber(lua, 2)), static_cast<float>(luaL_checknumber(lua, 3)));
    }

    int GetPosition(lua_State *lua)
    {
        auto *transform = ComponentArg<TransformComponent>(lua);
        return PushVec2(lua, transform ? &transform->position : nullptr);
    }

    int SetPosition(lua_State *lua)
    {
        const glm::vec2 position = Vec2Args(lua);
        if (auto *transform = ComponentArg<TransformComponent>(lua))
        {
            transform->position = position;
        }
        return 0;
    }

    int GetVelocity(lua_State *lua)
    {
        auto *rigidBody = ComponentArg<RigidBodyComponent>(lua);
        return PushVec2(lua, rigidBody ? &rigidBody->velocity : nullptr);
    }

    int SetVelocity(lua_State *lua)
    {
        const glm::vec2 velocity = Vec2Args(lua);
        if (auto *rigidBody = ComponentArg<RigidBodyComponent>(lua))
        {
            rigidBody->velocity = velocity;
        }
        return 0;
    }

    int SetRotation(lua_State *lua)
    {
        const double degrees = luaL_checknumber(lua, 2);
        if (auto *transform = ComponentArg<TransformComponent>(lua))
        {
            transform->rotation = degrees;
        }
        return 0;
    }

    int SetSpriteRow(lua_State *lua)
    {
        const lua_Integer row = luaL_checkinteger(lua, 2);
        if (auto *sprite = ComponentArg<SpriteComponent>(lua))
        {
            sprite->srcRect.y = static_cast<int>(row) * sprite->height;
        }
        return 0;
    }

    int SetFlip(lua_State *lua)
    {
        const bool horizontal = lua_toboolean(lua, 2);
        if (auto *sprite = ComponentArg<SpriteComponent>(lua))
        {
            sprite->flip = horizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        }
        return 0;
    }

    int SetProjectileVelocity(lua_State *lua)
    {
        const glm::vec2 velocity = Vec2Args(lua);
        if (auto *emitter = ComponentArg<ProjectileEmitterComponent>(lua))
        {
            emitter->projectileVelocity = velocity;
        }
        return 0;
    }

    int GetEntityByTag(lua_State *lua)
    {
        Registry &registry = RegistryOf(lua);
        auto entity = registry.GetEntityByTag(luaL_checkstring(lua, 1));
        if (!entity || !entity->IsAlive())
        {
            lua_pushnil(lua);
        }
        else
        {
            lua_pushinteger(lua, EncodeEntity(*entity));
        }
        return 1;
    }
}

void RegisterLuaBindings(lua_State *lua, Registry &registry)
{
    const luaL_Reg functions[] = {
        {"get_position", GetPosition},
        {"set_position", SetPosition},
        {"get_velocity", GetVelocity},
        {"set_velocity", SetVelocity},
        {"set_rotation", SetRotation},
        {"set_sprite_row", SetSpriteRow},
        {"set_flip", SetFlip},
        {"set_projectile_velocity", SetProjectileVelocity},
        {"get_entity_by_tag", GetEntityByTag},
    };
    for (const auto &function : functions)
    {
        lua_pushlightuserdata(lua, &registry);
        lua_pushcclosure(lua, function.func, 1);
        lua_setglobal(lua, function.name);
    }
}
