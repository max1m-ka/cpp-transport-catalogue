#pragma once

#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"

class RequestHandler {
public:
    RequestHandler(
        const transport_catalogue::TransportCatalogue& db,
        const renderer::MapRenderer& renderer
    );

    svg::Document RenderMap() const;

private:
    const transport_catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};