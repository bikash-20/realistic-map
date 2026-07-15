#include "MapRenderer.h"

#include <algorithm>

#include "Util.h"

namespace rm {

namespace {

// --- Palette (day / night) --------------------------------------------------
struct Palette {
    Color bg, park, parkLine, water, building, roadShadow;
    Color roadHigh, roadHighFill, roadMajor, roadMajorFill;
    Color roadMinor, roadMinorFill, roadLocal, roadLocalFill;
    Color pathBase, pathDash, pathAlt, node, nodeStr;
    Color start, end, startLine, endLine, onPath, onPathLine;
    Color label, muted, car, carLine;
};

Palette dayPalette() {
    return Palette{
        .bg            = {232,224,216,255},
        .park          = {200,230,192,255},
        .parkLine      = {170,210,160,255},
        .water         = {170,218,255,255},
        .building      = {220,213,200,255},
        .roadShadow    = {200,193,180,255},
        .roadHigh      = {255,245,200,255},  .roadHighFill = {255,224,130,255},
        .roadMajor     = {255,255,255,255},  .roadMajorFill= {255,255,255,255},
        .roadMinor     = {245,241,236,255},  .roadMinorFill= {250,247,242,255},
        .roadLocal     = {240,236,228,255},  .roadLocalFill= {248,245,238,255},
        .pathBase      = { 66,133,244,255},
        .pathDash      = {255,255,255,180},
        .pathAlt       = {120,144,156,220},
        .node          = {255,255,255,255},
        .nodeStr       = {170,170,170,255},
        .start         = { 52,168, 83,255},
        .end           = {234, 67, 53,255},
        .startLine     = { 40,130, 60,255},
        .endLine       = {180, 50, 40,255},
        .onPath        = { 66,133,244,255},
        .onPathLine    = { 40,100,200,255},
        .label         = { 50, 50, 50,255},
        .muted         = {128,134,139,255},
        .car           = {251,188,  4,255},
        .carLine       = {210,155,  0,255},
    };
}

Palette nightPalette() {
    return Palette{
        .bg            = { 30, 32, 38,255},
        .park          = { 38, 70, 50,255},
        .parkLine      = { 28, 56, 36,255},
        .water         = { 24, 60, 90,255},
        .building      = { 60, 64, 72,255},
        .roadShadow    = { 18, 20, 24,255},
        .roadHigh      = {120, 96,  0,255}, .roadHighFill = {160,120, 30,255},
        .roadMajor     = { 80, 80, 85,255}, .roadMajorFill = {110,110,118,255},
        .roadMinor     = { 60, 62, 68,255}, .roadMinorFill = { 90, 94,100,255},
        .roadLocal     = { 50, 52, 58,255}, .roadLocalFill = { 76, 80, 88,255},
        .pathBase      = { 90,170,255,255},
        .pathDash      = {220,235,255,180},
        .pathAlt       = {120,150,180,180},
        .node          = {220,222,228,255},
        .nodeStr       = {120,124,132,255},
        .start         = { 80,200,110,255},
        .end           = {250, 90, 80,255},
        .startLine     = { 40,140, 70,255},
        .endLine       = {180, 50, 40,255},
        .onPath        = { 90,170,255,255},
        .onPathLine    = { 40,100,200,255},
        .label         = {220,222,228,255},
        .muted         = {150,154,160,255},
        .car           = {251,188,  4,255},
        .carLine       = {210,155,  0,255},
    };
}

const Palette& activePalette(bool night) {
    static Palette d = dayPalette();
    static Palette n = nightPalette();
    return night ? n : d;
}

void roadClassStyle(RoadClass rc, const Palette& p,
                    Color& outline, Color& fill, float& thickness)
{
    switch (rc) {
        case RoadClass::Highway:
            outline = p.roadHigh;   fill = p.roadHighFill;  thickness = 12.0f; break;
        case RoadClass::Major:
            outline = p.roadMajor;  fill = p.roadMajorFill; thickness =  9.0f; break;
        case RoadClass::Minor:
            outline = p.roadMinor;  fill = p.roadMinorFill; thickness =  6.0f; break;
        case RoadClass::Local:
            outline = p.roadLocal;  fill = p.roadLocalFill; thickness =  4.5f; break;
    }
}

[[maybe_unused]] static std::string hitNodeScreen(const NavSystem& nav, Vector2 mp, float r) {
    for (const auto& [name, node] : nav.nodes) {
        if (CheckCollisionPointCircle(mp, node.pos, r)) return name;
    }
    return "";
}

[[maybe_unused]] static void drawCurvedSegment(Vector2 a, Vector2 b, Vector2 ctrl,
                                            float thick, Color c)
{
    DrawQuadBezier(a, ctrl, b, thick, c, 28);
}

[[maybe_unused]] static Vector2 perpNudge(Vector2 a, Vector2 b, float k) {
    Vector2 dir = Vec2Norm({b.x - a.x, b.y - a.y});
    return Vector2{-dir.y * k, dir.x * k};
}

} // namespace

void drawDecorations(const RenderState& s) {
    const Palette& p = activePalette(s.nightMode);
    for (const auto& pk : s.deco.parks) {
        Rectangle r{
            s.view.offsetX() + (pk.x - s.view.minX()) * s.view.scale() * s.view.zoom(),
            s.view.offsetY() + (pk.y - s.view.minY()) * s.view.scale() * s.view.zoom(),
            pk.w * s.view.scale() * s.view.zoom(),
            pk.h * s.view.scale() * s.view.zoom(),
        };
        DrawRectangleRec(r, p.park);
        DrawRectangleLinesEx(r, 1, p.parkLine);
    }
    for (const auto& w : s.deco.waters) {
        Rectangle r{
            s.view.offsetX() + (w.x - s.view.minX()) * s.view.scale() * s.view.zoom(),
            s.view.offsetY() + (w.y - s.view.minY()) * s.view.scale() * s.view.zoom(),
            w.w * s.view.scale() * s.view.zoom(),
            w.h * s.view.scale() * s.view.zoom(),
        };
        DrawRectangleRec(r, p.water);
    }
    for (const auto& b : s.deco.blocks) {
        Rectangle r{
            s.view.offsetX() + (b.x - s.view.minX()) * s.view.scale() * s.view.zoom(),
            s.view.offsetY() + (b.y - s.view.minY()) * s.view.scale() * s.view.zoom(),
            b.w * s.view.scale() * s.view.zoom(),
            b.h * s.view.scale() * s.view.zoom(),
        };
        DrawRectangleRec(r, p.building);
    }
}

void drawRoadNetwork(const RenderState& s) {
    const Palette& p = activePalette(s.nightMode);
    for (const auto& [src, edges] : s.nav.graph) {
        Vector2 a = s.nav.nodes.at(src).pos;
        for (const auto& e : edges) {
            // Render each undirected road only once.
            if (src > e.dest) continue;
            Vector2 b = s.nav.nodes.at(e.dest).pos;
            Color outline, fill; float thick;
            roadClassStyle(e.roadClass, p, outline, fill, thick);
            // Shadow stripe (slightly wider dark stroke under the road)
            DrawLineEx(a, b, thick + 2.0f, withAlpha(p.roadShadow, 180));
            DrawLineEx(a, b, thick,         fill);
            DrawLineEx(a, b, 1.2f,          withAlpha(outline, 160));

            // Direction chevron — small arrow 60% along the segment.
            Vector2 dir = Vec2Norm({b.x - a.x, b.y - a.y});
            float tpos = 0.6f;
            Vector2 tip{a.x + dir.x * (b.x - a.x) * tpos + dir.x * 6,
                        a.y + dir.y * (b.y - a.y) * tpos + dir.y * 6};
            Vector2 base{a.x + dir.x * (b.x - a.x) * tpos,
                         a.y + dir.y * (b.y - a.y) * tpos};
            Vector2 perp{-dir.y, dir.x};
            Vector2 l1{base.x + perp.x * 4, base.y + perp.y * 4};
            Vector2 r1{base.x - perp.x * 4, base.y - perp.y * 4};
            DrawTriangle(tip, l1, r1, withAlpha(outline, 200));
        }
    }
}

void drawRoute(const RenderState& s, const std::vector<std::string>& path,
               Color base, bool animated, bool dashed)
{
    if (path.size() < 2) return;
    const Palette& p = activePalette(s.nightMode);
    Color outline = withAlpha(p.roadShadow, 160);

    // 1) underlay shadow
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        Vector2 a = s.nav.nodes.at(path[i]).pos;
        Vector2 b = s.nav.nodes.at(path[i + 1]).pos;
        DrawLineEx(a, b, 11.0f, outline);
    }
    // 2) main color
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        Vector2 a = s.nav.nodes.at(path[i]).pos;
        Vector2 b = s.nav.nodes.at(path[i + 1]).pos;
        DrawLineEx(a, b, 7.0f, base);
    }
    // 3) animated chevrons
    if (animated) {
        for (std::size_t i = 0; i + 1 < path.size(); ++i) {
            Vector2 a = s.nav.nodes.at(path[i]).pos;
            Vector2 b = s.nav.nodes.at(path[i + 1]).pos;
            Vector2 dir = Vec2Norm({b.x - a.x, b.y - a.y});
            float total = Vec2Len({b.x - a.x, b.y - a.y});
            float stride = 20.0f;
            float offset = fmodf(s.dashOffset, stride);
            float tt = -offset;
            while (tt < total) {
                float t0 = std::max(tt, 0.0f);
                float t1 = std::min(tt + 8.0f, total);
                if (t1 > t0) {
                    Vector2 p0{a.x + dir.x * t0, a.y + dir.y * t0};
                    Vector2 p1{a.x + dir.x * t1, a.y + dir.y * t1};
                    DrawLineEx(p0, p1, 3.0f, p.pathDash);
                }
                tt += stride;
            }
        }
    }
    (void)dashed;
}

