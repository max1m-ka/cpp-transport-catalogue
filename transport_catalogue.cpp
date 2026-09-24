#include "transport_catalogue.h"

#include <utility>

using namespace std;

namespace transport_catalogue{

void TransportCatalogue::AddStop(const string& name, geo::Coordinates coordinates) {
    stops_.push_back({name, coordinates});
    
    const Stop* stop_ptr = &stops_.back();
    stopname_to_stop_[stop_ptr->name] = stop_ptr;
}

const Stop* TransportCatalogue::FindStop(string_view name) const {
    const auto it = stopname_to_stop_.find(name);
    
    if (it == stopname_to_stop_.end()) {
        return nullptr;
    }
    
    return it->second;
}

void TransportCatalogue::AddBus(const string& name, const vector<string_view>& stop_names) {
    Bus bus;
    bus.name = name;
    bus.stops.reserve(stop_names.size());
    
    for (string_view stop_name : stop_names) {
        bus.stops.push_back(FindStop(stop_name));
    }
    
    buses_.push_back(std::move(bus));
    
    const Bus* bus_ptr = &buses_.back();
    busname_to_bus_[bus_ptr->name] = bus_ptr;
    
    for (const Stop* stop : bus_ptr->stops) {
        stop_to_buses_[stop].insert(bus_ptr->name);
    }
}

const Bus* TransportCatalogue::FindBus(string_view name) const {
    const auto it = busname_to_bus_.find(name);
    
    if (it == busname_to_bus_.end()) {
        return nullptr;
    }
    
    return it->second;
}

optional<BusInfo> TransportCatalogue::GetBusInfo(string_view name) const {
    const Bus* bus = FindBus(name);
    
    if (bus == nullptr) {
        return nullopt;
    }
    
    std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    
    double route_length = 0.0;
    
    for (size_t i = 1; i < bus->stops.size(); ++i) {
        route_length += geo::ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
    }
    
    return BusInfo{bus->stops.size(),
        unique_stops.size(),
        route_length};
}

std::unordered_set<std::string_view> TransportCatalogue::GetBusesByStop(std::string_view stop_name) const{
    const Stop* stop = FindStop(stop_name);
    
    if (stop == nullptr) {
        return {};
    }
    
    const auto it = stop_to_buses_.find(stop);
    if (it == stop_to_buses_.end()) {
        return {};
    }
    
    return it->second;
}
}
