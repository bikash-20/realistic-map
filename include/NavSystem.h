#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "raylib.h"

namespace rm {

enum class RoadClass { Highway, Major, Minor, Local };

struct Edge {
    std::string dest;
    float       distKm;          // physical length in km
    RoadClass   roadClass;
    bool        oneWay;          // true => only a -> b
    // Optional single control point for a quadratic Bezier curve.
    // Logical coords (same space as Node::rawX/rawY). curved == false means
    // a straight line between the endpoints.
    bool        curved  = false;
    Vector2     ctrl{0, 0};      // only meaningful if curved == true
};

struct Node {
    std::string name;
    Vector2     pos{0, 0};       // set by MapView layout
    float       rawX{0}, rawY{0}; // logical coords before layout
};

struct Park  { float x, y, w, h; };
struct Water { float x, y, w, h; };
struct Block { float x, y, w, h; };

struct MapDecoration {
    std::vector<Park>  parks;
    std::vector<Water> waters;
    std::vector<Block> blocks;
};

class NavSystem {
public:
    std::unordered_map<std::string, Node>                   nodes;
    std::unordered_map<std::string, std::vector<Edge>>      graph;

    // --- mutation ---
    void addNode(const std::string& name, float rx, float ry);
    void addRoute(const std::string& a,
                  const std::string& b,
                  float distKm,
                  RoadClass rc = RoadClass::Minor,
                  bool oneWay  = false);
    // Curved variant: endpoint = (a,b), single Bezier control point (cx, cy)
    // expressed in the same logical coordinate space as Node::rawX/rawY.
    // Control point applies to one direction only (a->b); the reverse edge
    // (b->a) keeps its own control point if the caller provides one.
    void addRouteCurve(const std::string& a,
                       const std::string& b,
                       float distKm,
                       RoadClass rc,
                       bool oneWay,
                       Vector2 ctrl);
    // Symmetric curve: applies the same control point to both directions.
    void addRouteCurveSym(const std::string& a,
                          const std::string& b,
                          float distKm,
                          RoadClass rc,
                          Vector2 ctrl);

    // --- queries ---
    const Edge* findEdge(const std::string& a, const std::string& b) const;
    float       edgeDist(const std::string& a, const std::string& b) const;
    float       routeDist(const std::vector<std::string>& path) const;
    float       heuristic(const std::string& a, const std::string& b) const; // haversine in km

    // --- algorithms ---
    // Dijkstra; returns empty path if no route found.
    std::vector<std::string> findPath(const std::string& start,
                                      const std::string& end) const;
    // 2nd-best path: cheapest path that differs from "best" by at least one edge.
    // Excludes any edge appearing in best by setting its weight to +inf.
    std::vector<std::string> findAlternative(const std::string& start,
                                             const std::string& end,
                                             const std::vector<std::string>& best) const;

    // Edge weight used for time-based routing: distance / speed(roadClass).
    float edgeWeight(const Edge& e, bool traffic) const;
};

} // namespace rm
