#include "./TestFramework.h"
#include "EventBus/EventBus.h"

namespace
{
    struct Ping
    {
        int value;
        explicit Ping(int value) : value(value) {}
    };
    struct Pong
    {
    };
}

TEST(EventBusDeliversToEverySubscriberOfThatType)
{
    EventBus bus;
    int total = 0;
    int pongs = 0;
    bus.Subscribe<Ping>([&](Ping &ping) { total += ping.value; });
    bus.Subscribe<Ping>([&](Ping &ping) { total += ping.value * 10; });
    bus.Subscribe<Pong>([&](Pong &) { ++pongs; });
    bus.Emit<Ping>(2);
    CHECK_EQ(total, 22);
    CHECK_EQ(pongs, 0);
}

TEST(EventBusEmitWithoutSubscribersDoesNothing)
{
    EventBus bus;
    bus.Emit<Ping>(1);
    CHECK(true);
}

TEST(EventBusResetDropsSubscribers)
{
    EventBus bus;
    int calls = 0;
    bus.Subscribe<Pong>([&](Pong &) { ++calls; });
    bus.Reset();
    bus.Emit<Pong>();
    CHECK_EQ(calls, 0);
}

TEST(EventBusHandlerMaySubscribeWhileBeingCalled)
{
    EventBus bus;
    int calls = 0;
    bus.Subscribe<Pong>([&](Pong &) {
        ++calls;
        bus.Subscribe<Pong>([&](Pong &) { ++calls; });
        bus.Subscribe<Ping>([](Ping &) {});
    });
    bus.Emit<Pong>();
    CHECK_EQ(calls, 1);
    bus.Emit<Pong>();
    CHECK_EQ(calls, 3);
}
