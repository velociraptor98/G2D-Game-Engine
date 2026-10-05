#ifndef ECS_H
#define ECS_H
#include <bitset>
#include <cassert>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

const std::size_t MAX_COMPONENTS = 32;
using Signature = std::bitset<MAX_COMPONENTS>;
using EntityId = std::uint32_t;

namespace ecs_detail
{
    inline std::size_t nextComponentId = 0;
}

// Ids are handed out on first use, so they can differ between runs and
// must never be saved to disk.
template <typename T>
std::size_t ComponentId()
{
    static const std::size_t id = ecs_detail::nextComponentId++;
    assert(id < MAX_COMPONENTS);
    return id;
}

class Registry;

// A handle to an entity. Ids are recycled after a kill; the generation tells a
// stale handle apart from the new entity that reuses its id.
class Entity
{
public:
    Entity(EntityId id, std::uint32_t generation, Registry *registry)
        : id(id), generation(generation), registry(registry) {}
    EntityId GetId() const { return id; }
    std::uint32_t GetGeneration() const { return generation; }
    void Kill();
    // False for a stale handle, and as soon as Kill() is requested.
    bool IsAlive() const;
    void Tag(const std::string &tag);
    bool HasTag(const std::string &tag) const;
    void Group(const std::string &group);
    bool BelongsToGroup(const std::string &group) const;
    template <typename T, typename... TArgs>
    T &AddComponent(TArgs &&...args);
    template <typename T>
    void RemoveComponent();
    template <typename T>
    bool HasComponent() const;
    template <typename T>
    T &GetComponent() const;
    bool operator==(const Entity &other) const { return id == other.id && generation == other.generation; }
    bool operator!=(const Entity &other) const { return !(*this == other); }

private:
    EntityId id;
    std::uint32_t generation;
    Registry *registry;
};

class System
{
public:
    virtual ~System() = default;
    const Signature &GetSignature() const { return signature; }
    const std::vector<Entity> &GetEntities() const { return entities; }
    void AddEntity(Entity entity);
    bool RemoveEntity(Entity entity);
    // Called when an entity leaves this system; its components are still readable.
    virtual void OnEntityRemoved(Entity) {}

protected:
    template <typename T>
    void RequireComponent() { signature.set(ComponentId<T>()); }

private:
    Signature signature;
    std::vector<Entity> entities;
};

class IPool
{
public:
    virtual ~IPool() = default;
    virtual void Remove(EntityId entity) = 0;
};

// Components are packed contiguously; removal swaps the last element into the
// hole. Adding to or removing from a pool can therefore move other components,
// so never hold a component reference across an AddComponent or Registry::Update.
template <typename T>
class Pool : public IPool
{
public:
    template <typename... TArgs>
    T &Add(EntityId entity, TArgs &&...args)
    {
        auto existing = entityToIndex.find(entity);
        if (existing != entityToIndex.end())
        {
            data[existing->second] = T(std::forward<TArgs>(args)...);
            return data[existing->second];
        }
        entityToIndex[entity] = data.size();
        indexToEntity.push_back(entity);
        data.emplace_back(std::forward<TArgs>(args)...);
        return data.back();
    }

    void Remove(EntityId entity) override
    {
        auto it = entityToIndex.find(entity);
        if (it == entityToIndex.end())
        {
            return;
        }
        const std::size_t index = it->second;
        const std::size_t last = data.size() - 1;
        if (index != last)
        {
            data[index] = std::move(data[last]);
            indexToEntity[index] = indexToEntity[last];
            entityToIndex[indexToEntity[index]] = index;
        }
        data.pop_back();
        indexToEntity.pop_back();
        entityToIndex.erase(entity);
    }

    T &Get(EntityId entity) { return data[entityToIndex.at(entity)]; }

private:
    std::vector<T> data;
    std::vector<EntityId> indexToEntity;
    std::unordered_map<EntityId, std::size_t> entityToIndex;
};

// Killing entities and removing components are deferred until Update(), so
// systems can do either while iterating without invalidating their entity lists.
// Newly created entities and added components join systems on the next Update().
class Registry
{
public:
    Registry() = default;
    Registry(const Registry &) = delete;
    Registry &operator=(const Registry &) = delete;

    Entity CreateEntity();
    void KillEntity(Entity entity);
    bool IsAlive(Entity entity) const;
    // Requests a kill of every entity; like Kill(), it takes effect on Update().
    void KillAllEntities();
    void Update();

    void TagEntity(Entity entity, const std::string &tag);
    bool HasTag(Entity entity, const std::string &tag) const;
    std::optional<Entity> GetEntityByTag(const std::string &tag) const;
    void GroupEntity(Entity entity, const std::string &group);
    bool BelongsToGroup(Entity entity, const std::string &group) const;
    std::vector<Entity> GetEntitiesByGroup(const std::string &group) const;

