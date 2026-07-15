#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "raylib.h"

#include "AStar.h"
#include "Config.h"
#include "MapData.h"
#include "MapRenderer.h"
#include "MapView.h"
#include "NavSystem.h"
#include "RouteAnimator.h"
#include "Sidebar.h"
#include "Util.h"

using namespace rm;

namespace {

Color clearColor(bool night) {
    return night ? Color{30, 32, 38, 255} : Color{232, 224, 216, 255};
}

void drawZoomButtons(Rectangle bounds, MapView& view) {
    DrawRectangleRounded(bounds, 0.18f, 8, WHITE);
    DrawRectangleRoundedLinesEx(bounds, 0.18f, 8, 1,
                              Color{200, 200, 200, 255});
    DrawLine((int)bounds.x, (int)bounds.y + (int)bounds.height / 2,
             (int)bounds.x + (int)bounds.width, (int)bounds.y + (int)bounds.height / 2,
             Color{200, 200, 200, 255});
    DrawText("+", (int)bounds.x + (int)bounds.width / 2 - 6,
             (int)bounds.y + 6, 22, Color{50, 50, 50, 255});
    DrawText("-", (int)bounds.x + (int)bounds.width / 2 - 6,
             (int)bounds.y + (int)bounds.height / 2 + 4,
             22, Color{50, 50, 50, 255});

    Rectangle plus {bounds.x,                bounds.y, bounds.width, bounds.height / 2};
    Rectangle minus{bounds.x, bounds.y + bounds.height / 2, bounds.width, bounds.height / 2};
    if (CheckCollisionPointRec(GetMousePosition(), plus) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        view.zoomAtScreen({bounds.x - 50, bounds.y + bounds.height / 4}, 1.15f);
    }
    if (CheckCollisionPointRec(GetMousePosition(), minus) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        view.zoomAtScreen({bounds.x - 50, bounds.y + 3 * bounds.height / 4}, 1.0f / 1.15f);
    }
}

// Hit-tests a node in raw-world coordinates (independent of zoom).
std::string hoverNode(const NavSystem& nav, Vector2 mp,
                      float defaultR, float zoom)
{
    float r = defaultR / std::max(0.6f, zoom); // smaller hit area when zoomed out
    for (const auto& [name, node] : nav.nodes) {
        if (CheckCollisionPointCircle(mp, node.pos, r)) return name;
    }
    return "";
}

} // namespace

