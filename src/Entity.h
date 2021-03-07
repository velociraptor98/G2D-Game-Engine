#ifndef ENTITY_H
#define ENTITY_H
#include <vector>
#include <string>
#include "./EntityManager.h"
#include "./Component.h"
#include <map>
class EntityManager;
class Component;
class Entity
{
private:
    EntityManager &manager;
    bool isActive;
    std::vector<Component *> components;
    std::map<const std::type_info*,Component*> componentTypeMap;
public:
    std::string name;
    Entity(EntityManager &manager);
    Entity(EntityManager &manager, std::string name);
    void Update(float deltaTime);
    void Render();
    void Destroy();
    bool IsActive() const;
    template <typename T, typename... TArgs>
    T &AddComponents(TArgs &&... args)
    {
        T *newComponent(new T(std::forward<TArgs>(args)...));
        newComponent->owner = this;
        components.emplace_back(newComponent);
        componentTypeMap[&typeid(*newComponent)] = newComponent;
        newComponent->init();
        return *newComponent;
    }
    template <typename T>
    T* &getComponent()
    {
        return static_cast<T*>(componentTypeMap[&typeid(T)]);
    }
};
#endif