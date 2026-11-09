#pragma once

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace json_reader {

renderer::RenderSettings ParseRenderSettings(const json::Dict& settings);

void LoadBaseRequests(const json::Array& requests, transport_catalogue::TransportCatalogue& catalogue);

json::Array ProcessStatRequests(const json::Array& requests, const transport_catalogue::TransportCatalogue& catalogue, const RequestHandler& handler);
}