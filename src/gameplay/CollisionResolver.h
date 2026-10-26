#pragma once

#include <glm/vec3.hpp>

#include <vector>

class DebrisField;
class Pod;
class Station;

struct ImpactEvent {
    glm::vec3 point{0.0f};
    float impactSpeed = 0.0f;
    float damage = 0.0f;
    bool withStation = false;
};

class CollisionResolver {
public:
    std::vector<ImpactEvent> resolve(Pod& pod, DebrisField& debrisField, const Station& station);

private:
    void resolvePodVsDebris(Pod& pod, DebrisField& debrisField, std::vector<ImpactEvent>& events);
    void resolvePodVsStation(Pod& pod, const Station& station, std::vector<ImpactEvent>& events);
    void resolveDebrisVsStation(DebrisField& debrisField, const Station& station);
};
