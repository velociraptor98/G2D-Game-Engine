#ifndef DAMAGEONCONTACTCOMPONENT_H
#define DAMAGEONCONTACTCOMPONENT_H
// Damages anything with health on another team while touching it, every frame
// of contact. Projectiles set destroyOnContact so they hit once.
struct DamageOnContactComponent
{
    int damage;
    bool destroyOnContact;
    DamageOnContactComponent(int damage = 0, bool destroyOnContact = false)
        : damage(damage), destroyOnContact(destroyOnContact)
    {
    }
};
#endif
