#include "json_builder.h"
#include <stdexcept>
#include <utility>

namespace json {
void Builder::CheckCanAdd() const {
    if (started_ && open_.empty()) {
        throw std::logic_error("JSON is already complete");
    }
    if (!open_.empty() && open_.back().node.IsDict() && !open_.back().pending_key) {
        throw std::logic_error("Dictionary key is required");
    }
}

void Builder::AddNode(Node node) {
    CheckCanAdd();
    if (open_.empty()) {
        root_ = std::move(node);
        started_ = true;
    } else if (open_.back().node.IsArray()) {
        auto& array = const_cast<Array&>(open_.back().node.AsArray());
        array.push_back(std::move(node));
    } else {
        auto& dict = const_cast<Dict&>(open_.back().node.AsDict());
        auto [it, inserted] = dict.emplace(std::move(*open_.back().pending_key), std::move(node));
        open_.back().pending_key.reset();
        if (!inserted) {
            throw std::logic_error("Duplicate dictionary key");
        }
    }
}

KeyContext Builder::Key(std::string key) {
    if (open_.empty() || !open_.back().node.IsDict() || open_.back().pending_key) {
        throw std::logic_error("Key is not allowed here");
    }
    open_.back().pending_key = std::move(key);
    return KeyContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    AddNode(std::visit([](auto&& item) -> Node {
        return Node(std::forward<decltype(item)>(item));
    }, std::move(value)));
    return *this;
}

DictContext Builder::StartDict() {
    CheckCanAdd();
    open_.push_back({Node(Dict{}), std::nullopt});
    return DictContext(*this);
}

ArrayContext Builder::StartArray() {
    CheckCanAdd();
    open_.push_back({Node(Array{}), std::nullopt});
    return ArrayContext(*this);
}

Builder& Builder::EndDict() {
    if (open_.empty() || !open_.back().node.IsDict() || open_.back().pending_key) {
        throw std::logic_error("No complete dictionary to close");
    }
    Node completed = std::move(open_.back().node);
    open_.pop_back();
    AddNode(std::move(completed));
    return *this;
}

Builder& Builder::EndArray() {
    if (open_.empty() || !open_.back().node.IsArray()) {
        throw std::logic_error("No array to close");
    }
    Node completed = std::move(open_.back().node);
    open_.pop_back();
    AddNode(std::move(completed));
    return *this;
}

Node Builder::Build() {
    if (!started_ || !open_.empty()) {
        throw std::logic_error("JSON is not complete");
    }
    return std::move(root_);
}

DictContext KeyContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictContext(builder_);
}
DictContext KeyContext::StartDict() { return builder_.StartDict(); }
ArrayContext KeyContext::StartArray() { return builder_.StartArray(); }
KeyContext DictContext::Key(std::string key) { return builder_.Key(std::move(key)); }
Builder& DictContext::EndDict() { return builder_.EndDict(); }
ArrayContext ArrayContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return *this;
}
DictContext ArrayContext::StartDict() { return builder_.StartDict(); }
ArrayContext ArrayContext::StartArray() { return builder_.StartArray(); }
Builder& ArrayContext::EndArray() { return builder_.EndArray(); }
}
