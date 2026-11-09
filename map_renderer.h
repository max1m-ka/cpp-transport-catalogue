#pragma once

#include "domain.h"
#include "svg.h"

#include <vector>

namespace renderer {
struct RenderSettings {
    double width = 0;
    double height = 0;
    double padding = 0;
    double line_width = 0;
    double stop_radius = 0;

    int bus_label_font_size = 0;
    svg::Point bus_label_offset;

    int stop_label_font_size = 0;
    svg::Point stop_label_offset;

    svg::Color underlayer_color;
    double underlayer_width = 0;
    std::vector<svg::Color> color_palette;
};

class MapRenderer {
public:
    explicit MapRenderer(RenderSettings settings);

    svg::Document RenderMap(const std::vector<const transport_catalogue::Bus*>& buses) const;
private:
    RenderSettings settings_;
};
}