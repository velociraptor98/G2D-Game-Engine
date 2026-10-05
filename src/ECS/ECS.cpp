#include "./ECS.h"
#include <algorithm>

void Entity::Kill()
{
    registry->KillEntity(*this);
}

bool Entity::IsAlive() const
{
    return registry->IsAlive(*this);
}

void Entity::Tag(const std::string &tag)
{
    registry->TagEntity(*this, tag);
}

bool Entity::HasTag(const std::string &tag) const
{
    return registry->HasTag(*this, tag);
}

void Entity::Group(const std::string &group)
{
    registry->GroupEntity(*this, group);
}

bool Entity::BelongsToGroup(const std::string &group) const
{
    return registry->BelongsToGroup(*this, group);
}

void System::AddEntity(Entity entity)
{
    if (std::find(entities.begin(), entities.end(), entity) == entities.end())
    {
        entities.push_back(entity);
    }
}

bool System::RemoveEntity(Entity entity)
{
    auto it = std::find(entities.begin(), entities.end(), entity);
    if (it == entities.end())
    {
        return false;
    }
    entities.erase(it);
    return true;
}

Entity Registry::CreateEntity()
{
    EntityId id;
    if (freeIds.empty())
    {
        id = static_cast<EntityId>(entitySignatures.size());
        entitySignatures.emplace_back();
        aliveEntities.push_back(true);
        generations.push_back(0);
    }
    else
    {
        id = freeIds.front();
        freeIds.pop_front();
        aliveEntities[id] = true;
    }
    entitiesToRefresh.insert(id);
    return HandleFor(id);
}

void Registry::KillEntity(Entity entity)
{
    if (IsCurrent(entity) && aliveEntities[entity.GetId()])
    {
        entitiesToKill.insert(entity.GetId());
    }
}

bool Registry::IsAlive(Entity entity) const
{
    return IsCurrent(entity) && aliveEntities[entity.GetId()] && entitiesToKill.count(entity.GetId()) == 0;
}

void Registry::Update()
{
    for (EntityId id : entitiesToKill)
    {
        DestroyEntity(id);
        entitiesToRefresh.erase(id);
    }
    entitiesToKill.clear();

    for (EntityId id : entitiesToRefresh)
    {
        RefreshEntity(id);
    }
    entitiesToRefresh.clear();
}

void Registry::RefreshEntity(EntityId id)
{
    const Signature &signature = entitySignatures[id];
    const Entity entity = HandleFor(id);
    for (auto &entry : systems)
    {
        System &system = *entry.second;
        if ((signature & system.GetSignature()) == system.GetSignature())
        {
            system.AddEntity(entity);
        }
        else if (system.RemoveEntity(entity))
        {
            system.OnEntityRemoved(entity);
        }
    }
    for (std::size_t componentId = 0; componentId < componentPools.size(); ++componentId)
    {
        if (componentPools[componentId] && !signature.test(componentId))
        {
            componentPools[componentId]->Remove(id);
        }
    }
}

void Registry::DestroyEntity(EntityId id)
{
    const Entity entity = HandleFor(id);
    for (auto &entry : systems)
    {
        if (entry.second->RemoveEntity(entity))
        {
            entry.second->OnEntityRemoved(entity);
        }
    }
    for (auto &pool : componentPools)
    {
        if (pool)
        {
            pool->Remove(id);
        }
    }
    auto tag = tagPerEntity.find(id);
    if (tag != tagPerEntity.end())
    {
        entityPerTag.erase(tag->second);
        tagPerEntity.erase(tag);
    }
    auto group = groupPerEntity.find(id);
    if (group != groupPerEntity.end())
    {
        entitiesPerGroup[group->second].erase(id);
        groupPerEntity.erase(group);
    }
    entitySignatures[id].reset();
    aliveEntities[id] = false;
    ++generations[id];
    freeIds.push_back(id);
}

void Registry::TagEntity(Entity entity, const std::string &tag)
{
    assert(IsCurrent(entity));
    auto previousOwner = entityPerTag.find(tag);
    if (previousOwner != entityPerTag.end())
    {
        tagPerEntity.erase(previousOwner->second);
    }
    auto previousTag = tagPerEntity.find(entity.GetId());
    if (previousTag != tagPerEntity.end())
    {
        entityPerTag.erase(previousTag->second);
    }
    entityPerTag[tag] = entity.GetId();
    tagPerEntity[entity.GetId()] = tag;
}

bool Registry::HasTag(Entity entity, const std::string &tag) const
{
    if (!IsCurrent(entity))
    {
        return false;
    }
    auto it = tagPerEntity.find(entity.GetId());
    return it != tagPerEntity.end() && it->second == tag;
}

std::optional<Entity> Registry::GetEntityByTag(const std::string &tag) const
{
    auto it = entityPerTag.find(tag);
    if (it == entityPerTag.end())
    {
        return std::nullopt;
    }
    return HandleFor(it->second);
}

void Registry::GroupEntity(Entity entity, const std::string &group)
{
    assert(IsCurrent(entity));
    auto previous = groupPerEntity.find(entity.GetId());
    if (previous != groupPerEntity.end())
    {
        entitiesPerGroup[previous->second].erase(entity.GetId());
    }
    entitiesPerGroup[group].insert(entity.GetId());
    groupPerEntity[entity.GetId()] = group;
}

bool Registry::BelongsToGroup(Entity entity, const std::string &group) const
{
    if (!IsCurrent(entity))
    {
        return false;
    }
    auto it = groupPerEntity.find(entity.GetId());
    return it != groupPerEntity.end() && it->second == group;
}

std::vector<Entity> Registry::GetEntitiesByGroup(const std::string &group) const
{
    std::vector<Entity> entities;
    auto it = entitiesPerGroup.find(group);
    if (it != entitiesPerGroup.end())
    {
        for (EntityId id : it->second)
        {
            entities.push_back(HandleFor(id));
        }
    }
    return entities;
}
