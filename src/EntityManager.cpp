#include "./EntityManager.h"

void EntityManager::Clear()
{
    for(auto& entity : entities)
    {
        entity->Destroy();
    }
}
bool EntityManager::hasEntities()
{
    return !(entities.size() == 0);
}
void EntityManager::Update(float deltaTime)
{
    for(auto& entity:entities)
    {
        entity->Update(deltaTime);
    }
}
void EntityManager::Render()
{
    for(auto& entity:entities)
    {
        entity->Render();
    }
}
Entity& EntityManager::AddEntity(std::string name)
{
    Entity *entity = new Entity(*this,name);
    entities.emplace_back(entity);
    return *entity;
}
std::vector<Entity*> EntityManager::GetEntities() const
{
    return entities;
}
unsigned int EntityManager::GetEntityCount()
{
    return entities.size();
}
