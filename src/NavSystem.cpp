#include "NavSystem.h"
#include "Util.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <unordered_set>

namespace rm {

void NavSystem::addNode(const std::string& name, float rx, float ry) {
    nodes[name] = Node{name, {0, 0}, rx, ry};
}

void NavSystem::addRoute(const std::string& a, const std::string& b,
                         float distKm, RoadClass rc, bool oneWay) {
    graph[a].push_back(Edge{b, distKm, rc, oneWay});
    if (!oneWay) {
        graph[b].push_back(Edge{a, distKm, rc, false});
    }
}

const Edge* NavSystem::findEdge(const std::string& a,
                                const std::string& b) const {
    auto it = graph.find(a);
    if (it == graph.end()) return nullptr;
    for (const auto& e : it->second) {
        if (e.dest == b) return &e;
    }
    return nullptr;
}

float NavSystem::edgeDist(const std::string& a,
                          const std::string& b) const {
    if (auto* e = findEdge(a, b)) return e->distKm;
    return 0.0f;
}

float NavSystem::routeDist(const std::vector<std::string>& path) const {
    float total = 0.0f;
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        total += edgeDist(path[i], path[i + 1]);
    }
    return total;
}

float NavSystem::heuristic(const std::string& a,
                           const std::string& b) const {
    auto ia = nodes.find(a);
    auto ib = nodes.find(b);
    if (ia == nodes.end() || ib == nodes.end()) return 0.0f;
    return haversineKm(ia->second.rawX, ia->second.rawY,
                       ib->second.rawX, ib->second.rawY);
}

float NavSystem::edgeWeight(const Edge& e, bool traffic) const {
    // km / (km/h) => hours; * 60 = minutes.
    float speed = 40.0f; // default local
    switch (e.roadClass) {
        case RoadClass::Highway: speed = 100.0f; break;
        case RoadClass::Major:   speed =  70.0f; break;
        case RoadClass::Minor:   speed =  50.0f; break;
        case RoadClass::Local:   speed =  30.0f; break;
    }
    float minutes = (e.distKm / speed) * 60.0f;
    if (traffic) minutes *= 1.6f;
    return minutes;
}

namespace {
struct DijkstraResult {
    std::unordered_map<std::string, float> dist;
    std::unordered_map<std::string, std::string> prev;
    bool found = false;
};

DijkstraResult runDijkstra(
    const NavSystem& nav,
    const std::string& src,
    const std::string& dst,
    const std::unordered_set<std::string>* forbiddenEdgesFrom = nullptr,
    const std::unordered_set<std::string>* forbiddenEdgesTo   = nullptr)
{
    DijkstraResult r;
    r.found = false;
    constexpr float INF = 1e18f;
    for (const auto& [k, _] : nav.nodes) r.dist[k] = INF;
    r.dist[src] = 0.0f;

    using Pq = std::pair<float, std::string>;
    std::priority_queue<Pq, std::vector<Pq>, std::greater<Pq>> pq;
    pq.push({0.0f, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > r.dist[u]) continue;
        if (u == dst) { r.found = true; }
        auto it = nav.graph.find(u);
        if (it == nav.graph.end()) continue;
        for (const auto& e : it->second) {
            if (forbiddenEdgesFrom && forbiddenEdgesFrom->count(u) &&
                forbiddenEdgesTo     && (*forbiddenEdgesTo).count(e.dest))
            {
                // skip edge u->dest
                if ((*forbiddenEdgesFrom).count(u) &&
                    (*forbiddenEdgesTo).count(e.dest)) continue;
            }
            float nd = d + e.distKm;
            if (nd < r.dist[e.dest]) {
                r.dist[e.dest] = nd;
                r.prev[e.dest] = u;
                pq.push({nd, e.dest});
            }
        }
    }
    return r;
}
} // namespace

std::vector<std::string> NavSystem::findPath(const std::string& start,
                                             const std::string& end) const {
    auto r = runDijkstra(*this, start, end);
    std::vector<std::string> path;
    if (!r.found) return path;
    for (std::string v = end; v != start; v = r.prev[v]) {
        path.push_back(v);
        if (r.prev.find(v) == r.prev.end()) return {}; // broken
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::string> NavSystem::findAlternative(
    const std::string& start,
    const std::string& end,
    const std::vector<std::string>& best) const
{
    // For each edge in the best path, run Dijkstra with that edge removed and
    // pick the cheapest remaining path. Returns shortest if it differs.
    std::vector<std::string> bestAlt;
    float bestAltCost = std::numeric_limits<float>::infinity();
    const float primaryCost = routeDist(best);

    for (std::size_t i = 0; i + 1 < best.size(); ++i) {
        std::unordered_set<std::string> fromSet{best[i]};
        std::unordered_set<std::string> toSet{best[i + 1]};
        auto r = runDijkstra(*this, start, end, &fromSet, &toSet);
        if (!r.found) continue;
        if (r.dist[end] >= primaryCost - 0.001f) continue;
        if (r.dist[end] >= bestAltCost) continue;

        std::vector<std::string> cand;
        for (std::string v = end; v != start; v = r.prev[v]) {
            cand.push_back(v);
            if (r.prev.find(v) == r.prev.end()) { cand.clear(); break; }
        }
        if (cand.empty()) continue;
        cand.push_back(start);
        std::reverse(cand.begin(), cand.end());
        bestAlt = cand;
        bestAltCost = r.dist[end];
    }
    return bestAlt;
}

} // namespace rm
