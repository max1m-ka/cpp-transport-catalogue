#include "svg.h"

#include <string>
#include <utility>

namespace svg {

namespace {

std::string Escape(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    for (char ch : text) {
        switch (ch) {
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            default: result += ch; break;
        }
    }
    return result;
}

}

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    switch(line_cap) {
        case StrokeLineCap::BUTT: out << "butt"; break;
        case StrokeLineCap::ROUND: out << "round"; break;
        case StrokeLineCap::SQUARE: out << "square"; break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    switch (line_join){
        case StrokeLineJoin::ARCS: out << "arcs"; break;
        case StrokeLineJoin::BEVEL: out << "bevel"; break;
        case StrokeLineJoin::MITER: out << "miter"; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"; break;
        case StrokeLineJoin::ROUND: out << "round"; break;
    }
    return out;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << '\n';
}

void Object::Render(std::ostream& out) const {
    RenderContext context(out);
    Render(context);
}

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\"" << center_.x
        << "\" cy=\"" << center_.y
        << "\" r=\"" << radius_ << "\"";

    RenderAttrs(context.out);
    out << "/>";
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\"";
    for (std::size_t i = 0; i < points_.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << points_[i].x << ',' << points_[i].y;
    }
    out << "\"";

    RenderAttrs(context.out);
    out << "/>";
}

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<text";
    RenderAttrs(context.out);

    out << " x=\"" << pos_.x
        << "\" y=\"" << pos_.y
        << "\" dx=\"" << offset_.x
        << "\" dy=\"" << offset_.y
        << "\" font-size=\"" << font_size_ << '"';

    if (!font_family_.empty()) {
        out << " font-family=\""
            << Escape(font_family_)
            << '"';
    }

    if (!font_weight_.empty()) {
        out << " font-weight=\""
            << Escape(font_weight_)
            << '"';
    }

    out << '>'
        << Escape(data_)
        << "</text>";
}

void Document::AddPtr(std::unique_ptr<Object>&& object) {
    objects_.push_back(std::move(object));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";

    const RenderContext context(out, 2, 2);
    for (const auto& object : objects_) {
        object->Render(context);
    }

    out << "</svg>";
}

}
