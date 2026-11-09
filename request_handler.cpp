#include "request_handler.h"

#include "request_handler.h"

RequestHandler::RequestHandler(
    const transport_catalogue::TransportCatalogue& db,
    const renderer::MapRenderer& renderer
)
    : db_(db)
    , renderer_(renderer) {
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.RenderMap(db_.GetAllBuses());
}