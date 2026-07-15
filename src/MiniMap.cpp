#include "MiniMap.h"

#include <algorithm>
#include <vector>

#include "NavSystem.h"
#include "Util.h"

namespace rm {

namespace {

struct MiniTransform {
    float sx, sy;     // raw world coord -> minimap pixel
    float ox, oy;     // pixel offset (top-left of rect)
};

MiniTransform computeMiniTransform(Rectangle rect, const NavSystem& nav) {
    if (nav.nodes.empty()) return {1.0f, 1.0f, rect.x, rect.y};
    float mnx = 1e9f, mxx = -1e9f, mny = 1e9f, mxy = -1e9f;
    for (const auto& [k, n] : nav.nodes) {
        mnx = std::min(mnx, n.rawX);
        mxx = std::max(mxx, n.rawX);
        mny = std::min(mny, n.rawY);
        mxy = std::max(mxy, n.rawY);
    }
    float w = std::max(1.0f, mxx - mnx);
    float h = std::max(1.0f, mxy - mny);
    // 10px inset on every side so nodes never touch the border.
    float pad = 10.0f;
    float sx = (rect.width  - 2 * pad) / w;
    float sy = (rect.height - 2 * pad) / h;
    float s  = std::min(sx, sy);
    // Center within the rect after applying uniform scale.
    float usedW = w * s, usedH = h * s;
    float ox = rect.x + (rect.width  - usedW) * 0.5f - mnx * s;
    float oy = rect.y + (rect.height - usedH) * 0.5f - mny * s;
    return {s, s, ox, oy};
}

inline Vector2 toMini(const MiniTransform& t, float wx, float wy) {
    return {t.ox + wx * t.sx, t.oy + wy * t.sy};
}

} // namespace

void drawMiniMap(Rectangle rect, const RenderState& s) {
    // --- panel ---
    DrawRectangleRec(rect, withAlpha(BLACK, s.nightMode ? 180 : 230));
    DrawRectangleLinesEx(rect, 1, withAlpha(s.nightMode ? WHITE : BLACK,
                                            s.nightMode ? 40 : 60));

    BeginScissorMode((int)rect.x + 1, (int)rect.y + 1,
                     (int)rect.width - 2, (int)rect.height - 2);

    const MiniTransform t = computeMiniTransform(rect, s.nav);

    // tiny palette re-used from the main renderer so the minimap matches.
    auto dayP  = Color{232, 224, 216, 255};
    auto nPark = Color{200, 230, 192, 255};
    auto nWat  = Color{170, 218, 255, 255};
    auto nBld  = Color{220, 213, 200, 255};
    auto nRoad = Color{220, 215, 205, 255};
    auto nNode = Color{255, 255, 255, 255};
    auto nStr  = Color{170, 170, 170, 255};
    if (s.nightMode) {
        dayP  = Color{ 30,  32,  38, 255};
        nPark = Color{ 38,  70,  50, 255};
        nWat  = Color{ 24,  60,  90, 255};
        nBld  = Color{ 60,  64,  72, 255};
        nRoad = Color{ 80,  82,  88, 255};
        nNode = Color{220, 222, 228, 255};
        nStr  = Color{120, 124, 132, 255};
    }
    (void)dayP;

    // --- decorations (simplified rectangles only, no outlines) ---
    for (const auto& pk : s.deco.parks) {
        Vector2 a = toMini(t, pk.x, pk.y);
        DrawRectangleRec({a.x, a.y, pk.w * t.sx, pk.h * t.sy}, nPark);
    }
    for (const auto& w : s.deco.waters) {
        Vector2 a = toMini(t, w.x, w.y);
        DrawRectangleRec({a.x, a.y, w.w * t.sx, w.h * t.sy}, nWat);
    }
    for (const auto& b : s.deco.blocks) {
        Vector2 a = toMini(t, b.x, b.y);
        DrawRectangleRec({a.x, a.y, b.w * t.sx, b.h * t.sy}, nBld);
    }

    // --- roads (thin lines, no class distinction) ---
    for (const auto& [src, edges] : s.nav.graph) {
        for (const auto& e : edges) {
            if (src > e.dest) continue;
            auto itA = s.nav.nodes.find(src);
            auto itB = s.nav.nodes.find(e.dest);
            if (itA == s.nav.nodes.end() || itB == s.nav.nodes.end()) continue;
            Vector2 a = toMini(t, itA->second.rawX, itA->second.rawY);
            Vector2 b = toMini(t, itB->second.rawX, itB->second.rawY);
            DrawLineEx(a, b, 1.0f, nRoad);
        }
    }

    // --- route (brighter so it stands out) ---
    Color routeC = s.nightMode ? Color{ 90,170,255,255}
                               : Color{ 66,133,244,255};
    for (std::size_t i = 0; i + 1 < s.path.size(); ++i) {
        auto itA = s.nav.nodes.find(s.path[i]);
        auto itB = s.nav.nodes.find(s.path[i + 1]);
        if (itA == s.nav.nodes.end() || itB == s.nav.nodes.end()) continue;
        Vector2 a = toMini(t, itA->second.rawX, itA->second.rawY);
        Vector2 b = toMini(t, itB->second.rawX, itB->second.rawY);
        DrawLineEx(a, b, 2.0f, routeC);
    }

    // --- nodes ---
    for (const auto& [k, n] : s.nav.nodes) {
        Vector2 p = toMini(t, n.rawX, n.rawY);
        DrawCircleV(p, 2.0f, nNode);
        DrawCircleLines((int)p.x, (int)p.y, 2.0f, nStr);
        // highlight start/end
        if (k == s.startNode) {
            DrawCircleV(p, 3.0f, Color{ 52,168, 83,255});
        } else if (k == s.endNode) {
            DrawCircleV(p, 3.0f, Color{234, 67, 53,255});
        }
    }

    // --- viewport indicator ---
    // Visible world rectangle = (minX, minY) to (minX + sw/scale/zoom, ...).
    float visW = s.view.screenW() / (s.view.scale() * s.view.zoom());
    float visH = s.view.screenH() / (s.view.scale() * s.view.zoom());
    float visX = s.view.minX();
    float visY = s.view.minY();
    Vector2 vTL = toMini(t, visX,                visY);
    Vector2 vBR = toMini(t, visX + visW,         visY + visH);
    Rectangle vp{vTL.x, vTL.y, vBR.x - vTL.x, vBR.y - vTL.y};
    Color vpCol = s.nightMode ? Color{220,222,228,160}
                              : Color{ 26,115,232,180};
    DrawRectangleLinesEx(vp, 1.0f, vpCol);

    EndScissorMode();

    // --- label ---
    DrawText("Overview", (int)rect.x + 6, (int)rect.y + 4, 10,
             withAlpha(s.nightMode ? WHITE : BLACK, 200));
}

void drawHoverTooltip(const RenderState& s) {
    if (s.hoverNode.empty()) return;
    auto it = s.nav.nodes.find(s.hoverNode);
    if (it == s.nav.nodes.end()) return;
    Vector2 np = it->second.pos;

    // collect up to 4 connected neighbours, sorted by distance
    struct Row { std::string name; float km; };
    std::vector<Row> rows;
    auto gIt = s.nav.graph.find(s.hoverNode);
    if (gIt != s.nav.graph.end()) {
        for (const auto& e : gIt->second) {
            rows.push_back({e.dest, e.distKm});
        }
    }
    std::sort(rows.begin(), rows.end(),
              [](const Row& a, const Row& b) { return a.km < b.km; });
    if (rows.size() > 4) rows.resize(4);

    // Card sizing
    const int titleH = 16;
    const int rowH   = 13;
    const int padX   = 8;
    const int padY   = 6;
    int cardW = 140;
    for (const auto& r : rows) {
        int w = MeasureText(r.name.c_str(), 11) +
                MeasureText("99.9 km", 11) + 20;
        cardW = std::max(cardW, w + padX * 2);
    }
    int cardH = titleH + padY * 2 +
                (int)rows.size() * rowH + (rows.empty() ? 0 : 2);

    // Position card just above-right of the node, clamping to screen.
    float cardX = np.x + 16;
    float cardY = np.y - cardH - 8;
    if (cardX + cardW > s.view.screenW() - 4) cardX = np.x - cardW - 16;
    if (cardY < 4) cardY = np.y + 16;

    Color bg     = s.nightMode ? Color{ 20, 22, 28, 230}
                               : Color{255,255,255,235};
    Color border = s.nightMode ? Color{ 60, 66, 76, 255}
                               : Color{200,200,200,255};
    Color title  = s.nightMode ? Color{220,222,228,255}
                               : Color{ 26,115,232,255};
    Color sub    = s.nightMode ? Color{170,174,182,255}
                               : Color{ 80, 80, 80, 255};

    DrawRectangleRounded({cardX, cardY, (float)cardW, (float)cardH},
                         0.10f, 6, bg);
    DrawRectangleRoundedLinesEx({cardX, cardY, (float)cardW, (float)cardH},
                                0.10f, 6, 1.0f, border);

    DrawText(s.hoverNode.c_str(), (int)cardX + padX, (int)cardY + padY,
             12, title);

    int y = (int)cardY + padY + titleH;
    if (rows.empty()) {
        DrawText("(no connections)", (int)cardX + padX, y, 10, sub);
    } else {
        for (const auto& r : rows) {
            DrawText(r.name.c_str(), (int)cardX + padX, y, 11, sub);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f km", r.km);
            int tw = MeasureText(buf, 11);
            DrawText(buf, (int)cardX + cardW - padX - tw, y, 11, sub);
            y += rowH;
        }
    }
}

} // namespace rm