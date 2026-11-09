#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace transport_catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip = false;
};

struct BusInfo {
    size_t stops_count;
    size_t unique_stops_count;
    int route_length;
    double curvature;
};

}