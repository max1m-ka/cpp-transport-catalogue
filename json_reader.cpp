#include "json_reader.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

svg::Color ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();
    }

    const json::Array& parts = node.AsArray();
    std::ostringstream out;

    out << (parts.size() == 3 ? "rgb(" : "rgba(")
        << parts[0].AsInt() << ','
        << parts[1].AsInt() << ','
        << parts[2].AsInt();

    if (parts.size() == 4) {
        out << ',' << parts[3].AsDouble();
    }

    out << ')';
    return out.str();
}

svg::Point ParsePoint(const json::Node& node) {
    const auto& values = node.AsArray();
    return {values[0].AsDouble(), values[1].AsDouble()};
}

}

renderer::RenderSettings json_reader::ParseRenderSettings(
    const json::Dict& settings
) {
    renderer::RenderSettings result;

    result.width = settings.at("width").AsDouble();
    result.height = settings.at("height").AsDouble();
    result.padding = settings.at("padding").AsDouble();
    result.line_width = settings.at("line_width").AsDouble();
    result.stop_radius = settings.at("stop_radius").AsDouble();

    result.bus_label_font_size =
        settings.at("bus_label_font_size").AsInt();
    result.bus_label_offset =
        ParsePoint(settings.at("bus_label_offset"));

    result.stop_label_font_size =
        settings.at("stop_label_font_size").AsInt();
    result.stop_label_offset =
        ParsePoint(settings.at("stop_label_offset"));

    result.underlayer_color =
        ParseColor(settings.at("underlayer_color"));
    result.underlayer_width =
        settings.at("underlayer_width").AsDouble();

    for (const json::Node& color :
         settings.at("color_palette").AsArray()) {
        result.color_palette.push_back(ParseColor(color));
    }

    return result;
}

namespace {

void LoadStops(const json::Array& requests, transport_catalogue::TransportCatalogue& catalogue) {
    for (const json::Node& node : requests) {
        const auto& request = node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            catalogue.AddStop(request.at("name").AsString(),
                {request.at("latitude").AsDouble(), request.at("longitude").AsDouble()});
        }
    }
}

void LoadRoadDistances(const json::Array& requests, transport_catalogue::TransportCatalogue& catalogue) {
    for (const json::Node& node : requests) {
        const auto& request = node.AsMap();
        if (request.at("type").AsString() != "Stop") continue;
        const auto* from = catalogue.FindStop(request.at("name").AsString());
        for (const auto& [to_name, distance] : request.at("road_distances").AsMap()) {
            catalogue.SetDistance(from, catalogue.FindStop(to_name), distance.AsInt());
        }
    }
}

void LoadBuses(const json::Array& requests, transport_catalogue::TransportCatalogue& catalogue) {
    for (const json::Node& node : requests) {
        const auto& request = node.AsMap();
        if (request.at("type").AsString() != "Bus") continue;
        std::vector<std::string_view> stop_names;
        for (const json::Node& stop : request.at("stops").AsArray()) {
            stop_names.push_back(stop.AsString());
        }
        catalogue.AddBus(request.at("name").AsString(), stop_names,
                         request.at("is_roundtrip").AsBool());
    }
}

json::Dict MakeMapAnswer(int id, const RequestHandler& handler,
                         std::optional<std::string>& cached_map) {
    if (!cached_map) {
        std::ostringstream svg_output;
        handler.RenderMap().Render(svg_output);
        cached_map = svg_output.str();
    }
    return {{"request_id", id}, {"map", *cached_map}};
}

json::Dict MakeBusAnswer(int id, const json::Dict& request,
                         const transport_catalogue::TransportCatalogue& catalogue) {
    json::Dict answer{{"request_id", id}};
    const auto stat = catalogue.GetBusInfo(request.at("name").AsString());
    if (!stat) {
        answer["error_message"] = "not found";
    } else {
        answer["curvature"] = stat->curvature;
        answer["route_length"] = stat->route_length;
        answer["stop_count"] = static_cast<int>(stat->stops_count);
        answer["unique_stop_count"] = static_cast<int>(stat->unique_stops_count);
    }
    return answer;
}

json::Dict MakeStopAnswer(int id, const json::Dict& request,
                          const transport_catalogue::TransportCatalogue& catalogue) {
    json::Dict answer{{"request_id", id}};
    const auto& name = request.at("name").AsString();
    if (!catalogue.FindStop(name)) {
        answer["error_message"] = "not found";
        return answer;
    }
    const auto& buses = catalogue.GetBusesByStop(name);
    std::vector<std::string_view> sorted_buses(buses.begin(), buses.end());
    std::sort(sorted_buses.begin(), sorted_buses.end());
    json::Array bus_names;
    bus_names.reserve(sorted_buses.size());
    for (std::string_view bus_name : sorted_buses) {
        bus_names.emplace_back(std::string(bus_name));
    }
    answer["buses"] = std::move(bus_names);
    return answer;
}

}  // namespace

void json_reader::LoadBaseRequests(
    const json::Array& requests, transport_catalogue::TransportCatalogue& catalogue) {
    LoadStops(requests, catalogue);
    LoadRoadDistances(requests, catalogue);
    LoadBuses(requests, catalogue);
}

json::Array json_reader::ProcessStatRequests(
    const json::Array& requests,
    const transport_catalogue::TransportCatalogue& catalogue,
    const RequestHandler& handler) {
    json::Array answers;
    answers.reserve(requests.size());
    std::optional<std::string> cached_map;
    for (const json::Node& node : requests) {
        const auto& request = node.AsMap();
        const int id = request.at("id").AsInt();
        const auto& type = request.at("type").AsString();
        if (type == "Map") {
            answers.emplace_back(MakeMapAnswer(id, handler, cached_map));
        } else if (type == "Bus") {
            answers.emplace_back(MakeBusAnswer(id, request, catalogue));
        } else if (type == "Stop") {
            answers.emplace_back(MakeStopAnswer(id, request, catalogue));
        } else {
            answers.emplace_back(json::Dict{{"request_id", id}});
        }
    }
    return answers;
}
