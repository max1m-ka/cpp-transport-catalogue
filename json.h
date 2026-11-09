#pragma once

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node() = default;
    Node(std::nullptr_t);
    Node(Array value);
    Node(Dict value);
    Node(bool value);
    Node(int value);
    Node(double value);
    Node(std::string value);
    Node(const char* value);

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    const Value& GetValue() const noexcept;

    friend bool operator==(const Node& lhs, const Node& rhs);
    friend bool operator!=(const Node& lhs, const Node& rhs);

private:
    Value value_ = nullptr;
};

class Document {
public:
    explicit Document(Node root);
    const Node& GetRoot() const;

    friend bool operator==(const Document& lhs, const Document& rhs);
    friend bool operator!=(const Document& lhs, const Document& rhs);

private:
    Node root_;
};

Document Load(std::istream& input);
void Print(const Document& doc, std::ostream& output);

}  // namespace json
