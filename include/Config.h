#pragma once
#include <cstddef>

namespace rm {

struct Config {
    // --- window ---
    int   windowW = 1200;
    int   windowH = 720;
    int   targetFps = 60;

    // --- sidebar ---
    float sidebarW = 310.0f;

    // --- map padding (logical, before zoom) ---
    float padL = 310.0f;
    float padR = 30.0f;
    float padT = 60.0f;
    float padB = 60.0f;

    // --- animation ---
    float carSpeedPx = 90.0f;     // pixels per second
    float dashSpeed  = 40.0f;     // dash-flow speed
    float arrivedHoldSeconds = 1.5f;

    // --- camera ---
    float minZoom = 0.6f;
    float maxZoom = 4.0f;
    float zoomStep = 1.15f;

    // --- routing ---
    bool  trafficEnabled = false;
    float trafficFactor  = 1.6f;   // multiplies edge weight when toggled
    bool  showAlternative = false;
    bool  nightMode = false;
};

extern const Config kDefaultConfig;

} // namespace rm
