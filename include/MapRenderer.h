#pragma once
#include "raylib.h"

#include "MapView.h"
#include "NavSystem.h"

namespace rm {

struct RenderState {
    const NavSystem&        nav;
    const MapView&          view;
    const MapDecoration&    deco;
    const std::vector<std::string>& path;
    const std::vector<std::string>& altPath;
    std::string             startNode;
    std::string             endNode;
    std::string             hoverNode;     // empty if none
    bool                    nightMode;
    float                   dashOffset;    // animated dashes
};

void drawDecorations(const RenderState& s);
void drawRoadNetwork(const RenderState& s);
void drawRoute(const RenderState& s, const std::vector<std::string>& path,
               Color base, bool animated, bool dashed);
void drawNodes(const RenderState& s);

} // namespace rm
