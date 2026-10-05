#ifndef TEAMCOMPONENT_H
#define TEAMCOMPONENT_H
#include <string>
#include <utility>
// Entities on the same team never damage each other. An entity without this
// component is on the unnamed team "".
struct TeamComponent
{
    std::string team;
    TeamComponent(std::string team = "") : team(std::move(team)) {}
};
#endif
