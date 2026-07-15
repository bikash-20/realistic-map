#include "AStar.h"

#include <limits>
#include <queue>
#include <unordered_map>

#include "NavSystem.h"

namespace rm {

std::vector<std::string> aStar(const NavSystem& nav,
                                const std::string& start,
                                const std::string& end,
                                bool traffic)
{
    using Pq = std::pair<float, std::string>; // f, node
    std::priority_queue<Pq, std::vector<Pq>, std::greater<Pq>> open;
    std::unordered_map<std::string, float> g;
    std::unordered_map<std::string, std::string> prev;

    constexpr float INF = 1e18f;
    for (const auto& [k, _] : nav.nodes) g[k] = INF;

    if (!nav.nodes.count(start) || !nav.nodes.count(end)) return {};
    g[start] = 0.0f;
    open.push({nav.heuristic(start, end), start});

    while (!open.empty()) {
        auto [fScore, u] = open.top();
        open.pop();
        if (u == end) break;
        if (fScore > g[u] + nav.heuristic(u, end)) continue;

        auto it = nav.graph.find(u);
        if (it == nav.graph.end()) continue;
        for (const auto& e : it->second) {
            float w = nav.edgeWeight(e, traffic);
            float tentative = g[u] + w;
            if (tentative < g[e.dest]) {
                g[e.dest] = tentative;
                prev[e.dest] = u;
                float f = tentative + nav.heuristic(e.dest, end);
                open.push({f, e.dest});
            }
        }
    }

    std::vector<std::string> path;
    if (g[end] >= INF - 1) return path;
    for (std::string v = end; v != start; v = prev[v]) {
        path.push_back(v);
        if (prev.find(v) == prev.end()) return {};
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace rm
