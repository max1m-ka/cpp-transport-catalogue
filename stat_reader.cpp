#include "stat_reader.h"

#include <iomanip>
#include <ostream>
#include <vector>
#include <algorithm>

namespace stat_reader {

namespace {

std::string_view Trim(std::string_view str) {
    const auto start = str.find_first_not_of(" \t\r\n");
    if (start == str.npos) {
        return {};
    }
    return str.substr(start, str.find_last_not_of(" \t\r\n") - start + 1);
}

void PrintBusInfo(const transport_catalogue::TransportCatalogue& transport_catalogue,
                  std::string_view bus_name,
                  std::ostream& output) {
    const auto bus_info = transport_catalogue.GetBusInfo(bus_name);

    output << "Bus " << bus_name << ": ";

    if (!bus_info) {
        output << "not found\n";
        return;
    }

    output << bus_info->stops_count << " stops on route, "
           << bus_info->unique_stops_count << " unique stops, "
           << bus_info->route_length << " route length, "
           << std::defaultfloat << std::setprecision(6) << bus_info->curvature
           << " curvature\n";
}

void PrintStopInfo(const transport_catalogue::TransportCatalogue& transport_catalogue,
                   std::string_view stop_name,
                   std::ostream& output) {
    output << "Stop " << stop_name << ": ";

    if (transport_catalogue.FindStop(stop_name) == nullptr) {
        output << "not found\n";
        return;
    }
    
    
    const auto buses = transport_catalogue.GetBusesByStop(stop_name);

    if (buses.empty()) {
        output << "no buses\n";
        return;
    }
    
    std::vector<std::string_view> sort_buses(buses.begin(), buses.end());
    std::sort(sort_buses.begin(), sort_buses.end());

    output << "buses";
    for (std::string_view bus_name : sort_buses) {
        output << ' ' << bus_name;
    }
    output << '\n';
}

}

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                       std::string_view request,
                       std::ostream& output) {
    request = Trim(request);
    if (request.substr(0, 4) == "Bus ") {
        PrintBusInfo(transport_catalogue, Trim(request.substr(4)), output);
    } else if (request.substr(0, 5) == "Stop ") {
        PrintStopInfo(transport_catalogue, Trim(request.substr(5)), output);
    }
}

}
