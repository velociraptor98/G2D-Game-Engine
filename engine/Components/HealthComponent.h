#ifndef HEALTHCOMPONENT_H
#define HEALTHCOMPONENT_H
struct HealthComponent
{
    int health;
    int maxHealth;
    bool showHealthBar;
    HealthComponent(int health = 100, int maxHealth = 100, bool showHealthBar = true)
        : health(health), maxHealth(maxHealth), showHealthBar(showHealthBar)
    {
    }
};
#endif
