#ifndef COLLISIONEVENT_H
#define COLLISIONEVENT_H
#include "../ECS/ECS.h"
struct CollisionEvent
{
    Entity a;
    Entity b;
    CollisionEvent(Entity a, Entity b) : a(a), b(b) {}
};
#endif
