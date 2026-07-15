#include "OsmData.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pugixml.hpp"

namespace rm {

namespace {

// Pixel viewport used for the lat/lon -> raw-pixel projection. Sized to match
// the original default city (which lives in roughly [0, 700] x [0, 700]).
constexpr float kViewportW = 900.0f;
constexpr float kViewportH = 700.0f;
constexpr float kViewportMargin = 40.0f;

struct OsmNode {
    double lat = 0.0;
    double lon = 0.0;
    std::string name; // empty -> anonymous
};

// Map an OSM highway tag value to our 4-class enum.
// 0 Highway, 1 Major, 2 Minor, 3 Local. Returns -1 for unsupported types.
int roadClassFromHighway(const char* hwy) {
    if (!hwy || !*hwy) return -1;
    std::string h = hwy;
    // Arterials / trunk roads.
    if (h == "motorway" || h == "motorway_link" ||
        h == "trunk"     || h == "trunk_link")         return 0;
    // Primary / secondary arterials.
    if (h == "primary"   || h == "primary_link" ||
        h == "secondary" || h == "secondary_link")    return 1;
    // Tertiary and unclassified connectors.
    if (h == "tertiary"  || h == "tertiary_link" ||
        h == "unclassified")                          return 2;
    // Local streets we want to render.
    if (h == "residential" || h == "service" ||
        h == "living_street")                          return 3;
    // Skip: footway, path, cycleway, steps, track, pedestrian, bus_stop, ...
    return -1;
}

// Haversine distance in km between two lat/lon points.
double haversineKm(double lat1, double lon1, double lat2, double lon2) {
    constexpr double kEarthRadiusKm = 6371.0;
    constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
    double p1 = lat1 * kDegToRad;
    double p2 = lat2 * kDegToRad;
    double dp = (lat2 - lat1) * kDegToRad;
    double dl = (lon2 - lon1) * kDegToRad;
    double a = std::sin(dp / 2) * std::sin(dp / 2) +
               std::cos(p1) * std::cos(p2) *
               std::sin(dl / 2) * std::sin(dl / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return kEarthRadiusKm * c;
}

// Lookup the value of a <tag k="key" v="value"/> child, or "".
std::string tagValue(const pugi::xml_node& parent, const char* key) {
    for (auto t : parent.children("tag")) {
        std::string k = t.attribute("k").as_string("");
        if (k == key) return t.attribute("v").as_string("");
    }
    return "";
}

bool inBbox(double lat, double lon, const OsmBBox& bb) {
    return lat >= bb.south && lat <= bb.north &&
           lon >= bb.west  && lon <= bb.east;
}

} // namespace

bool loadOsmSpec(const std::string& path, const OsmBBox& bbox,
                 CitySpec& out)
{
    pugi::xml_document doc;
    auto result = doc.load_file(path.c_str());
    if (!result) {
        std::fprintf(stderr,
                     "OsmData: failed to parse %s: %s\n",
                     path.c_str(), result.description());
        return false;
    }

    auto osm = doc.child("osm");
    if (!osm) {
        std::fprintf(stderr, "OsmData: missing <osm> root in %s\n",
                     path.c_str());
        return false;
    }

    // --- Pass 1: collect nodes that fall inside our bbox ---------------
    std::unordered_map<long long, OsmNode> nodes;
    nodes.reserve(1024);
    int droppedOutOfBbox = 0;

    for (auto n : osm.children("node")) {
        double lat = n.attribute("lat").as_double(0.0);
        double lon = n.attribute("lon").as_double(0.0);
        if (!inBbox(lat, lon, bbox)) {
            ++droppedOutOfBbox;
            continue;
        }
        OsmNode node;
        node.lat = lat;
        node.lon = lon;
        node.name = tagValue(n, "name");
        long long id = n.attribute("id").as_llong(0);
        nodes.emplace(id, std::move(node));
    }

    if (nodes.empty()) {
        std::fprintf(stderr,
                     "OsmData: no nodes inside bbox in %s\n",
                     path.c_str());
        return false;
    }

    // --- Pass 2: collect ways with a usable highway tag ---------------
    struct WayHit {
        std::vector<long long> ids;
        int cls = -1;
        bool oneway = false;
    };
    std::vector<WayHit> ways;

    for (auto w : osm.children("way")) {
        std::string hwy = tagValue(w, "highway");
        int cls = roadClassFromHighway(hwy.c_str());
        if (cls < 0) continue;

        WayHit hit;
        hit.cls = cls;
        std::string ow = tagValue(w, "oneway");
        hit.oneway = (ow == "yes" || ow == "true" || ow == "1");

        for (auto nd : w.children("nd")) {
            long long ref = nd.attribute("ref").as_llong(0);
            if (ref != 0) hit.ids.push_back(ref);
        }
        if (hit.ids.size() >= 2) ways.push_back(std::move(hit));
    }

    if (ways.empty()) {
        std::fprintf(stderr,
                     "OsmData: no highway ways inside bbox in %s\n",
                     path.c_str());
        return false;
    }

    // --- Projection: bbox -> pixel viewport, uniform scale, aspect lock ---
    double worldW = std::max(1e-9, bbox.east  - bbox.west);
    double worldH = std::max(1e-9, bbox.north - bbox.south);
    float availW = kViewportW - 2 * kViewportMargin;
    float availH = kViewportH - 2 * kViewportMargin;
    float scale  = static_cast<float>(std::min(availW / worldW,
                                               availH / worldH));
    float offsetX = kViewportMargin +
                    (availW - static_cast<float>(worldW) * scale) * 0.5f;
    float offsetY = kViewportMargin;
    auto project = [&](double lat, double lon) {
        float x = offsetX + static_cast<float>(lon - bbox.west) * scale;
        // Invert latitude: north is up, but screen Y grows downward.
        float y = offsetY +
                  static_cast<float>(bbox.north - lat) * scale;
        return std::pair<float, float>{x, y};
    };

    // --- Build CitySpec ------------------------------------------------
    CitySpec spec;

    // Intersections: every node that participates in at least one surviving
    // way becomes a routable vertex. We give it a unique synthetic name
    // ("OSM-1234567") plus, if the original OSM tag had a human name, a
    // friendlier version registered first so the UI picks it up.
    std::unordered_map<long long, std::size_t> nodeIndex;
    auto addNode = [&](long long id, const OsmNode& n) {
        auto [px, py] = project(n.lat, n.lon);
        nodeIndex[id] = spec.nodes.size();
        spec.nodes.emplace_back(n.name.empty()
                                    ? "OSM-" + std::to_string(id)
                                    : n.name,
                                px, py);
    };

    // Pre-seed: any node that is referenced by a kept way and has a name
    // should appear as a named landmark even if some of its intermediate
    // neighbors are anonymous. Pass 1 above already collected them.
    for (const auto& w : ways) {
        for (long long id : w.ids) {
            if (!nodeIndex.count(id) && nodes.count(id)) {
                addNode(id, nodes[id]);
            }
        }
    }

    // Routes: emit consecutive (a, b) pairs along each way. Anonymous
    // intermediate nodes that aren't named still need to exist in the
    // graph so the polyline stays connected -- they were just registered.
    for (const auto& w : ways) {
        for (std::size_t i = 1; i < w.ids.size(); ++i) {
            auto itA = nodes.find(w.ids[i - 1]);
            auto itB = nodes.find(w.ids[i]);
            if (itA == nodes.end() || itB == nodes.end()) continue;
            auto idxA = nodeIndex.at(w.ids[i - 1]);
            auto idxB = nodeIndex.at(w.ids[i]);
            const auto& nameA = std::get<0>(spec.nodes[idxA]);
            const auto& nameB = std::get<0>(spec.nodes[idxB]);
            double km = haversineKm(itA->second.lat, itA->second.lon,
                                    itB->second.lat, itB->second.lon);
            // Treat sub-1.5m segments as zero so we don't pollute the route
            // list with degenerate duplicates.
            if (km < 0.0015) km = 0.0015;
            // No Bezier control points for OSM data -- the polyline itself
            // already provides the curve through its intermediate nodes.
            spec.routes.emplace_back(
                nameA, nameB,
                static_cast<float>(km),
                w.cls,
                w.oneway,
                /*hasCurve=*/false,
                Vector2{0.0f, 0.0f});
        }
    }

    if (spec.nodes.empty() || spec.routes.empty()) {
        std::fprintf(stderr,
                     "OsmData: empty CitySpec after conversion "
                     "(nodes=%zu, routes=%zu, dropped=%d)\n",
                     spec.nodes.size(), spec.routes.size(),
                     droppedOutOfBbox);
        return false;
    }

    std::fprintf(stderr,
                 "OsmData: loaded %s -> %zu nodes, %zu routes "
                 "(dropped %d nodes outside bbox, kept %zu ways)\n",
                 path.c_str(),
                 spec.nodes.size(), spec.routes.size(),
                 droppedOutOfBbox, ways.size());

    out = std::move(spec);
    return true;
}

} // namespace rm
