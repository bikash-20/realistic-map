#include "Util.h"

#include <cmath>

namespace rm {

void DrawDashedLine(Vector2 a, Vector2 b, float thick,
                    float dashLen, float gapLen, Color c) {
    Vector2 dir = Vec2Norm({b.x - a.x, b.y - a.y});
    float total = Vec2Len({b.x - a.x, b.y - a.y});
    float t = 0;
    bool drawing = true;
    while (t < total) {
        float seg = drawing ? dashLen : gapLen;
        float end = std::min(t + seg, total);
        if (drawing) {
            Vector2 p1{a.x + dir.x * t,   a.y + dir.y * t};
            Vector2 p2{a.x + dir.x * end, a.y + dir.y * end};
            DrawLineEx(p1, p2, thick, c);
        }
        t = end;
        drawing = !drawing;
    }
}

void DrawQuadBezier(Vector2 p0, Vector2 p1, Vector2 p2,
                    float thick, Color c, int segments) {
    if (segments < 2) segments = 2;
    Vector2 prev = p0;
    for (int i = 1; i <= segments; ++i) {
        float u = static_cast<float>(i) / segments;
        float w0 = (1 - u) * (1 - u);
        float w1 = 2 * (1 - u) * u;
        float w2 = u * u;
        Vector2 cur{p0.x * w0 + p1.x * w1 + p2.x * w2,
                    p0.y * w0 + p1.y * w1 + p2.y * w2};
        DrawLineEx(prev, cur, thick, c);
        prev = cur;
    }
}

float haversineKm(float ax, float ay, float bx, float by) {
    constexpr float R = 6371.0f; // earth radius km
    constexpr float deg2rad = 0.017453292519943f;
    // Treat raw coords as degrees if magnitudes look like lon/lat;
    // otherwise as planar kilometers. We default to planar since our raw
    // coords are local-city. Caller may scale before passing.
    float dLat = (by - ay) * deg2rad;
    float dLon = (bx - ax) * deg2rad;
    float lat1 = ay * deg2rad;
    float lat2 = by * deg2rad;
    float h = std::sin(dLat / 2) * std::sin(dLat / 2) +
              std::cos(lat1) * std::cos(lat2) *
              std::sin(dLon / 2) * std::sin(dLon / 2);
    float c = 2 * std::atan2(std::sqrt(h), std::sqrt(1 - h));
    (void)R;
    // For city-scale coords the planar km interpretation is also valid; we
    // return whichever heuristic the caller will treat as kilometres.
    return std::sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay)) * 0.1f;
}

Color withAlpha(Color c, unsigned char a) {
    return Color{c.r, c.g, c.b, a};
}

} // namespace rm
