#include "gameplay/CollisionResolver.h"

#include "gameplay/DebrisField.h"
#include "gameplay/Pod.h"
#include "gameplay/Station.h"
#include "physics/Collision.h"
#include "scene/SceneNode.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <algorithm>

namespace {

constexpr float kPodMass = 8.0f;
constexpr float kPodDebrisRestitution = 0.45f;
constexpr float kStationRestitution = 0.35f;
constexpr float kDebrisStationRestitution = 0.6f;
constexpr float kMinDamagingSpeed = 0.6f;
constexpr float kMinStationDamagingSpeed = 1.5f;
constexpr float kMaxSingleImpactDamage = 22.0f;
constexpr float kStationBroadPhaseRadius = 80.0f;

struct StationContact {
    bool hit = false;
    glm::vec3 normal{0.0f};
    float penetration = 0.0f;
};

StationContact deepestStationContact(const Sphere& worldSphere, const Station& station)
{
    StationContact deepest;
    for (const StationCollider& collider : station.colliders()) {
        const glm::mat4 worldToLocal = glm::inverse(collider.node->worldMatrix());
        const Sphere localSphere{glm::vec3(worldToLocal * glm::vec4(worldSphere.center, 1.0f)), worldSphere.radius};
        const Contact contact = collision::sphereVsAabb(localSphere, collider.localBox);
        if (contact.hit && contact.penetration > deepest.penetration) {
            deepest.hit = true;
            deepest.penetration = contact.penetration;
            deepest.normal = collider.node->localToWorldDirection(contact.normal);
        }
    }
    for (const Sphere& sphere : station.worldSphereColliders()) {
        const Contact contact = collision::sphereVsSphere(sphere, worldSphere);
        if (contact.hit && contact.penetration > deepest.penetration) {
            deepest.hit = true;
            deepest.penetration = contact.penetration;
            deepest.normal = contact.normal;
        }
    }
    return deepest;
}

bool nearStation(const glm::vec3& position, const Station& station)
{
    return glm::distance(position, station.dockingTargetWorld()) < kStationBroadPhaseRadius
        || glm::length(position) < kStationBroadPhaseRadius;
}

}

std::vector<ImpactEvent> CollisionResolver::resolve(Pod& pod, DebrisField& debrisField, const Station& station)
{
    std::vector<ImpactEvent> events;
    if (!pod.isDocked()) {
        resolvePodVsDebris(pod, debrisField, events);
        resolvePodVsStation(pod, station, events);
    }
    resolveDebrisVsStation(debrisField, station);
    return events;
}

void CollisionResolver::resolvePodVsDebris(Pod& pod, DebrisField& debrisField, std::vector<ImpactEvent>& events)
{
    for (Debris& debris : debrisField.pieces()) {
        if (!debris.isActive()) {
            continue;
        }
        const Sphere podSphere{pod.position(), Pod::kCollisionRadius};
        const Sphere debrisSphere{debris.position(), debris.radius()};
        const Contact contact = collision::sphereVsSphere(podSphere, debrisSphere);
        if (!contact.hit) {
            continue;
        }

        glm::vec3 podVelocity = pod.velocity();
        glm::vec3 debrisVelocity = debris.velocity();
        const float impactSpeed = collision::resolveImpulse(podVelocity, 1.0f / kPodMass, debrisVelocity,
            1.0f / debris.mass(), contact.normal, kPodDebrisRestitution);
        pod.applyImpulse(podVelocity - pod.velocity());
        debris.setVelocity(debrisVelocity);

        const float podShare = debris.mass() / (debris.mass() + kPodMass);
        pod.pushOut(-contact.normal * contact.penetration * podShare);
        debris.pushOut(contact.normal * contact.penetration * (1.0f - podShare));

        if (impactSpeed < kMinDamagingSpeed) {
            continue;
        }
        const glm::vec3 tumbleAxis = glm::cross(contact.normal, debrisVelocity - podVelocity);
        debris.startTumbling(tumbleAxis, std::min(0.6f + impactSpeed * 0.25f, 4.0f));

        ImpactEvent event;
        event.point = pod.position() + contact.normal * Pod::kCollisionRadius;
        event.impactSpeed = impactSpeed;
        event.damage = std::clamp(impactSpeed * (0.5f + debris.mass() * 0.04f), 3.0f, kMaxSingleImpactDamage);
        events.push_back(event);
    }
}

void CollisionResolver::resolvePodVsStation(Pod& pod, const Station& station, std::vector<ImpactEvent>& events)
{
    if (!nearStation(pod.position(), station)) {
        return;
    }
    const StationContact contact = deepestStationContact({pod.position(), Pod::kCollisionRadius}, station);
    if (!contact.hit) {
        return;
    }
    pod.pushOut(contact.normal * contact.penetration);

    const glm::vec3 surfaceVelocity = station.pointVelocity(pod.position());
    const glm::vec3 relativeVelocity = pod.velocity() - surfaceVelocity;
    const float closingSpeed = -glm::dot(relativeVelocity, contact.normal);
    if (closingSpeed <= 0.0f) {
        return;
    }
    pod.applyImpulse(contact.normal * closingSpeed * (1.0f + kStationRestitution));

    if (closingSpeed < kMinStationDamagingSpeed) {
        return;
    }
    ImpactEvent event;
    event.point = pod.position() - contact.normal * Pod::kCollisionRadius;
    event.impactSpeed = closingSpeed;
    event.damage = std::min((closingSpeed - kMinStationDamagingSpeed) * 3.0f + 4.0f, kMaxSingleImpactDamage);
    event.withStation = true;
    events.push_back(event);
}

void CollisionResolver::resolveDebrisVsStation(DebrisField& debrisField, const Station& station)
{
    for (Debris& debris : debrisField.pieces()) {
        if (!debris.isActive() || !nearStation(debris.position(), station)) {
            continue;
        }
        const StationContact contact = deepestStationContact({debris.position(), debris.radius()}, station);
        if (!contact.hit) {
            continue;
        }
        debris.pushOut(contact.normal * contact.penetration);
        const glm::vec3 relativeVelocity = debris.velocity() - station.pointVelocity(debris.position());
        const float closingSpeed = -glm::dot(relativeVelocity, contact.normal);
        if (closingSpeed > 0.0f) {
            debris.setVelocity(debris.velocity() + contact.normal * closingSpeed * (1.0f + kDebrisStationRestitution));
        }
    }
}
