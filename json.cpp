#include "json.h"

#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <type_traits>

namespace json {
namespace {

template <typename T>
const T& ExtractValue(const Node::Value& value, const char* expected) {
    if (const auto* result = std::get_if<T>(&value)) {
        return *result;
    }
    throw std::logic_error(std::string("Node does not contain ") + expected);
}

class Parser {
public:
    explicit Parser(std::istream& input) : input_(input) {}

    Node Parse() {
        SkipWhitespace();
        Node result = ParseValue();
        SkipWhitespace();
        if (input_.peek() != std::char_traits<char>::eof()) {
            Fail("Unexpected characters after JSON value");
        }
        return result;
    }

private:
    std::istream& input_;

    [[noreturn]] void Fail(const std::string& message) const {
        throw ParsingError(message);
    }

    void SkipWhitespace() {
        while (true) {
            const int c = input_.peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                input_.get();
            } else {
                return;
            }
        }
    }

    char ReadChar() {
        const int c = input_.get();
        if (c == std::char_traits<char>::eof()) {
            Fail("Unexpected end of input");
        }
        return static_cast<char>(c);
    }

    void Expect(char expected) {
        if (ReadChar() != expected) {
            Fail(std::string("Expected '") + expected + "'");
        }
    }

    Node ParseValue() {
        const int c = input_.peek();
        if (c == std::char_traits<char>::eof()) Fail("Expected a JSON value");
        switch (c) {
            case 'n': ParseKeyword("null"); return Node{nullptr};
            case 't': ParseKeyword("true"); return Node{true};
            case 'f': ParseKeyword("false"); return Node{false};
            case '"': input_.get(); return Node{ParseString()};
            case '[': return ParseArray();
            case '{': return ParseDict();
            default:
                if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber();
                Fail("Unexpected character while parsing JSON value");
        }
    }

    void ParseKeyword(const char* word) {
        while (*word) {
            if (ReadChar() != *word++) Fail("Invalid JSON keyword");
        }
    }

    std::string ParseString() {
        std::string result;
        while (true) {
            const char c = ReadChar();
            if (c == '"') return result;
            if (static_cast<unsigned char>(c) < 0x20) Fail("Unescaped control character in string");
            if (c != '\\') {
                result.push_back(c);
                continue;
            }
            switch (ReadChar()) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                default: Fail("Unknown escape sequence in string");
            }
        }
    }

    Node ParseNumber() {
        std::string token;
        auto take = [&]() { token.push_back(ReadChar()); };
        if (input_.peek() == '-') take();
        if (input_.peek() == '0') {
            take();
            if (input_.peek() >= '0' && input_.peek() <= '9') Fail("Leading zero in number");
        } else {
            ReadDigits(token, true);
        }
        bool is_integer = true;
        if (input_.peek() == '.') {
            is_integer = false;
            take();
            ReadDigits(token, true);
        }
        const int exp = input_.peek();
        if (exp == 'e' || exp == 'E') {
            is_integer = false;
            take();
            if (input_.peek() == '+' || input_.peek() == '-') take();
            ReadDigits(token, true);
        }
        try {
            std::size_t consumed = 0;
            if (is_integer) {
                long long value = std::stoll(token, &consumed);
                if (consumed != token.size()) Fail("Invalid integer");
                if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
                    return Node{static_cast<int>(value)};
                }
            }
            consumed = 0;
            const double value = std::stod(token, &consumed);
            if (consumed != token.size() || !std::isfinite(value)) Fail("Number is outside supported range");
            return Node{value};
        } catch (const ParsingError&) {
            throw;
        } catch (const std::exception&) {
            Fail("Invalid or out-of-range number");
        }
    }

    void ReadDigits(std::string& token, bool require_one) {
        int count = 0;
        while (input_.peek() >= '0' && input_.peek() <= '9') {
            token.push_back(ReadChar());
            ++count;
        }
        if (require_one && count == 0) Fail("Expected digit in number");
    }

    Node ParseArray() {
        Expect('[');
        SkipWhitespace();
        Array values;
        if (input_.peek() == ']') { input_.get(); return Node{std::move(values)}; }
        while (true) {
            SkipWhitespace();
            values.push_back(ParseValue());
            SkipWhitespace();
            const char separator = ReadChar();
            if (separator == ']') break;
            if (separator != ',') Fail("Expected ',' or ']' in array");
            SkipWhitespace();
            if (input_.peek() == ']') Fail("Trailing comma in array");
        }
        return Node{std::move(values)};
    }

    Node ParseDict() {
        Expect('{');
        SkipWhitespace();
        Dict values;
        if (input_.peek() == '}') { input_.get(); return Node{std::move(values)}; }
        while (true) {
            SkipWhitespace();
            if (input_.get() != '"') Fail("Expected string key in object");
            std::string key = ParseString();
            SkipWhitespace();
            Expect(':');
            SkipWhitespace();
            auto [it, inserted] = values.emplace(std::move(key), ParseValue());
            if (!inserted) Fail("Duplicate key in object");
            SkipWhitespace();
            const char separator = ReadChar();
            if (separator == '}') break;
            if (separator != ',') Fail("Expected ',' or '}' in object");
            SkipWhitespace();
            if (input_.peek() == '}') Fail("Trailing comma in object");
        }
        return Node{std::move(values)};
    }
};

