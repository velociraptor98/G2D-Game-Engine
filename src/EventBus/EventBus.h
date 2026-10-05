#ifndef EVENTBUS_H
#define EVENTBUS_H
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

// Synchronous publish/subscribe: Emit() calls every handler for that event type
// before returning.
class EventBus
{
public:
    template <typename TEvent>
    void Subscribe(std::function<void(TEvent &)> handler)
    {
        handlers[std::type_index(typeid(TEvent))].push_back(
            [handler = std::move(handler)](void *event) { handler(*static_cast<TEvent *>(event)); });
    }

    template <typename TEvent, typename... TArgs>
    void Emit(TArgs &&...args)
    {
        auto it = handlers.find(std::type_index(typeid(TEvent)));
        if (it == handlers.end())
        {
            return;
        }
        TEvent event(std::forward<TArgs>(args)...);
        // Copied so a handler may subscribe without invalidating this loop.
        const auto subscribers = it->second;
        for (const auto &handler : subscribers)
        {
            handler(&event);
        }
    }

    void Reset() { handlers.clear(); }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(void *)>>> handlers;
};
#endif
