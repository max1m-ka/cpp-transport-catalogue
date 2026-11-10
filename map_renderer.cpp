#include "map_renderer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace renderer {
namespace {

constexpr double EPSILON = 1e-6;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt begin, PointInputIt end,
                    double width, double height, double padding)
        : padding_(padding) {
        if (begin == end) {
            return;
        }

        const auto [left, right] = std::minmax_element(
            begin, end, [](const auto& lhs, const auto& rhs) {
                return lhs.lng < rhs.lng;
            });
        min_lon_ = left->lng;
        const double max_lon = right->lng;

        const auto [bottom, top] = std::minmax_element(
            begin, end, [](const auto& lhs, const auto& rhs) {
                return lhs.lat < rhs.lat;
            });
        const double min_lat = bottom->lat;
        max_lat_ = top->lat;

        std::optional<double> width_zoom;
        std::optional<double> height_zoom;

        if (!IsZero(max_lon - min_lon_)) {
            width_zoom =
                (width - 2 * padding) / (max_lon - min_lon_);
        }
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom =
                (height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

void AddLabel(
    svg::Document& document,
    svg::Point position,
    svg::Point offset,
    int font_size,
    std::string_view data,
    bool bold,
    const svg::Color& color,
    const RenderSettings& settings
) {
    svg::Text label;
    label.SetPosition(position)
        .SetOffset(offset)
        .SetFontSize(static_cast<uint32_t>(font_size))
        .SetFontFamily("Verdana")
        .SetData(std::string(data));

    if (bold) {
        label.SetFontWeight("bold");
    }

    svg::Text underlayer = label;
    underlayer.SetFillColor(settings.underlayer_color)
        .SetStrokeColor(settings.underlayer_color)
        .SetStrokeWidth(settings.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    label.SetFillColor(color);

    document.Add(std::move(underlayer));
    document.Add(std::move(label));
}

void DrawRoutes(svg::Document& document, const std::vector<const transport_catalogue::Bus*>& visible_buses,  const SphereProjector& project, const RenderSettings& settings) {
    for (std::size_t i = 0; i < visible_buses.size(); ++i) {
        const auto* bus = visible_buses[i];
        svg::Polyline line;

        for (const auto* stop : bus->stops) {
            line.AddPoint(project(stop->coordinates));
        }

        line.SetFillColor(svg::NoneColor)
            .SetStrokeColor(
                settings.color_palette[
                    i % settings.color_palette.size()
                ]
            )
            .SetStrokeWidth(settings.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        document.Add(std::move(line));
    }
}

void DrawBusLabels(svg::Document& document, const std::vector<const transport_catalogue::Bus*>& visible_buses,  const SphereProjector& project, const RenderSettings& settings) {
    for (std::size_t i = 0; i < visible_buses.size(); ++i) {
        const auto* bus = visible_buses[i];
        const auto& color =
            settings.color_palette[
                i % settings.color_palette.size()
            ];

        const auto* first = bus->stops.front();

        AddLabel(
            document,
            project(first->coordinates),
            settings.bus_label_offset,
            settings.bus_label_font_size,
            bus->name,
            true,
            color,
            settings
        );

        if (!bus->is_roundtrip) {
            const auto* last =
                bus->stops[bus->stops.size() / 2];

            if (first != last) {
                AddLabel(
                    document,
                    project(last->coordinates),
                    settings.bus_label_offset,
                    settings.bus_label_font_size,
                    bus->name,
                    true,
                    color,
                    settings
                );
            }
        }
    }
}

void DrawStopCircles(svg::Document& document, const std::map<std::string_view, const transport_catalogue::Stop*>& visible_stops,  const SphereProjector& project, const RenderSettings& settings) {
    for (const auto& [name, stop] : visible_stops) {
        svg::Circle circle;
        circle.SetCenter(project(stop->coordinates))
            .SetRadius(settings.stop_radius)
            .SetFillColor("white");

        document.Add(std::move(circle));
    }
}

void DrawStopLabels(svg::Document& document, const std::map<std::string_view, const transport_catalogue::Stop*>& visible_stops,  const SphereProjector& project, const RenderSettings& settings) {
    for (const auto& [name, stop] : visible_stops) {
        AddLabel(
            document,
            project(stop->coordinates),
            settings.stop_label_offset,
            settings.stop_label_font_size,
            name,
            false,
            "black",
            settings
        );
    }
}

}

MapRenderer::MapRenderer(RenderSettings settings)
    : settings_(std::move(settings)) {
}

svg::Document MapRenderer::RenderMap(
    const std::vector<const transport_catalogue::Bus*>& buses
) const {
    std::vector<const transport_catalogue::Bus*> visible_buses;
    std::vector<geo::Coordinates> coordinates;
    std::map<
        std::string_view,
        const transport_catalogue::Stop*
    > visible_stops;

    for (const auto* bus : buses) {
        if (bus->stops.empty()) {
            continue;
        }

        visible_buses.push_back(bus);

        for (const auto* stop : bus->stops) {
            coordinates.push_back(stop->coordinates);
            visible_stops.emplace(stop->name, stop);
        }
    }

    std::sort(
        visible_buses.begin(),
        visible_buses.end(),
        [](const auto* lhs, const auto* rhs) {
            return lhs->name < rhs->name;
        }
    );

    const SphereProjector project(
        coordinates.begin(),
        coordinates.end(),
        settings_.width,
        settings_.height,
        settings_.padding
    );

    svg::Document document;
    DrawRoutes(document, visible_buses, project, settings_);
    DrawBusLabels(document, visible_buses, project, settings_);
    DrawStopCircles(document, visible_stops, project, settings_);
    DrawStopLabels(document, visible_stops, project, settings_);
    return document;
}

} // namespace renderer
