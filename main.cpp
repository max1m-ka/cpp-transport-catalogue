#include "json.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <iostream>
#include <utility>

int main() {
    const json::Document input = json::Load(std::cin);
    const json::Dict& root = input.GetRoot().AsMap();

    transport_catalogue::TransportCatalogue catalogue;

    json_reader::LoadBaseRequests(
        root.at("base_requests").AsArray(),
        catalogue
    );

    renderer::MapRenderer map_renderer(
        json_reader::ParseRenderSettings(
            root.at("render_settings").AsMap()
        )
    );

    RequestHandler handler(catalogue, map_renderer);

    json::Array answers = json_reader::ProcessStatRequests(
        root.at("stat_requests").AsArray(),
        catalogue,
        handler
    );

    json::Print(
        json::Document{std::move(answers)},
        std::cout
    );
}