void PrintString(const std::string& value, std::ostream& out) {
    out.put('"');
    for (unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            default:
                if (c < 0x20) {
                    const char* hex = "0123456789abcdef";
                    out << "\\u00" << hex[c >> 4] << hex[c & 0x0F];
                } else out.put(static_cast<char>(c));
        }
    }
    out.put('"');
}

void PrintNode(const Node& node, std::ostream& out);

void PrintArray(const Array& array, std::ostream& out) {
    out.put('[');
    bool first = true;
    for (const Node& item : array) {
        if (!first) out.put(',');
        first = false;
        PrintNode(item, out);
    }
    out.put(']');
}

void PrintDict(const Dict& dict, std::ostream& out) {
    out.put('{');
    bool first = true;
    for (const auto& [key, value] : dict) {
        if (!first) out.put(',');
        first = false;
        PrintString(key, out);
        out.put(':');
        PrintNode(value, out);
    }
    out.put('}');
}

void PrintNode(const Node& node, std::ostream& out) {
    std::visit([&out](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) out << "null";
        else if constexpr (std::is_same_v<T, std::string>) PrintString(value, out);
        else if constexpr (std::is_same_v<T, Array>) PrintArray(value, out);
        else if constexpr (std::is_same_v<T, Dict>) PrintDict(value, out);
        else if constexpr (std::is_same_v<T, bool>) out << (value ? "true" : "false");
        else out << value;
    }, node.GetValue());
}

}  // namespace

Node::Node(std::nullptr_t) : value_(nullptr) {}
Node::Node(Array value) : value_(std::move(value)) {}
Node::Node(Dict value) : value_(std::move(value)) {}
Node::Node(bool value) : value_(value) {}
Node::Node(int value) : value_(value) {}
Node::Node(double value) : value_(value) {}
Node::Node(std::string value) : value_(std::move(value)) {}
Node::Node(const char* value) : value_(std::string(value)) {}

bool Node::IsInt() const { return std::holds_alternative<int>(value_); }
bool Node::IsDouble() const { return IsInt() || IsPureDouble(); }
bool Node::IsPureDouble() const { return std::holds_alternative<double>(value_); }
bool Node::IsBool() const { return std::holds_alternative<bool>(value_); }
bool Node::IsString() const { return std::holds_alternative<std::string>(value_); }
bool Node::IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Node::IsArray() const { return std::holds_alternative<Array>(value_); }
bool Node::IsMap() const { return std::holds_alternative<Dict>(value_); }

int Node::AsInt() const { return ExtractValue<int>(value_, "int"); }
bool Node::AsBool() const { return ExtractValue<bool>(value_, "bool"); }
double Node::AsDouble() const {
    if (const auto* value = std::get_if<double>(&value_)) return *value;
    if (const auto* value = std::get_if<int>(&value_)) return static_cast<double>(*value);
    throw std::logic_error("Node does not contain a number");
}
const std::string& Node::AsString() const { return ExtractValue<std::string>(value_, "string"); }
const Array& Node::AsArray() const { return ExtractValue<Array>(value_, "array"); }
const Dict& Node::AsMap() const { return ExtractValue<Dict>(value_, "map"); }

const Node::Value& Node::GetValue() const noexcept { return value_; }

bool operator==(const Node& lhs, const Node& rhs) { return lhs.value_ == rhs.value_; }
bool operator!=(const Node& lhs, const Node& rhs) { return !(lhs == rhs); }

Document::Document(Node root) : root_(std::move(root)) {}
const Node& Document::GetRoot() const { return root_; }
bool operator==(const Document& lhs, const Document& rhs) { return lhs.root_ == rhs.root_; }
bool operator!=(const Document& lhs, const Document& rhs) { return !(lhs == rhs); }
Document Load(std::istream& input) { return Document{Parser(input).Parse()}; }
void Print(const Document& doc, std::ostream& output) { PrintNode(doc.GetRoot(), output); }

}  // namespace json
