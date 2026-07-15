#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace rm {

class NavSystem;

// A* with haversine heuristic. Falls back to Dijkstra when heuristic is zero
// (e.g. disconnected components or degenerate coordinates).
std::vector<std::string> aStar(const NavSystem& nav,
                                const std::string& start,
                                const std::string& end,
                                bool traffic);

} // namespace rm