void drawNodes(const RenderState& s) {
    const Palette& p = activePalette(s.nightMode);
    for (const auto& [name, node] : s.nav.nodes) {
        bool isStart = name == s.startNode;
        bool isEnd   = name == s.endNode;
        bool isHover = name == s.hoverNode;
        bool onPath  = std::find(s.path.begin(), s.path.end(), name) != s.path.end();

        float r = isStart || isEnd ? 13.0f : onPath ? 10.0f : 8.0f;
        if (isHover) r += 1.5f;

        Color fill   = isStart ? p.start : isEnd ? p.end :
                       onPath  ? p.onPath : p.node;
        Color stroke = isStart ? p.startLine : isEnd ? p.endLine :
                       onPath  ? p.onPathLine : p.nodeStr;

        DrawCircleV({node.pos.x + 2, node.pos.y + 2}, r + 1,
                    withAlpha(BLACK, 50));
        DrawCircleV(node.pos, r, fill);
        DrawCircleLines((int)node.pos.x, (int)node.pos.y, (int)r, stroke);
        if (isStart || isEnd) DrawCircleV(node.pos, 4.0f, WHITE);

        if (isStart || isEnd || isHover || onPath) {
            int fs = 11;
            int tw = MeasureText(name.c_str(), fs);
            DrawRectangle((int)node.pos.x - tw / 2 - 3,
                          (int)node.pos.y - (int)r - 18,
                          tw + 6, 15,
                          withAlpha(WHITE, s.nightMode ? 220 : 230));
            DrawText(name.c_str(),
                     (int)node.pos.x - tw / 2,
                     (int)node.pos.y - (int)r - 17,
                     fs, p.label);
        }
    }
}

} // namespace rm