    template <typename T, typename... TArgs>
    T &AddComponent(Entity entity, TArgs &&...args);
    template <typename T>
    void RemoveComponent(Entity entity);
    template <typename T>
    bool HasComponent(Entity entity) const;
    template <typename T>
    T &GetComponent(Entity entity) const;

    template <typename T, typename... TArgs>
    T &AddSystem(TArgs &&...args);
    template <typename T>
    void RemoveSystem();
    template <typename T>
    bool HasSystem() const;
    template <typename T>
    T &GetSystem() const;

private:
    void RefreshEntity(EntityId id);
    void DestroyEntity(EntityId id);
    Entity HandleFor(EntityId id) const { return Entity(id, generations[id], const_cast<Registry *>(this)); }
    bool IsCurrent(Entity entity) const
    {
        return entity.GetId() < generations.size() && generations[entity.GetId()] == entity.GetGeneration();
    }

    std::vector<Signature> entitySignatures;
    std::vector<bool> aliveEntities;
    std::vector<std::uint32_t> generations;
    std::deque<EntityId> freeIds;
    std::vector<std::unique_ptr<IPool>> componentPools;
    std::unordered_map<std::type_index, std::unique_ptr<System>> systems;
    std::set<EntityId> entitiesToRefresh;
    std::set<EntityId> entitiesToKill;
    std::unordered_map<std::string, EntityId> entityPerTag;
    std::unordered_map<EntityId, std::string> tagPerEntity;
    std::unordered_map<std::string, std::set<EntityId>> entitiesPerGroup;
    std::unordered_map<EntityId, std::string> groupPerEntity;
};

template <typename T, typename... TArgs>
T &Registry::AddComponent(Entity entity, TArgs &&...args)
{
    assert(IsCurrent(entity));
    const std::size_t componentId = ComponentId<T>();
    if (componentId >= componentPools.size())
    {
        componentPools.resize(componentId + 1);
    }
    if (!componentPools[componentId])
    {
        componentPools[componentId] = std::make_unique<Pool<T>>();
    }
    entitySignatures[entity.GetId()].set(componentId);
    entitiesToRefresh.insert(entity.GetId());
    auto &pool = static_cast<Pool<T> &>(*componentPools[componentId]);
    return pool.Add(entity.GetId(), std::forward<TArgs>(args)...);
}

template <typename T>
void Registry::RemoveComponent(Entity entity)
{
    assert(IsCurrent(entity));
    entitySignatures[entity.GetId()].reset(ComponentId<T>());
    entitiesToRefresh.insert(entity.GetId());
}

template <typename T>
bool Registry::HasComponent(Entity entity) const
{
    if (!IsCurrent(entity))
    {
        return false;
    }
    return entitySignatures[entity.GetId()].test(ComponentId<T>());
}

template <typename T>
T &Registry::GetComponent(Entity entity) const
{
    assert(IsCurrent(entity));
    const std::size_t componentId = ComponentId<T>();
    assert(componentId < componentPools.size() && componentPools[componentId]);
    return static_cast<Pool<T> &>(*componentPools[componentId]).Get(entity.GetId());
}

template <typename T, typename... TArgs>
T &Registry::AddSystem(TArgs &&...args)
{
    auto system = std::make_unique<T>(std::forward<TArgs>(args)...);
    T &ref = *system;
    systems[std::type_index(typeid(T))] = std::move(system);
    for (EntityId id = 0; id < aliveEntities.size(); ++id)
    {
        if (aliveEntities[id])
        {
            entitiesToRefresh.insert(id);
        }
    }
    return ref;
}

template <typename T>
void Registry::RemoveSystem()
{
    systems.erase(std::type_index(typeid(T)));
}

template <typename T>
bool Registry::HasSystem() const
{
    return systems.count(std::type_index(typeid(T))) > 0;
}

template <typename T>
T &Registry::GetSystem() const
{
    return static_cast<T &>(*systems.at(std::type_index(typeid(T))));
}

template <typename T, typename... TArgs>
T &Entity::AddComponent(TArgs &&...args)
{
    return registry->AddComponent<T>(*this, std::forward<TArgs>(args)...);
}

template <typename T>
void Entity::RemoveComponent()
{
    registry->RemoveComponent<T>(*this);
}

template <typename T>
bool Entity::HasComponent() const
{
    return registry->HasComponent<T>(*this);
}

template <typename T>
T &Entity::GetComponent() const
{
    return registry->GetComponent<T>(*this);
}
#endif
