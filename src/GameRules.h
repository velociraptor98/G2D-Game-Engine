#ifndef GAMERULES_H
#define GAMERULES_H
#include <string>
#include "./ECS/ECS.h"

// Empty while the mission is still being played.
inline std::string MissionStatus(const Registry &registry)
{
    const auto player = registry.GetEntityByTag("player");
    if (!player || !player->IsAlive())
    {
        return "GAME OVER";
    }
    for (auto enemy : registry.GetEntitiesByGroup("enemies"))
    {
        if (enemy.IsAlive())
        {
            return "";
        }
    }
    return "MISSION COMPLETE";
}
#endif
