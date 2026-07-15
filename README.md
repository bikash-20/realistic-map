# realistic-map

A modular, photo-realistic-in-spirit Google Maps–style city navigation demo built with [raylib](https://www.raylib.com/) and modern C++17.

The project evolved from a single-file Dijkstra demo (`legacy/bijkstra.cpp`) into a multi-stage, configurable routing application.

## Features (v1.0)

| Stage | Highlights |
|------:|------------|
| 1     | Clean modular layout, fix of original bugs, "Arrived" state |
| 2     | Road classes (highway / major / minor), one-way streets, curved Bézier streets, traffic factor, realistic minute estimates via per-class speed limits |
| 3     | Pan & zoom camera, search bar, hover tooltips, alternative-route view, day/night palette |
| 4/5   | A\* with haversine heuristic, JSON-driven map loading, small persisted config |

## Requirements

- macOS / Linux
- raylib 4.x — `brew install raylib`
- CMake 3.20+ — `brew install cmake`
- A C++17 compiler (clang / gcc)

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/realistic_map
```

## Controls

- **Left-click** on a node — set as destination
- **Right-click** on a node — set as start
- **Mouse wheel** — zoom
- **Middle-click drag** — pan
- **Type in sidebar** — search by name
- **"Alt route"** toggle — show 2nd-best path
- **"Traffic"** toggle — multiply edge weights by 1.6
- **"Day / Night"** toggle — swap palette

## Directory layout

```
include/        public headers
src/            implementation
data/city.json  default map data
legacy/         original single-file demos (unmaintained)
```

## License

MIT for project code. `data/city.json` contains hand-curated coordinates, free to use.
