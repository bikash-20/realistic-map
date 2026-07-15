#include "MapData.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace rm {

namespace {
bool tryParseFloat(const std::string& s, float& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtof(s.c_str(), &end);
    return end && *end == '\0';
}

// Tiny tokenizer-based JSON pluck: finds the value following "key":
// returning the substring between the next quote (string), or the number.
// Adequate for our single-file format; not a general JSON parser.
std::string pluck(const std::string& blob, const std::string& key) {
    auto pos = blob.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = blob.find(':', pos);
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < blob.size() && std::isspace(static_cast<unsigned char>(blob[pos]))) ++pos;
    if (pos >= blob.size()) return "";
    if (blob[pos] == '"') {
        auto end = blob.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return blob.substr(pos + 1, end - pos - 1);
    }
    auto end = pos;
    while (end < blob.size() && blob[end] != ',' && blob[end] != '}' &&
           blob[end] != '\n' && !std::isspace(static_cast<unsigned char>(blob[end])))
    {
        ++end;
    }
    return blob.substr(pos, end - pos);
}

bool extractArrayObjs(const std::string& blob, const std::string& key,
                      std::vector<std::string>& out)
{
    out.clear();
    auto pos = blob.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos = blob.find('[', pos);
    if (pos == std::string::npos) return false;
    auto end = blob.find(']', pos);
    if (end == std::string::npos) return false;
    std::string body = blob.substr(pos + 1, end - pos - 1);
    auto cur = body.find('{');
    while (cur != std::string::npos) {
        auto close = body.find('}', cur);
        if (close == std::string::npos) break;
        out.push_back(body.substr(cur, close - cur + 1));
        cur = body.find('{', close + 1);
    }
    return true;
}
} // namespace

CitySpec loadCitySpec(const std::string& path) {
    std::ifstream in(path);
    if (!in) return defaultCitySpec();
    std::stringstream ss;
    ss << in.rdbuf();
    std::string blob = ss.str();
    if (blob.empty()) return defaultCitySpec();

    CitySpec spec;
    std::vector<std::string> nodeObjs;
    extractArrayObjs(blob, "nodes", nodeObjs);
    for (const auto& o : nodeObjs) {
        std::string name = pluck(o, "name");
        float x = 0, y = 0;
        std::string xs = pluck(o, "x");
        std::string ys = pluck(o, "y");
        if (!tryParseFloat(xs, x) || !tryParseFloat(ys, y)) continue;
        if (name.empty()) continue;
        spec.nodes.emplace_back(name, x, y);
    }

    std::vector<std::string> routeObjs;
    extractArrayObjs(blob, "routes", routeObjs);
    for (const auto& o : routeObjs) {
        std::string from = pluck(o, "from");
        std::string to   = pluck(o, "to");
        std::string dk   = pluck(o, "km");
        std::string cls  = pluck(o, "class");
        std::string ow   = pluck(o, "oneway");
        float km = 0; int clsI = 2; int owI = 0;
        if (!tryParseFloat(dk, km)) continue;
        try { clsI = std::stoi(cls); } catch (...) { clsI = 2; }
        try { owI  = std::stoi(ow);  } catch (...) { owI  = 0; }
        if (from.empty() || to.empty()) continue;
        spec.routes.emplace_back(from, to, km, clsI, owI != 0);
    }

    if (spec.nodes.empty()) return defaultCitySpec();
    return spec;
}

CitySpec defaultCitySpec() {
    CitySpec s;
    s.nodes = {
        {"Northgate",  320, 60},
        {"Westfield",  100,160},
        {"Uptown",     520,110},
        {"Riverside",  160,280},
        {"Central",    380,260},
        {"Eastpark",   600,220},
        {"Harbor",      80,420},
        {"Midtown",    300,400},
        {"Commerce",   520,380},
        {"Lakeside",   680,350},
        {"Southport",  180,540},
        {"Downtown",   400,520},
        {"Airport",    620,510},
        {"Beachfront", 260,650},
        {"Terminal",   530,640},
    };
    s.routes = {
        {"Northgate","Westfield",8, 1,0},
        {"Northgate","Uptown",   7, 1,0},
        {"Northgate","Central",  9, 0,0},
        {"Westfield","Riverside",6, 1,0},
        {"Westfield","Harbor",  10, 2,0},
        {"Uptown",   "Eastpark", 5, 1,0},
        {"Uptown",   "Central",  6, 1,0},
        {"Riverside","Central",  5, 1,0},
        {"Riverside","Harbor",   7, 2,0},
        {"Riverside","Midtown",  6, 2,0},
        {"Central",  "Eastpark", 6, 0,0},
        {"Central",  "Midtown",  5, 1,0},
        {"Central",  "Commerce", 7, 1,0},
        {"Eastpark", "Lakeside", 4, 2,0},
        {"Eastpark", "Commerce", 6, 1,0},
        {"Harbor",   "Southport",6, 2,0},
        {"Harbor",   "Midtown",  5, 2,0},
        {"Midtown",  "Commerce", 4, 1,0},
        {"Midtown",  "Southport",7, 2,0},
        {"Midtown",  "Downtown", 5, 1,0},
        {"Commerce", "Lakeside", 5, 1,0},
        {"Commerce", "Downtown", 6, 1,0},
        {"Commerce", "Airport",  7, 0,0},
        {"Lakeside", "Airport",  6, 1,0},
        {"Southport","Downtown", 6, 2,0},
        {"Southport","Beachfront",5, 2,0},
        {"Downtown", "Airport",  7, 0,0},
        {"Downtown", "Beachfront",6, 2,0},
        {"Downtown", "Terminal", 5, 1,0},
        {"Airport",  "Terminal", 6, 0,0},
        {"Beachfront","Terminal",7, 2,0},
    };
    return s;
}

} // namespace rm
