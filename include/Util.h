#pragma once
#include "raylib.h"
#include <string>

namespace rm {

inline float Vec2Len(Vector2 v) { return sqrtf(v.x*v.x + v.y*v.y); }
inline Vector2 Vec2Norm(Vector2 v) {
    float l = Vec2Len(v);
    return l > 0 ? Vector2{v.x / l, v.y / l} : Vector2{0, 0};
}

// Draw a dashed line from a to b using screen-space dash/gap sizes.
void DrawDashedLine(Vector2 a, Vector2 b, float thick,
                    float dashLen, float gapLen, Color c);

// Draw a quadratic Bezier curve between two points with a single control point.
void DrawQuadBezier(Vector2 p0, Vector2 p1, Vector2 p2,
                    float thick, Color c, int segments = 24);

// A simple 2-arg haversine (rough approximation on a flat map; good enough
// for an A* heuristic at city scale where curvature is negligible).
float haversineKm(float ax, float ay, float bx, float by);

Color withAlpha(Color c, unsigned char a);

} // namespace rm
