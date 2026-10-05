#ifndef COLLISIONRESPONSECOMPONENT_H
#define COLLISIONRESPONSECOMPONENT_H
enum class SolidResponse
{
    None,
    Bounce,
    Block,
    Destroy
};

// How this entity reacts when its collider overlaps a solid one.
struct CollisionResponseComponent
{
    SolidResponse onSolid;
    bool flipSpriteOnBounce;
    CollisionResponseComponent(SolidResponse onSolid = SolidResponse::None, bool flipSpriteOnBounce = false)
        : onSolid(onSolid), flipSpriteOnBounce(flipSpriteOnBounce)
    {
    }
};
#endif
