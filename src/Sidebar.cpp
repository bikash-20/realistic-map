#include "Sidebar.h"

#include <algorithm>
#include <cctype>
#include <vector>

#include "NavSystem.h"
#include "Util.h"

namespace rm {

namespace {

// Lowercase a string for case-insensitive comparison.
std::string toLower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// Score a candidate name against a query.
// 100 = exact match, 80 = prefix, 40 = contains, 0 = no match.
int matchScore(const std::string& name, const std::string& qLower) {
    if (qLower.empty()) return 0;
    auto n = toLower(name);
    if (n == qLower) return 100;
    if (n.rfind(qLower, 0) == 0) return 80;
    if (n.find(qLower) != std::string::npos) return 40;
    return 0;
}

// --- Palette (day / night) --------------------------------------------------
struct SidebarPalette {
    Color shellShadow;     // outer drop-shadow strip behind the sidebar
    Color bg;              // main sidebar background
    Color border;          // outer right border line
    Color headerBg;        // header band
    Color headerDivider;   // line under the header
    Color headerBrand;     // logo dot/arrow
    Color headerTitle;     // "Navigation" title text
    Color headerInner;     // small inner-circle inside the logo

    Color searchBg;        // search-box background
    Color searchBorder;    // search-box border (idle)
    Color searchBorderOn;  // search-box border (focused)
    Color searchPlaceholder;
    Color searchText;
    Color searchIcon;

    Color chipBg;          // summary card "total km / min" background
    Color chipText;        // chip text color (dark on day, near-white at night)

    Color cardBg;          // info card background (Fastest route etc.)
    Color cardTitle;       // card title (muted)
    Color cardValue;       // card primary value (highlight)

    Color toggleIdleBg;    // toggle pill bg (off)
    Color toggleTrafficBg; // toggle pill bg (traffic on)
    Color toggleAltBg;     // toggle pill bg (alt on)
    Color toggleNightBg;   // toggle pill bg (night on)
    Color toggleBorder;    // toggle pill border
    Color toggleFg;        // toggle pill foreground text

    Color panelBg;         // suggestion dropdown panel bg
    Color panelBorder;     // suggestion dropdown border
    Color panelHover;      // suggestion row hover
    Color matchDot;        // small dot beside each suggestion
    Color suggText;        // suggestion row text
    Color startDot;        // "Start" pill fg/border
    Color endDot;          // "End" pill fg/border

    Color sectionDivider;  // horizontal hairline divider
    Color sectionTitle;    // "Turn-by-turn" / "Alternative" small caps
    Color stepLine;        // vertical line connecting route steps
    Color stepText;        // step name text
    Color mutedText;       // secondary line under a step (distance + from→to)
    Color stepActive;      // background row highlight (current animator seg)

