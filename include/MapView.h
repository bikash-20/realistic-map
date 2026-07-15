#pragma once
#include "raylib.h"

#include "NavSystem.h"

namespace rm {

// MapView owns the world->screen transformation: pan offset and zoom.
// Call updateFromInputs() each frame to react to mouse drag / wheel.
class MapView {
public:
    void reset(const NavSystem& nav, float screenW, float screenH,
               float padL, float padR, float padT, float padB);
    void updateFromInputs();

    // Convert a screen point to a world point (used for hit-testing).
    Vector2 screenToWorld(Vector2 s) const;
    Vector2 worldToScreen(Vector2 w) const;

    void panWorld(Vector2 deltaWorld);
    void zoomAtScreen(Vector2 anchor, float factor);

    bool containsScreenPoint(Vector2 p, float screenW, float screenH) const;

    // --- accessors ---
    float minX() const { return m_minX; }
    float minY() const { return m_minY; }
    float scale() const { return m_scale; }
    float offsetX() const { return m_offX; }
    float offsetY() const { return m_offY; }
    float zoom() const { return m_zoom; }

private:
    float m_minX = 0, m_minY = 0, m_scale = 1.0f;
    float m_offX = 0, m_offY = 0;
    float m_zoom = 1.0f;

    float m_minZoom = 0.6f, m_maxZoom = 4.0f;

    Vector2 m_lastDrag{};
    bool    m_dragging = false;
};

} // namespace rm
