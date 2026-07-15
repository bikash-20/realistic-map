#include "MapView.h"

#include <algorithm>
#include <cmath>

namespace rm {

void MapView::reset(const NavSystem& nav, float screenW, float screenH,
                    float padL, float padR, float padT, float padB)
{
    float mnx = 1e9f, mxx = -1e9f, mny = 1e9f, mxy = -1e9f;
    for (const auto& [k, n] : nav.nodes) {
        mnx = std::min(mnx, n.rawX);
        mxx = std::max(mxx, n.rawX);
        mny = std::min(mny, n.rawY);
        mxy = std::max(mxy, n.rawY);
    }
    float aw = screenW - padL - padR;
    float ah = screenH - padT - padB;
    m_scale = std::min(aw / (mxx - mnx), ah / (mxy - mny));
    m_offX = padL + (aw - (mxx - mnx) * m_scale) / 2.0f;
    m_offY = padT + (ah - (mxy - mny) * m_scale) / 2.0f;
    m_minX = mnx;
    m_minY = mny;
    m_zoom = 1.0f;
    m_dragging = false;
}

void MapView::updateFromInputs() {
    Vector2 m = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        m_dragging = true;
        m_lastDrag = m;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) m_dragging = false;
    if (m_dragging) {
        Vector2 d{m.x - m_lastDrag.x, m.y - m_lastDrag.y};
        m_offX += d.x;
        m_offY += d.y;
        m_lastDrag = m;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        float factor = (wheel > 0) ? 1.15f : (1.0f / 1.15f);
        zoomAtScreen(m, factor);
    }
}

Vector2 MapView::screenToWorld(Vector2 s) const {
    float invZ = 1.0f / m_zoom;
    float dx = (s.x - m_offX) * invZ;
    float dy = (s.y - m_offY) * invZ;
    return Vector2{m_minX + dx / m_scale, m_minY + dy / m_scale};
}

Vector2 MapView::worldToScreen(Vector2 w) const {
    return Vector2{m_offX + (w.x - m_minX) * m_scale * m_zoom,
                   m_offY + (w.y - m_minY) * m_scale * m_zoom};
}

void MapView::panWorld(Vector2 deltaWorld) {
    m_offX -= deltaWorld.x * m_scale * m_zoom;
    m_offY -= deltaWorld.y * m_scale * m_zoom;
}

void MapView::zoomAtScreen(Vector2 anchor, float factor) {
    float newZoom = std::clamp(m_zoom * factor, m_minZoom, m_maxZoom);
    if (std::fabs(newZoom - m_zoom) < 1e-6f) return;
    // Keep the world point under anchor stable.
    Vector2 worldBefore = screenToWorld(anchor);
    m_zoom = newZoom;
    Vector2 screenAfter = worldToScreen(worldBefore);
    m_offX += anchor.x - screenAfter.x;
    m_offY += anchor.y - screenAfter.y;
}

bool MapView::containsScreenPoint(Vector2 p, float screenW, float screenH) const {
    return p.x >= 0 && p.x < screenW - 0 && p.y >= 0 && p.y < screenH;
}

} // namespace rm
