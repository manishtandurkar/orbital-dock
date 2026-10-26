#pragma once

#include <string>

enum class LossReason {
    None,
    HullDestroyed,
    LifeSupportDepleted
};

class Mission {
public:
    static constexpr float kLifeSupportSeconds = 75.0f;

    void reset();
    void updateFlight(float deltaSeconds);
    void startEva();
    void updateEva(float deltaSeconds);
    void recordImpact(float damage);
    void markRepaired();
    void fail(LossReason reason);

    float lifeSupportSeconds() const;
    float lifeSupportFraction() const;
    bool isLifeSupportDepleted() const;
    bool isRepaired() const;
    LossReason lossReason() const;
    std::string lossMessage() const;
    float missionSeconds() const;
    int impactCount() const;
    float damageTaken() const;

private:
    float lifeSupportSeconds_ = kLifeSupportSeconds;
    float missionSeconds_ = 0.0f;
    int impactCount_ = 0;
    float damageTaken_ = 0.0f;
    bool repaired_ = false;
    LossReason lossReason_ = LossReason::None;
};
