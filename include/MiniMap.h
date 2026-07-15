#pragma once
#include "raylib.h"

#include "MapRenderer.h"

namespace rm {

// Bottom-right minimap overlay.
// - Draws decorations, the road network, all nodes, and the active path,
//   clipped to the given rectangle.
// - Shows a faint rectangle marking the main view's currently visible region.
// - Click anywhere inside the rect to recenter the main view (one-shot).
void drawMiniMap(Rectangle rect, const RenderState& s);

// Small dark card near the hovered node listing its top connected places
// (closest neighbours) with distances.
void drawHoverTooltip(const RenderState& s);

} // namespace rm