    Color altMeta;         // alternative "X km Y min" text
    Color errorText;       // "No path found" text
    Color footer;          // bottom-of-sidebar footer text
};

SidebarPalette daySidebar() {
    return SidebarPalette{
        .shellShadow     = {  0,  0,  0, 30},
        .bg              = {255,255,255,255},
        .border          = {220,220,220,255},
        .headerBg        = {255,255,255,255},
        .headerDivider   = {230,230,230,255},
        .headerBrand     = {234, 67, 53,255},
        .headerTitle     = { 50, 50, 50,255},
        .headerInner     = {255,255,255,255},

        .searchBg        = {241,243,244,255},
        .searchBorder    = {220,220,220,255},
        .searchBorderOn  = { 66,133,244,255},
        .searchPlaceholder= {128,134,139,255},
        .searchText      = { 50, 50, 50,255},
        .searchIcon      = {128,134,139,255},

        .chipBg          = {248,249,250,255},
        .chipText        = { 50, 50, 50,255},

        .cardBg          = {232,240,253,255},
        .cardTitle       = {128,134,139,255},
        .cardValue       = { 26,115,232,255},

        .toggleIdleBg    = {255,255,255,255},
        .toggleTrafficBg = {254,239,195,255},
        .toggleAltBg     = {232,240,253,255},
        .toggleNightBg   = { 55, 65, 81,255},
        .toggleBorder    = {128,134,139,255},
        .toggleFg        = {  0,  0,  0,255},

        .panelBg         = {255,255,255,240},
        .panelBorder     = {220,220,220,255},
        .panelHover      = {232,240,253,255},
        .matchDot        = { 66,133,244,220},
        .suggText        = { 50, 50, 50,255},
        .startDot        = { 52,168, 83,255},
        .endDot          = {234, 67, 53,255},

        .sectionDivider  = {230,230,230,255},
        .sectionTitle    = {128,134,139,255},
        .stepLine        = {200,210,230,255},
        .stepText        = { 50, 50, 50,255},
        .mutedText       = {120,128,138,255},
        .stepActive      = {232,240,253,255},

        .altMeta         = {120,144,156,255},
        .errorText       = {200, 60, 60,255},
        .footer          = {128,134,139,255},
    };
}

SidebarPalette nightSidebar() {
    return SidebarPalette{
        .shellShadow     = {  0,  0,  0, 90},
        .bg              = { 22, 24, 30,255},
        .border          = { 44, 48, 56,255},
        .headerBg        = { 18, 20, 26,255},
        .headerDivider   = { 44, 48, 56,255},
        .headerBrand     = {250, 90, 80,255},
        .headerTitle     = {220,222,228,255},
        .headerInner     = { 22, 24, 30,255},

        .searchBg        = { 34, 38, 46,255},
        .searchBorder    = { 60, 66, 76,255},
        .searchBorderOn  = {120,170,255,255},
        .searchPlaceholder= {150,154,160,255},
        .searchText      = {230,232,238,255},
        .searchIcon      = {150,154,160,255},

        .chipBg          = { 32, 36, 44,255},
        .chipText        = {220,222,228,255},

        .cardBg          = { 26, 36, 56,255},
        .cardTitle       = {160,170,186,255},
        .cardValue       = {120,170,255,255},

        .toggleIdleBg    = { 32, 36, 44,255},
        .toggleTrafficBg = {110, 80, 30,255},
        .toggleAltBg     = { 34, 46, 78,255},
        .toggleNightBg   = { 90,110,140,255},
        .toggleBorder    = { 80, 86, 96,255},
        .toggleFg        = {230,232,238,255},

        .panelBg         = { 28, 32, 40,240},
        .panelBorder     = { 60, 66, 76,255},
        .panelHover      = { 40, 56, 90,255},
        .matchDot        = {120,170,255,220},
        .suggText        = {230,232,238,255},
        .startDot        = { 80,200,110,255},
        .endDot          = {250, 90, 80,255},

        .sectionDivider  = { 44, 48, 56,255},
        .sectionTitle    = {150,154,160,255},
        .stepLine        = { 70, 90,120,255},
        .stepText        = {230,232,238,255},
        .mutedText       = {150,160,172,255},
        .stepActive      = { 40, 56, 90,255},

        .altMeta         = {150,170,186,255},
        .errorText       = {240,110,110,255},
        .footer          = {120,124,132,255},
    };
}

const SidebarPalette& activeSidebar(bool night) {
    static SidebarPalette d = daySidebar();
    static SidebarPalette n = nightSidebar();
    return night ? n : d;
}

// Small helper: pill button (clickable, single-frame true on click).
// Honors the active palette's fg/background/border colors.
bool pillButton(Rectangle r, const char* label, int fontSize,
                const SidebarPalette& p, Color fg, Color bg, Color border)
{
    DrawRectangleRounded(r, 0.5f, 8, bg);
    DrawRectangleRoundedLinesEx(r, 0.5f, 8, 1.0f, border);
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)r.x + (int)r.width / 2 - tw / 2,
             (int)r.y + (int)r.height / 2 - fontSize / 2,
             fontSize, fg);
    (void)p;
    return CheckCollisionPointRec(GetMousePosition(), r) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void infoCard(Rectangle r, const char* title, const char* value,
              const SidebarPalette& p)
{
    DrawRectangleRounded(r, 0.18f, 8, p.cardBg);
    DrawText(title, (int)r.x + 12, (int)r.y + 8,  12, p.cardTitle);
    DrawText(value, (int)r.x + 12, (int)r.y + 24, 18, p.cardValue);
}

} // namespace

