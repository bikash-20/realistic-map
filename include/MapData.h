#pragma once
#include <string>
#include <tuple>
#include <vector>

#include "raylib.h"

namespace rm {

struct CitySpec {
    std::vector<std::tuple<std::string, float, float>> nodes;
    // route: from, to, distKm, roadClass (0 Highway, 1 Major, 2 Minor, 3 Local),
    //        oneWay, curve (cx,cy), roadName. If hasCurve is false the route is straight.
    // roadName may be empty -- the renderer will fall back to "Unnamed Road"
    // in turn-by-turn and to a generic dot in the legend.
    std::vector<std::tuple<std::string, std::string, float, int, bool,
                           bool, Vector2, std::string>> routes;
};

// Tries to load a JSON city file. On any failure, returns the bundled default
// city so the app always has a usable map.
CitySpec loadCitySpec(const std::string& path);

// The hand-curated fallback city used when no JSON file is present or valid.
CitySpec defaultCitySpec();

} // namespace rm
