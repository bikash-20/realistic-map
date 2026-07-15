#pragma once

#include <string>

#include "MapData.h"

namespace rm {

// Geographic bounding box in decimal degrees.
// south <= north, west <= east. Use the same order Overpass expects
// (south, west, north, east).
struct OsmBBox {
    double south;
    double west;
    double north;
    double east;
};

// Loads an OpenStreetMap XML extract (downloaded from Overpass) and converts
// it into a CitySpec.
//
// Two-pass strategy (cheap O(N)):
//   1. Walk every <node> in bbox; remember id -> (lat, lon, name).
//   2. Walk every <way> with a "highway" tag; resolve its <nd> refs and emit
//      consecutive (from, to) routes. Distances are computed with haversine
//      and expressed in km.
//
// Output is in raw (x, y) pixel coordinates (the same coordinate space the
// JSON loader uses). Projection: a uniform lat/lon->pixel fit that fills a
// 900 x 700 viewport while preserving aspect ratio, anchored to (60, 60).
//
// Returns false on any parse error or empty result; out is left untouched.
bool loadOsmSpec(const std::string& path, const OsmBBox& bbox, CitySpec& out);

} // namespace rm
