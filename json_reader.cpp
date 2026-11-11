#include "json_reader.h"
#include "json_builder.h"

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

void json_reader::LoadBaseRequests(
    const json::Array& requests,
    transport_catalogue::TransportCatalogue& catalogue
) {
    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsDict();

        if (request.at("type").AsString() == "Stop") {
            catalogue.AddStop(
                request.at("name").AsString(),
                {
                    request.at("latitude").AsDouble(),
                    request.at("longitude").AsDouble()
                }
            );
        }
    }

    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsDict();

        if (request.at("type").AsString() == "Stop") {
            const auto* from =
                catalogue.FindStop(request.at("name").AsString());

            for (const auto& [to_name, distance] :
                 request.at("road_distances").AsDict()) {
                const auto* to = catalogue.FindStop(to_name);
                catalogue.SetDistance(from, to, distance.AsInt());
            }
        }
    }

    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsDict();

        if (request.at("type").AsString() == "Bus") {
            std::vector<std::string_view> stop_names;

            for (const json::Node& stop :
                 request.at("stops").AsArray()) {
                stop_names.push_back(stop.AsString());
            }

            catalogue.AddBus(
                request.at("name").AsString(),
                stop_names,
                request.at("is_roundtrip").AsBool()
            );
        }
    }
}

json::Array json_reader::ProcessStatRequests(
    const json::Array& requests,
    const transport_catalogue::TransportCatalogue& catalogue,
    const RequestHandler& handler
) {
    json::Array answers;
    answers.reserve(requests.size());

    std::optional<std::string> cached_map;

    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsDict();
        const int id = request.at("id").AsInt();
        const std::string& type = request.at("type").AsString();

        if (type == "Map") {
            if (!cached_map) {
                std::ostringstream svg_output;
                handler.RenderMap().Render(svg_output);
                cached_map = svg_output.str();
            }
            answers.emplace_back(json::Builder{}
                .StartDict()
                    .Key("request_id").Value(id)
                    .Key("map").Value(*cached_map)
                .EndDict().Build());
        } else if (type == "Bus") {
            const std::string& name = request.at("name").AsString();
            const auto stat = catalogue.GetBusInfo(name);

            if (stat) {
                answers.emplace_back(json::Builder{}
                    .StartDict()
                        .Key("request_id").Value(id)
                        .Key("curvature").Value(stat->curvature)
                        .Key("route_length").Value(stat->route_length)
                        .Key("stop_count").Value(static_cast<int>(stat->stops_count))
                        .Key("unique_stop_count").Value(static_cast<int>(stat->unique_stops_count))
                    .EndDict().Build());
            } else {
                answers.emplace_back(json::Builder{}
                    .StartDict()
                        .Key("request_id").Value(id)
                        .Key("error_message").Value(std::string("not found"))
                    .EndDict().Build());
            }
        } else if (type == "Stop") {
            const std::string& name = request.at("name").AsString();
            if (catalogue.FindStop(name)) {
                const auto& buses = catalogue.GetBusesByStop(name);
                std::vector<std::string_view> sort_buses(buses.begin(), buses.end());
                std::sort(sort_buses.begin(), sort_buses.end());

                json::Array bus_names;
                bus_names.reserve(sort_buses.size());
                for (std::string_view bus_name : sort_buses) {
                    bus_names.emplace_back(std::string(bus_name));
                }

                answers.emplace_back(json::Builder{}
                    .StartDict()
                        .Key("request_id").Value(id)
                        .Key("buses").Value(std::move(bus_names))
                    .EndDict().Build());
            } else {
                answers.emplace_back(json::Builder{}
                    .StartDict()
                        .Key("request_id").Value(id)
                        .Key("error_message").Value(std::string("not found"))
                    .EndDict().Build());
            }
        }
    }

    return answers;
}