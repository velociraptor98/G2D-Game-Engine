#ifndef ENTITY_H
#define ENTITY_H
#include <vector>
#include <string>
#include <map>
#include <typeindex>
#include <iostream>
#include "./Component.h"
class EntityManager;
class Entity
{
private:
    EntityManager &manager;
    bool isActive;
    std::vector<Component *> components;
    // type_index rather than &typeid(T): type_info addresses aren't guaranteed
    // unique across translation units, so pointer keys can miss on some platforms.
    std::map<std::type_index,Component*> componentTypeMap;
public:
    std::string name;
    Entity(EntityManager &manager);
    Entity(EntityManager &manager, std::string name);
    ~Entity();
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
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
        componentTypeMap[std::type_index(typeid(T))] = newComponent;
        newComponent->init();
        return *newComponent;
    }
    template <typename T>
    T* getComponent()
    {
        auto it = componentTypeMap.find(std::type_index(typeid(T)));
        return it == componentTypeMap.end() ? nullptr : static_cast<T*>(it->second);
    }
    void getAllComponents(){
        for(auto const& x : componentTypeMap){
            std::cout<<x.first.name()<<" : "<<x.second<<std::endl;
        }
    }
    template <typename T>
    bool hasComponent() const{
        return componentTypeMap.count(std::type_index(typeid(T))) > 0;
    }
};
#endif
