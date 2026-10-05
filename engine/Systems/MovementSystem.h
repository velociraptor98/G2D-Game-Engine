#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H
#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
class MovementSystem : public System
{
public:
    MovementSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<RigidBodyComponent>();
    }
    void Update(float deltaTime)
    {
        for (auto entity : GetEntities())
        {
            auto &transform = entity.GetComponent<TransformComponent>();
            transform.position += entity.GetComponent<RigidBodyComponent>().velocity * deltaTime;
        }
    }
};
#endif
