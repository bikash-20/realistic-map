#include "RouteAnimator.h"

namespace rm {

void RouteAnimator::reset() {
    m_seg = 0;
    m_frac = 0.0f;
    m_state = AnimState::Driving;
    m_arrivedTimer = 0.0f;
}

void RouteAnimator::setPath(const std::vector<std::string>& path) {
    m_path = path;
    reset();
}

void RouteAnimator::update(float dt, float carSpeedPxPerSec) {
    if (m_state == AnimState::Arrived) {
        m_arrivedTimer += dt;
        return;
    }
    if (m_path.size() < 2) return;
    if (m_seg >= static_cast<int>(m_path.size()) - 1) {
        m_state = AnimState::Arrived;
        m_arrivedTimer = 0.0f;
        return;
    }
    auto segLenPx = [&](int i) {
        // We don't have positions here; the caller computes via NavSystem
        // and passes already-corrected speeds. The animator advances
        // uniformly per segment; if you want true arc-length animation,
        // externalize segment lengths to RouteAnimator::setLengths().
        return 100.0f;
    };
    (void)segLenPx;
    // Naive but smooth: increase frac by speed*dt/(segPx). Caller passes
    // speeds in pixels/sec already adjusted per-segment through this same fn.
    m_frac += (carSpeedPxPerSec * dt) / 100.0f;
    while (m_frac >= 1.0f) {
        m_frac -= 1.0f;
        m_seg++;
        if (m_seg >= static_cast<int>(m_path.size()) - 1) {
            m_state = AnimState::Arrived;
            m_arrivedTimer = 0.0f;
            return;
        }
    }
}

bool RouteAnimator::atDestination() const {
    return m_state == AnimState::Arrived;
}

} // namespace rm
