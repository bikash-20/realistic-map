#include "Sidebar.h"

#include <algorithm>

#include "Util.h"

namespace rm {

namespace {

// Clickable pill. Returns true exactly on the frame the pill is clicked.
bool pillButton(Rectangle r, const char* label, int fontSize, Color fg,
                Color bg, Color border)
{
    DrawRectangleRounded(r, 0.5f, 8, bg);
    DrawRectangleRoundedLinesEx(r, 0.5f, 8, 1.0f, border);
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)r.x + (int)r.width / 2 - tw / 2,
             (int)r.y + (int)r.height / 2 - fontSize / 2, fontSize, fg);
    return CheckCollisionPointRec(GetMousePosition(), r) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void infoCard(Rectangle r, const char* title, const char* value,
              Color titleCol, Color valueCol)
{
    DrawRectangleRounded(r, 0.18f, 8, Color{232,240,253,255});
    DrawText(title, (int)r.x + 12, (int)r.y + 8, 12, Color{128,134,139,255});
    DrawText(value, (int)r.x + 12, (int)r.y + 24, 18, Color{26,115,232,255});
    (void)titleCol;
    (void)valueCol;
}

} // namespace

void drawSidebar(Rectangle bounds,
                 SidebarState& state,
                 SidebarIO& io,
                 const UiCallbacks& cb)
{
    // --- shell ---
    DrawRectangle((int)bounds.x + 3, (int)bounds.y,
                  (int)bounds.width, (int)bounds.height,
                  Color{0, 0, 0, 30});
    DrawRectangleRec(bounds, WHITE);
    DrawLine((int)bounds.x + (int)bounds.width, (int)bounds.y,
             (int)bounds.x + (int)bounds.width, (int)bounds.height,
             Color{220, 220, 220, 255});

    // --- header ---
    DrawRectangleRec({bounds.x, bounds.y, bounds.width, 64}, WHITE);
    DrawLine((int)bounds.x, (int)bounds.y + 64,
             (int)bounds.x + (int)bounds.width, (int)bounds.y + 64,
             Color{230, 230, 230, 255});
    DrawCircleV({bounds.x + 24, bounds.y + 24}, 8, Color{234, 67, 53, 255});
    DrawCircleV({bounds.x + 24, bounds.y + 24}, 4, WHITE);
    DrawTriangle({bounds.x + 19, bounds.y + 30},
                 {bounds.x + 29, bounds.y + 30},
                 {bounds.x + 24, bounds.y + 38},
                 Color{234, 67, 53, 255});
    DrawText("Navigation", (int)bounds.x + 40, (int)bounds.y + 14,
             18, Color{50, 50, 50, 255});

    int y = (int)bounds.y + 80;

    // --- search ---
    Rectangle search{bounds.x + 12, (float)y, bounds.width - 24, 30};
    bool focused = CheckCollisionPointRec(GetMousePosition(), search);
    DrawRectangleRounded(search, 0.5f, 8, Color{241, 243, 244, 255});
    DrawRectangleRoundedLinesEx(search, 0.5f, 8, 1.0f,
                                focused ? Color{66, 133, 244, 255}
                                        : Color{220, 220, 220, 255});
    DrawText("Q  Search places", (int)search.x + 10, (int)search.y + 8,
             12, Color{128, 134, 139, 255});

    // very small input slot underneath (one line). For demo we just render
    // what the user typed above in tiny form.
    if (focused && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Nothing fancy; UX shows search suggestions below.
    }
    int k = GetCharPressed();
    if (focused) {
        while (k > 0) {
            if (k >= 32 && k < 127) io.searchBuffer.push_back((char)k);
            k = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !io.searchBuffer.empty())
            io.searchBuffer.pop_back();
    }
    if (!io.searchBuffer.empty()) {
        DrawText(io.searchBuffer.c_str(),
                 (int)search.x + 10, (int)search.y + 8, 12,
                 Color{50, 50, 50, 255});
    }

    // suggestions
    if (!io.searchBuffer.empty()) {
        Rectangle sug{search.x, search.y + search.height + 4,
                      search.width, 22};
        // Search suggestions are wired through the start/end pills below.
        Rectangle useStart{search.x, sug.y, sug.width / 2 - 2, 22};
        Rectangle useEnd  {search.x + sug.width / 2 + 2, sug.y, sug.width / 2 - 2, 22};
        if (pillButton(useStart, "Set as start", 11,
                       Color{52, 168, 83, 255}, WHITE,
                       Color{52, 168, 83, 255}))
            io.pendingStartNode = io.searchBuffer;
        if (pillButton(useEnd,   "Set as end",  11,
                       Color{234, 67, 53, 255}, WHITE,
                       Color{234, 67, 53, 255}))
            io.pendingEndNode = io.searchBuffer;
        y = (int)sug.y + 26;
    } else {
        y += 38;
    }

    // --- summary card ---
    if (state.path.size() >= 2) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f km", state.totalKm);
        infoCard({bounds.x + 12, (float)y, bounds.width - 24, 60},
                 "Fastest route", buf,
                 Color{128, 134, 139, 255},
                 Color{26, 115, 232, 255});
        snprintf(buf, sizeof(buf), "%.0f min", state.totalMinutes);
        DrawText(buf, (int)bounds.x + (int)bounds.width - 80,
                 (int)y + 24, 18, Color{26, 115, 232, 255});
        y += 70;

        // Chips row
        Rectangle chips{(float)bounds.x + 12, (float)y, bounds.width - 24, 36};
        DrawRectangleRounded(chips, 0.18f, 8, Color{248, 249, 250, 255});
        char stops[32];
        snprintf(stops, sizeof(stops), "%zu stops", state.path.size() - 2);
        DrawText(stops, (int)chips.x + 12, (int)chips.y + 11, 12,
                 Color{50, 50, 50, 255});
        snprintf(stops, sizeof(stops), "%.0f min", state.totalMinutes);
        DrawText(stops, (int)chips.x + 110, (int)chips.y + 11, 12,
                 Color{50, 50, 50, 255});
        snprintf(stops, sizeof(stops), "%.1f km", state.totalKm);
        DrawText(stops, (int)chips.x + 200, (int)chips.y + 11, 12,
                 Color{50, 50, 50, 255});
        y += 46;
    }

    // --- toggles row ---
    Rectangle tg1{bounds.x + 12, (float)y, (bounds.width - 36) / 3, 28};
    Rectangle tg2{tg1.x + tg1.width + 6, (float)y, (bounds.width - 36) / 3, 28};
    Rectangle tg3{tg2.x + tg2.width + 6, (float)y, (bounds.width - 36) / 3, 28};

    if (pillButton(tg1, "Traffic",  11, BLACK,
                   state.trafficEnabled ? Color{254,239,195,255} : WHITE,
                   Color{128,134,139,255}))
        io.pendingTrafficToggle = true;
    if (pillButton(tg2, "Alt",      11, BLACK,
                   state.showAlternative ? Color{232,240,253,255} : WHITE,
                   Color{128,134,139,255}))
        io.pendingAltToggle = true;
    if (pillButton(tg3, "Day/Night",11, BLACK,
                   state.nightMode ? Color{55, 65, 81,255} : WHITE,
                   Color{128,134,139,255}))
        io.pendingNightToggle = true;

    (void)cb;
    y += 38;

    // --- turn-by-turn ---
    if (state.path.size() >= 2) {
        DrawLine((int)bounds.x + 14, y,
                 (int)bounds.x + (int)bounds.width - 14, y,
                 Color{230, 230, 230, 255});
        y += 12;
        DrawText("Turn-by-turn",
                 (int)bounds.x + 14, y, 12, Color{128, 134, 139, 255});
        y += 22;

        int visibleSteps = std::min(
            (int)state.path.size(),
            ((int)bounds.y + (int)bounds.height - y - 60) / 38);
        for (int i = 0; i < visibleSteps; ++i) {
            bool isS = (i == 0);
            bool isE = (i == (int)state.path.size() - 1);

            if (i == state.animator.seg() && (int)state.path.size() >= 2)
                DrawRectangle((int)bounds.x, y - 2, (int)bounds.width, 36,
                              Color{232, 240, 253, 255});

            Color dotC = isS ? Color{52, 168, 83, 255}
                            : isE ? Color{234, 67, 53, 255}
                                  : Color{ 66,133,244,255};
            DrawCircleV({bounds.x + 20, (float)y + 14},
                        isS || isE ? 7.0f : 5.0f, dotC);
            if (isS || isE)
                DrawCircleV({bounds.x + 20, (float)y + 14}, 3, WHITE);
            if (i < visibleSteps - 1)
                DrawLine((int)bounds.x + 20, y + 21,
                         (int)bounds.x + 20, y + 38,
                         Color{200, 210, 230, 255});

            DrawText(state.path[i].c_str(),
                     (int)bounds.x + 34, y + 7, 13,
                     Color{50, 50, 50, 255});
            y += 38;
        }
    } else {
        DrawText("No path found",
                 (int)bounds.x + 14, y, 13, Color{200, 60, 60, 255});
    }

    // --- alternative path ---
    if (state.showAlternative && state.altPath.size() >= 2 &&
        y + 80 < bounds.y + bounds.height)
    {
        DrawLine((int)bounds.x + 14, y,
                 (int)bounds.x + (int)bounds.width - 14, y,
                 Color{230, 230, 230, 255});
        y += 12;
        DrawText("Alternative", (int)bounds.x + 14, y, 12,
                 Color{128, 134, 139, 255});
        y += 20;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f km  %.0f min",
                 state.altKm, state.altMinutes);
        DrawText(buf, (int)bounds.x + 14, y, 13, Color{120, 144, 156, 255});
        y += 22;
        DrawText(state.altPath.back().c_str(),
                 (int)bounds.x + 14, y, 12, Color{50, 50, 50, 255});
        y += 18;
    }

    // --- footer ---
    int fy = (int)(bounds.y + bounds.height - 18);
    DrawText("Map data (c) 2026  |  Dijkstra + A* routing",
             (int)bounds.x + 14, fy, 10, Color{128, 134, 139, 255});
}

} // namespace rm
