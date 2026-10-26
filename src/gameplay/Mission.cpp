#include "gameplay/Mission.h"

#include <algorithm>

void Mission::reset()
{
    lifeSupportSeconds_ = kLifeSupportSeconds;
    missionSeconds_ = 0.0f;
    impactCount_ = 0;
    damageTaken_ = 0.0f;
    repaired_ = false;
    lossReason_ = LossReason::None;
}

void Mission::updateFlight(float deltaSeconds)
{
    missionSeconds_ += deltaSeconds;
}

void Mission::startEva()
{
    lifeSupportSeconds_ = kLifeSupportSeconds;
}

void Mission::updateEva(float deltaSeconds)
{
    missionSeconds_ += deltaSeconds;
    if (!repaired_) {
        lifeSupportSeconds_ = std::max(lifeSupportSeconds_ - deltaSeconds, 0.0f);
    }
}

void Mission::recordImpact(float damage)
{
    ++impactCount_;
    damageTaken_ += damage;
}

void Mission::markRepaired()
{
    repaired_ = true;
}

void Mission::fail(LossReason reason)
{
    lossReason_ = reason;
}

float Mission::lifeSupportSeconds() const
{
    return lifeSupportSeconds_;
}

float Mission::lifeSupportFraction() const
{
    return lifeSupportSeconds_ / kLifeSupportSeconds;
}

bool Mission::isLifeSupportDepleted() const
{
    return lifeSupportSeconds_ <= 0.0f;
}

bool Mission::isRepaired() const
{
    return repaired_;
}

LossReason Mission::lossReason() const
{
    return lossReason_;
}

std::string Mission::lossMessage() const
{
    switch (lossReason_) {
    case LossReason::HullDestroyed:
        return "Hull breach. The pod broke apart in the debris field.";
    case LossReason::LifeSupportDepleted:
        return "Life support ran out before the panel was repaired.";
    case LossReason::None:
        break;
    }
    return "Mission failed.";
}

float Mission::missionSeconds() const
{
    return missionSeconds_;
}

int Mission::impactCount() const
{
    return impactCount_;
}

float Mission::damageTaken() const
{
    return damageTaken_;
}
