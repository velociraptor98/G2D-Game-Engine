#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H
#include "./Entity.h"
#include "./Component.h"
#include <vector>
class Entity;
class EntityManager
{
    private:
    std::vector<Entity*> entities;
    public:
    void Clear();
    void Update(float deltaTime);
    void Render();
    bool hasEntities();
    Entity& AddEntity(std::string name);
    std::vector<Entity*> GetEntities() const;
    unsigned int GetEntityCount();
};
#endif