#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>

namespace svg {

using Color = std::string;
inline const Color NoneColor{"none"};

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }

    double x = 0;
    double y = 0;
};

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);
std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color);
    Owner& SetStrokeColor(Color color);
    Owner& SetStrokeWidth(double width);
    Owner& SetStrokeLineCap(StrokeLineCap line_cap);
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join);

protected:
    void RenderAttrs(std::ostream& out) const {
        if (fill_color_) {
            out << " fill=\"" << *fill_color_ << "\"";
        }
        if (stroke_color_) {
            out << " stroke=\"" << *stroke_color_ << "\"";
        }
        if (stroke_width_) {
            out << " stroke-width=\"" << *stroke_width_ << "\"";
        }
        if (stroke_linecap_) {
            out << " stroke-linecap=\"" << *stroke_linecap_ << "\"";
        }
        if (stroke_linejoin_) {
            out << " stroke-linejoin=\"" << *stroke_linejoin_ << "\"";
        }
    }
private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap_;
    std::optional<StrokeLineJoin> stroke_linejoin_;
};

template <typename Owner>
Owner& PathProps<Owner>::SetFillColor(Color color) {
    fill_color_ = std::move(color);
    return AsOwner();
}

template <typename Owner>
Owner& PathProps<Owner>::SetStrokeColor(Color color) {
    stroke_color_ = std::move(color);
    return AsOwner();
}

template <typename Owner>
Owner& PathProps<Owner>::SetStrokeWidth(double width) {
    stroke_width_ = width;
    return AsOwner();
}

template <typename Owner>
Owner& PathProps<Owner>::SetStrokeLineCap(StrokeLineCap line_cap) {
    stroke_linecap_ = line_cap;
    return AsOwner();
}

template <typename Owner>
Owner& PathProps<Owner>::SetStrokeLineJoin(StrokeLineJoin line_join) {
    stroke_linejoin_ = line_join;
    return AsOwner();
}

struct RenderContext {
    explicit RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

class Object {
public:
    void Render(const RenderContext& context) const;
    void Render(std::ostream& out) const;
    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points_;
};

class Text final : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

    Point pos_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class ObjectContainer {
public:
    virtual void AddPtr(std::unique_ptr<Object>&& object) = 0;

    template <typename Obj>
    void Add(Obj object) {
        AddPtr(std::make_unique<Obj>(std::move(object)));
    }

    virtual ~ObjectContainer() = default;
};

class Drawable {
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};

class Document final : public ObjectContainer {
public:
    Document() = default;

    void AddPtr(std::unique_ptr<Object>&& object) override;
    void Render(std::ostream& out) const;

private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}
