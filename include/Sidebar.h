#pragma once
#include <functional>
#include <string>
#include <vector>

#include "raylib.h"

#include "NavSystem.h"
#include "RouteAnimator.h"

namespace rm {

struct UiCallbacks {
    std::function<void(const std::string&)>   setStart;
    std::function<void(const std::string&)>   setEnd;
    std::function<void()>                     recompute;
    std::function<void(bool)>                 toggleTraffic;
    std::function<void(bool)>                 toggleAlt;
    std::function<void(bool)>                 toggleNight;
};

struct SidebarState {
    std::string             startNode;
    std::string             endNode;
    std::vector<std::string> path;
    std::vector<std::string> altPath;
    float                   totalKm = 0.0f;
    float                   totalMinutes = 0.0f;
    float                   altKm = 0.0f;
    float                   altMinutes = 0.0f;
    bool                    trafficEnabled = false;
    bool                    showAlternative = false;
    bool                    nightMode = false;
    RouteAnimator           animator;
};

struct SidebarIO {
    std::string searchBuffer;          // typed text
    std::string pendingStartNode;      // if set, main should call setStart
    std::string pendingEndNode;        // if set, main should call setEnd
    bool        pendingTrafficToggle = false;
    bool        pendingAltToggle     = false;
    bool        pendingNightToggle   = false;
    bool        requestRecompute     = false;
};

// Draws the sidebar. Mutates state (search buffer, toggles) and fills io.
void drawSidebar(Rectangle bounds,
                 const NavSystem& nav,
                 SidebarState& state,
                 SidebarIO& io,
                 const UiCallbacks& cb);

} // namespace rm
