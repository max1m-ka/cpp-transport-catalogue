#pragma once

#include <deque>
#include <string>
#include <vector>
#include <cstddef>
#include <utility>
#include <optional>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "domain.h"

namespace transport_catalogue {

struct Hasher {
    size_t operator() (const std::pair<const Stop*, const Stop*>& p) const {
        std::hash<const void*> hash;
        return hash(p.first) + hash(p.second) * 37;
    }
};

class TransportCatalogue {
public:
    void AddStop(const std::string& name, geo::Coordinates coordinates);
    void AddBus(const std::string& name, const std::vector<std::string_view>& stop_names, bool is_roundtrip);
    
    const Stop* FindStop(std::string_view name) const;
    const Bus* FindBus(std::string_view name) const;
    
    std::optional<BusInfo> GetBusInfo(std::string_view name) const;
    const std::unordered_set<std::string_view>& GetBusesByStop(std::string_view stop_name) const;
    const std::vector<const Bus*>& GetAllBuses() const;

    int GetDistance(const Stop* from, const Stop* to) const;
    void SetDistance(const Stop*, const Stop*, int distance);
    
private:
    std::deque<Bus> buses_;
    std::vector<const Bus*> all_buses_;
    std::deque<Stop> stops_;

    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    
    std::unordered_map<const Stop*, std::unordered_set<std::string_view>>  stop_to_buses_;
    
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, Hasher> distance_to_stop_;
};
}
