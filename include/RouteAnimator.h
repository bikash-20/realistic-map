#pragma once
#include <string>
#include <vector>

namespace rm {

enum class AnimState { Driving, Arrived };

class RouteAnimator {
public:
    void reset();
    void setPath(const std::vector<std::string>& path);

    // Advance one frame. dt is in seconds.
    void update(float dt, float carSpeedPxPerSec);

    int   seg()      const { return m_seg; }
    float frac()     const { return m_frac; }
    AnimState state() const { return m_state; }
    float  arrivedTimer() const { return m_arrivedTimer; }

    bool atDestination() const;

private:
    std::vector<std::string> m_path;
    int   m_seg   = 0;
    float m_frac  = 0.0f;
    AnimState m_state = AnimState::Driving;
    float m_arrivedTimer = 0.0f;
};

} // namespace rm