int main() {
    Config cfg{};

    InitWindow(cfg.windowW, cfg.windowH,
               "realistic-map  -  Dijkstra / A* Navigation");
    SetTargetFPS(cfg.targetFps);

    // --- load map ---
    NavSystem nav;
    auto spec = loadCitySpec("data/city.json");
    for (auto& [n, x, y] : spec.nodes) nav.addNode(n, x, y);
    for (auto& [a, b, km, rc, ow] : spec.routes) {
        nav.addRoute(a, b, km, static_cast<RoadClass>(rc), ow);
    }
    if (nav.nodes.empty()) return 1;

    // --- decoration (parks / water / buildings) ---------------------------
    // Reuse the same default city in the demo, regardless of loaded city.
    MapDecoration deco;
    deco.parks = {
        {30, 30,130, 90},{590, 30,110,110},{340,320, 90, 70},
        {60,460,100, 80},{480,430,120, 90}
    };
    deco.waters = {
        {610, 30,130,150},{0,590,210,110}
    };
    deco.blocks = {
        {190, 70, 55, 45},{265, 60, 45, 55},
        {440,140, 60, 40},{510,150, 45, 50},
        {180,290, 50, 40},{235,300, 55, 35},
        {400,270, 45, 45},{450,280, 55, 40},
        {310,430, 60, 40},{375,440, 50, 45},
        {190,550, 45, 40},{240,545, 55, 45},
        {450,540, 60, 45},{510,535, 45, 50},
        {560,220, 55, 45},{615,230, 50, 40}
    };

    // --- view ---
    MapView view;
    view.reset(nav, (float)cfg.windowW, (float)cfg.windowH,
               cfg.padL, cfg.padR, cfg.padT, cfg.padB);
    for (auto& [k, n] : nav.nodes) {
        n.pos = view.worldToScreen({n.rawX, n.rawY});
    }

    // --- start/end ---
    std::string startNode = "Westfield";
    std::string endNode   = "Airport";

    auto computeRoute = [&]() {
        auto path = aStar(nav, startNode, endNode, cfg.trafficEnabled);
        return path;
    };
    auto computeAlt = [&](const std::vector<std::string>& best) {
        return nav.findAlternative(startNode, endNode, best);
    };
    std::vector<std::string> path    = computeRoute();
    std::vector<std::string> altPath = computeAlt(path);

    // --- animator ---
    RouteAnimator animator;
    animator.setPath(path);

    SidebarState sbState{
        startNode, endNode,
        path, altPath,
        nav.routeDist(path),
        0.0f, nav.routeDist(altPath),
        0.0f,
        cfg.trafficEnabled,
        cfg.showAlternative,
        cfg.nightMode,
        animator,
    };

    // minute estimates use class-aware edge weights
    auto edgeMinutesSum = [&](const std::vector<std::string>& p) {
        float total = 0.0f;
        for (std::size_t i = 0; i + 1 < p.size(); ++i) {
            if (auto* e = nav.findEdge(p[i], p[i + 1]))
                total += nav.edgeWeight(*e, cfg.trafficEnabled);
        }
        return total;
    };
    sbState.totalMinutes = edgeMinutesSum(path);
    sbState.altMinutes   = edgeMinutesSum(altPath);

    // --- helpers ---
    auto recomputeAll = [&]() {
        path    = aStar(nav, startNode, endNode, cfg.trafficEnabled);
        altPath = nav.findAlternative(startNode, endNode, path);
        animator.setPath(path);
        sbState.path      = path;
        sbState.altPath   = altPath;
        sbState.startNode = startNode;
        sbState.endNode   = endNode;
        sbState.totalKm   = nav.routeDist(path);
        sbState.altKm     = nav.routeDist(altPath);
        sbState.totalMinutes = edgeMinutesSum(path);
        sbState.altMinutes   = edgeMinutesSum(altPath);
    };

    UiCallbacks cb{
        [&](const std::string& n){ if (n != endNode) { startNode = n; recomputeAll(); } },
        [&](const std::string& n){ if (n != startNode) { endNode = n;   recomputeAll(); } },
        [&](){ recomputeAll(); },
        [&](bool on){ (void)cfg.trafficEnabled; recomputeAll(); (void)on; },
        [&](bool on){ cfg.showAlternative = on; sbState.showAlternative = on; (void)on; },
        [&](bool on){ cfg.nightMode = on; sbState.nightMode = on; (void)on; },
    };

    SidebarIO io{};

    float dashTimer = 0.0f;
    bool  hasArrivedShown = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        dashTimer += dt * cfg.dashSpeed;

        // --- input ---
        view.updateFromInputs();
        Vector2 mp = GetMousePosition();
        std::string hover = hoverNode(nav, mp, 14.0f, view.zoom());

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (!hover.empty()) { endNode   = hover; recomputeAll(); }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (!hover.empty()) { startNode = hover; recomputeAll(); }
        }

        // --- request pipeline from sidebar ---
        if (!io.pendingStartNode.empty()) {
            if (nav.nodes.count(io.pendingStartNode)) {
                startNode = io.pendingStartNode; recomputeAll();
            }
            io.pendingStartNode.clear();
        }
        if (!io.pendingEndNode.empty()) {
            if (nav.nodes.count(io.pendingEndNode)) {
                endNode = io.pendingEndNode; recomputeAll();
            }
            io.pendingEndNode.clear();
        }
        if (io.pendingTrafficToggle) {
            cfg.trafficEnabled = !cfg.trafficEnabled;
            sbState.trafficEnabled = cfg.trafficEnabled;
            recomputeAll();
            io.pendingTrafficToggle = false;
        }
        if (io.pendingAltToggle) {
            cfg.showAlternative = !cfg.showAlternative;
            sbState.showAlternative = cfg.showAlternative;
            io.pendingAltToggle = false;
        }
        if (io.pendingNightToggle) {
            cfg.nightMode = !cfg.nightMode;
            sbState.nightMode = cfg.nightMode;
            io.pendingNightToggle = false;
        }
        if (io.requestRecompute) { recomputeAll(); io.requestRecompute = false; }

        // --- animator (smooth arc-length) ---
        if (animator.state() == AnimState::Driving &&
            path.size() >= 2 && animator.seg() < (int)path.size() - 1)
        {
            int i = animator.seg();
            Vector2 a = nav.nodes.at(path[i]).pos;
            Vector2 b = nav.nodes.at(path[i + 1]).pos;
            float segLen = Vec2Len({b.x - a.x, b.y - a.y});
            // Slower on minor roads, faster on highways.
            if (auto* e = nav.findEdge(path[i], path[i + 1])) {
                float k = 1.0f;
                switch (e->roadClass) {
                    case RoadClass::Highway: k = 1.5f; break;
                    case RoadClass::Major:   k = 1.1f; break;
                    case RoadClass::Minor:   k = 0.8f; break;
                    case RoadClass::Local:   k = 0.55f; break;
                }
                if (segLen > 0)
                    animator.update(dt, cfg.carSpeedPx * k);
                else
                    animator.update(dt, 0.0f);
            } else {
                animator.update(dt, 0.0f);
            }
        } else if (animator.state() == AnimState::Driving) {
            animator.update(dt, 0.0f);
        } else {
            // Arrived: hold a moment then loop.
            animator.update(dt, 0.0f);
            if (!hasArrivedShown && animator.arrivedTimer() > cfg.arrivedHoldSeconds) {
                hasArrivedShown = true;
                animator.setPath(path); // loop
            }
            if (hasArrivedShown && animator.state() == AnimState::Driving) {
                hasArrivedShown = false;
            }
        }

        // --- draw ---
        BeginDrawing();
        ClearBackground(clearColor(cfg.nightMode));

        RenderState rs{nav, view, deco, path, altPath,
                       startNode, endNode, hover, cfg.nightMode, dashTimer};
        drawDecorations(rs);
        drawRoadNetwork(rs);

        if (cfg.showAlternative && altPath.size() >= 2) {
            drawRoute(rs, altPath, Color{120, 144, 156, 220},
                      false, true);
        }
        if (path.size() >= 2) {
            drawRoute(rs, path, Color{66, 133, 244, 255},
                      true, true);
        }

        drawNodes(rs);

        // --- car ---
        if (path.size() >= 2 && animator.seg() < (int)path.size() - 1) {
            Vector2 a = nav.nodes.at(path[animator.seg()]).pos;
            Vector2 b = nav.nodes.at(path[animator.seg() + 1]).pos;
            Vector2 pos{a.x + (b.x - a.x) * animator.frac(),
                        a.y + (b.y - a.y) * animator.frac()};
            float angle = atan2f(b.y - a.y, b.x - a.x);
            DrawCircleV({pos.x + 2, pos.y + 2}, 10, Color{0, 0, 0, 60});
            DrawCircleV(pos, 10, Color{251, 188, 4, 255});
            DrawCircleLines((int)pos.x, (int)pos.y, 10,
                            Color{210, 155, 0, 255});
            float ax = pos.x + cosf(angle) * 6;
            float ay = pos.y + sinf(angle) * 6;
            float bx = pos.x + cosf(angle + 2.3f) * 5;
            float by = pos.y + sinf(angle + 2.3f) * 5;
            float cx = pos.x + cosf(angle - 2.3f) * 5;
            float cy = pos.y + sinf(angle - 2.3f) * 5;
            DrawTriangle({ax, ay}, {bx, by}, {cx, cy},
                         Color{100, 60, 0, 220});
        } else if (animator.atDestination()) {
            // Big "Arrived" caption over the destination node
            Vector2 d = nav.nodes.at(endNode).pos;
            const char* msg = "Arrived";
            int sz = 22;
            int tw = MeasureText(msg, sz);
            DrawRectangle((int)d.x - tw / 2 - 10, (int)d.y - 50,
                          tw + 20, 32, Color{52, 168, 83, 255});
            DrawText(msg, (int)d.x - tw / 2, (int)d.y - 46, sz, WHITE);
        }

        // --- sidebar ---
        drawSidebar({0.0f, 0.0f, cfg.sidebarW, (float)cfg.windowH},
                    sbState, io, cb);

        // --- zoom controls ---
        drawZoomButtons({(float)(cfg.windowW - 48), (float)(cfg.windowH - 100), 36, 80}, view);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
