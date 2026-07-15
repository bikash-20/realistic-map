#pragma once
#include <string>
#include <vector>

namespace rm {

struct CitySpec {
    std::vector<std::tuple<std::string, float, float>> nodes;
    std::vector<std::tuple<std::string, std::string, float, int, bool>> routes;
    // route: from, to, distKm, roadClass (0 Highway, 1 Major, 2 Minor, 3 Local), oneWay
};

// Tries to load a JSON city file. On any failure, returns the bundled default
// city so the app always has a usable map.
CitySpec loadCitySpec(const std::string& path);

// The hand-curated fallback city used when no JSON file is present or valid.
CitySpec defaultCitySpec();

} // namespace rm