void drawSidebar(Rectangle bounds,
                 const NavSystem& nav,
                 SidebarState& state,
                 SidebarIO& io,
                 const UiCallbacks& cb)
{
    const SidebarPalette& p = activeSidebar(state.nightMode);

    // --- shell ---
    DrawRectangle((int)bounds.x + 3, (int)bounds.y,
                  (int)bounds.width, (int)bounds.height, p.shellShadow);
    DrawRectangleRec(bounds, p.bg);
    DrawLine((int)bounds.x + (int)bounds.width, (int)bounds.y,
             (int)bounds.x + (int)bounds.width, (int)bounds.height,
             p.border);

    // --- header ---
    DrawRectangleRec({bounds.x, bounds.y, bounds.width, 64}, p.headerBg);
    DrawLine((int)bounds.x, (int)bounds.y + 64,
             (int)bounds.x + (int)bounds.width, (int)bounds.y + 64,
             p.headerDivider);
    DrawCircleV({bounds.x + 24, bounds.y + 24}, 8, p.headerBrand);
    DrawCircleV({bounds.x + 24, bounds.y + 24}, 4, p.headerInner);
    DrawTriangle({bounds.x + 19, bounds.y + 30},
                 {bounds.x + 29, bounds.y + 30},
                 {bounds.x + 24, bounds.y + 38},
                 p.headerBrand);
    DrawText("Navigation", (int)bounds.x + 40, (int)bounds.y + 14,
             18, p.headerTitle);

    int y = (int)bounds.y + 80;

    // --- search bar ---
    Rectangle search{bounds.x + 12, (float)y, bounds.width - 24, 30};
    bool focused = CheckCollisionPointRec(GetMousePosition(), search);
    DrawRectangleRounded(search, 0.5f, 8, p.searchBg);
    DrawRectangleRoundedLinesEx(search, 0.5f, 8, 1.0f,
                                focused ? p.searchBorderOn : p.searchBorder);

    if (io.searchBuffer.empty()) {
        DrawText("Q  Search places", (int)search.x + 10, (int)search.y + 8,
                 12, p.searchPlaceholder);
    } else {
        DrawText(io.searchBuffer.c_str(),
                 (int)search.x + 10, (int)search.y + 8, 12, p.searchText);
        int tw = MeasureText(io.searchBuffer.c_str(), 12);
        DrawText("x", (int)search.x + (int)search.width - 18,
                 (int)search.y + 8, 12, p.searchIcon);
        Rectangle clearBtn{search.x + search.width - 22, search.y,
                           22, search.height};
        if (CheckCollisionPointRec(GetMousePosition(), clearBtn) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            io.searchBuffer.clear();
        }
        (void)tw;
    }

    // Capture typed characters when the box is focused.
    int k = GetCharPressed();
    if (focused) {
        while (k > 0) {
            if (k >= 32 && k < 127) io.searchBuffer.push_back((char)k);
            k = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !io.searchBuffer.empty())
            io.searchBuffer.pop_back();
        if (IsKeyPressed(KEY_ESCAPE)) io.searchBuffer.clear();
    }

    y = (int)search.y + (int)search.height + 6;

    // --- suggestion list ---
    if (!io.searchBuffer.empty()) {
        std::string q = toLower(io.searchBuffer);
        std::vector<std::pair<int, std::string>> ranked;
        for (const auto& [name, _n] : nav.nodes) {
            int s = matchScore(name, q);
            if (s > 0) ranked.emplace_back(s, name);
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const auto& a, const auto& b) {
                      if (a.first != b.first) return a.first > b.first;
                      return a.second < b.second;
                  });
        if (ranked.size() > 5) ranked.resize(5);

        // Background panel for the dropdown
        float listH = (float)(ranked.size() * 24 + 8);
        Rectangle panel{search.x, (float)y, search.width, listH};
        DrawRectangleRounded(panel, 0.10f, 6, p.panelBg);
        DrawRectangleRoundedLinesEx(panel, 0.10f, 6, 1.0f, p.panelBorder);

        for (std::size_t i = 0; i < ranked.size(); ++i) {
            Rectangle row{search.x + 4, (float)y + 4 + i * 24,
                          search.width - 8, 22};
            bool hover = CheckCollisionPointRec(GetMousePosition(), row);
            if (hover) DrawRectangleRounded(row, 0.20f, 6, p.panelHover);

            DrawCircleV({row.x + 12, row.y + row.height / 2}, 4, p.matchDot);
            DrawText(ranked[i].second.c_str(),
                     (int)row.x + 22, (int)row.y + 5, 12, p.suggText);

            // Small "set as start" / "set as end" pills on the right.
            Rectangle setS{row.x + row.width - 86, row.y + 2, 38, 18};
            Rectangle setE{row.x + row.width - 44, row.y + 2, 38, 18};
            Color pillInnerBg = (state.nightMode ? Color{22,24,30,255}
                                                 : Color{255,255,255,255});
            if (pillButton(setS, "Start", 9, p,
                           p.startDot, pillInnerBg, p.startDot))
                io.pendingStartNode = ranked[i].second;
            if (pillButton(setE, "End", 9, p,
                           p.endDot, pillInnerBg, p.endDot))
                io.pendingEndNode = ranked[i].second;

            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Default click sets it as the destination.
                io.pendingEndNode = ranked[i].second;
            }
        }
        y += (int)listH + 4;
    } else {
        y += 8;
    }

    // --- summary card ---
    if (state.path.size() >= 2) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f km", state.totalKm);
        infoCard({bounds.x + 12, (float)y, bounds.width - 24, 60},
                 "Fastest route", buf, p);
        snprintf(buf, sizeof(buf), "%.0f min", state.totalMinutes);
        DrawText(buf, (int)bounds.x + (int)bounds.width - 80,
                 (int)y + 24, 18, p.cardValue);
        y += 70;

        // Chips row
        Rectangle chips{(float)bounds.x + 12, (float)y, bounds.width - 24, 36};
        DrawRectangleRounded(chips, 0.18f, 8, p.chipBg);
        char stops[32];
        snprintf(stops, sizeof(stops), "%zu stops", state.path.size() - 2);
        DrawText(stops, (int)chips.x + 12, (int)chips.y + 11, 12, p.chipText);
        snprintf(stops, sizeof(stops), "%.0f min", state.totalMinutes);
        DrawText(stops, (int)chips.x + 110, (int)chips.y + 11, 12, p.chipText);
        snprintf(stops, sizeof(stops), "%.1f km", state.totalKm);
        DrawText(stops, (int)chips.x + 200, (int)chips.y + 11, 12, p.chipText);
        y += 46;
    }

    // --- toggles row ---
    Rectangle tg1{bounds.x + 12, (float)y, (bounds.width - 36) / 3, 28};
    Rectangle tg2{tg1.x + tg1.width + 6, (float)y, (bounds.width - 36) / 3, 28};
    Rectangle tg3{tg2.x + tg2.width + 6, (float)y, (bounds.width - 36) / 3, 28};

    if (pillButton(tg1, "Traffic", 11, p, p.toggleFg,
                   state.trafficEnabled ? p.toggleTrafficBg : p.toggleIdleBg,
                   p.toggleBorder))
        io.pendingTrafficToggle = true;
    if (pillButton(tg2, "Alt", 11, p, p.toggleFg,
                   state.showAlternative ? p.toggleAltBg : p.toggleIdleBg,
                   p.toggleBorder))
        io.pendingAltToggle = true;
    if (pillButton(tg3, "Day/Night", 11, p, p.toggleFg,
                   state.nightMode ? p.toggleNightBg : p.toggleIdleBg,
                   p.toggleBorder))
        io.pendingNightToggle = true;

    (void)cb;
    y += 38;

    // --- turn-by-turn ---
    // Consecutive edges that share the same OSM way name are merged into a
    // single navigation step so the user sees "Head NW on Zindabazar Road
    // for 320 m" instead of 9 anonymous OSM-IDs strung together.
    struct Step {
        std::string from;        // first intersection on this segment
        std::string to;          // last intersection (== from for last step)
        std::string road;        // way name, "Unnamed Road" if missing
        float       km = 0.0f;   // sum of merged edge lengths
        bool        isLast = false;
        bool        isFirst = false;
        // Indices into state.path of the first/last vertex covered.
        int         pathStart = 0;
        int         pathEnd   = 0;
    };
    std::vector<Step> steps;
    if (state.path.size() >= 2) {
        // Build the (road, length) list for consecutive edges.
        struct Run {
            std::string road;
            float       km = 0.0f;
            int         start = 0; // index in path[] of the run's start node
            int         end   = 0; // index in path[] of the run's end node
        };
        std::vector<Run> runs;
        for (std::size_t i = 1; i < state.path.size(); ++i) {
            const std::string& a = state.path[i - 1];
            const std::string& b = state.path[i];
            std::string rn = nav.edgeName(a, b);
            float km = nav.edgeDist(a, b);
            if (!runs.empty() && runs.back().road == rn) {
                runs.back().km   += km;
                runs.back().end   = static_cast<int>(i);
            } else {
                runs.push_back(Run{rn, km,
                                   static_cast<int>(i - 1),
                                   static_cast<int>(i)});
            }
        }
        // Convert runs to Steps, attaching endpoints from state.path.
        steps.reserve(runs.size());
        for (std::size_t i = 0; i < runs.size(); ++i) {
            Step s;
            s.from      = state.path[runs[i].start];
            s.to        = state.path[runs[i].end];
            s.road      = runs[i].road;
            s.km        = runs[i].km;
            s.pathStart = runs[i].start;
            s.pathEnd   = runs[i].end;
            s.isFirst   = (i == 0);
            s.isLast    = (i + 1 == runs.size());
            steps.push_back(std::move(s));
        }
    }

    if (!steps.empty()) {
        DrawLine((int)bounds.x + 14, y,
                 (int)bounds.x + (int)bounds.width - 14, y,
                 p.sectionDivider);
        y += 12;
        DrawText("Turn-by-turn", (int)bounds.x + 14, y, 12, p.sectionTitle);
        y += 22;

        int visibleSteps = std::min(
            (int)steps.size(),
            ((int)bounds.y + (int)bounds.height - y - 60) / 38);

        for (int i = 0; i < visibleSteps; ++i) {
            const Step& step = steps[i];
            bool isS = step.isFirst;
            bool isE = step.isLast;

            // Highlight the row that contains the animator's current seg.
            bool active = state.animator.seg() >= step.pathStart &&
                          state.animator.seg() <  step.pathEnd;
            if (active)
                DrawRectangle((int)bounds.x, y - 2, (int)bounds.width, 36,
                              p.stepActive);

            Color dotC = isS ? p.startDot
                             : isE ? p.endDot
                                   : p.matchDot;
            DrawCircleV({bounds.x + 20, (float)y + 14},
                        isS || isE ? 7.0f : 5.0f, dotC);
            if (isS || isE)
                DrawCircleV({bounds.x + 20, (float)y + 14}, 3, p.bg);
            if (i < visibleSteps - 1)
                DrawLine((int)bounds.x + 20, y + 21,
                         (int)bounds.x + 20, y + 38, p.stepLine);

            // Primary line: "Onto Road Name" or "Start on Road Name"
            const char* lead = isS ? "Start on " : "Onto ";
            char line1[96];
            std::snprintf(line1, sizeof(line1), "%s%s",
                          lead, step.road.c_str());
            DrawText(line1, (int)bounds.x + 34, y + 2, 12, p.stepText);
            // Secondary line: distance + endpoint intersection (or just
            // distance on the final step).
            char line2[96];
            if (isE) {
                std::snprintf(line2, sizeof(line2),
                              "%.0f m \xC2\xB7 %s",
                              step.km * 1000.0f, step.to.c_str());
            } else {
                std::snprintf(line2, sizeof(line2),
                              "%.0f m \xC2\xB7 %s \xE2\x86\x92 %s",
                              step.km * 1000.0f,
                              step.from.c_str(), step.to.c_str());
            }
            DrawText(line2, (int)bounds.x + 34, y + 17,
                     10, p.mutedText);
            y += 38;
        }
    } else if (state.path.size() >= 2) {
        // Defensive: should not happen since steps is built from path, but
        // keeps the no-route state handled.
        DrawText("No path found",
                 (int)bounds.x + 14, y, 13, p.errorText);
    } else {
        DrawText("No path found",
                 (int)bounds.x + 14, y, 13, p.errorText);
    }

    // --- alternative path ---
    if (state.showAlternative && state.altPath.size() >= 2 &&
        y + 80 < bounds.y + bounds.height)
    {
        DrawLine((int)bounds.x + 14, y,
                 (int)bounds.x + (int)bounds.width - 14, y,
                 p.sectionDivider);
        y += 12;
        DrawText("Alternative", (int)bounds.x + 14, y, 12, p.sectionTitle);
        y += 20;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f km  %.0f min",
                 state.altKm, state.altMinutes);
        DrawText(buf, (int)bounds.x + 14, y, 13, p.altMeta);
        y += 22;
        DrawText(state.altPath.back().c_str(),
                 (int)bounds.x + 14, y, 12, p.stepText);
        y += 18;
    }

    // --- footer ---
    int fy = (int)(bounds.y + bounds.height - 18);
    DrawText("Map data (c) 2026  |  Dijkstra + A* routing",
             (int)bounds.x + 14, fy, 10, p.footer);
} 

} // namespace rm
