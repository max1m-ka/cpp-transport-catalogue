#pragma once

#include "json.h"

#include <deque>
#include <optional>
#include <string>
#include <utility>

namespace json {
class Builder;
class KeyContext;
class DictContext;
class ArrayContext;

class Builder {
public:
    KeyContext Key(std::string key);
    Builder& Value(Node::Value value);
    DictContext StartDict();
    ArrayContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Node Build();
private:
    void AddNode(Node node);
    void CheckCanAdd() const;
    Node root_;
    bool started_ = false;
    struct Frame {
        Node node;
        std::optional<std::string> pending_key;
    };
    std::deque<Frame> open_;
};

class KeyContext {
public:
    explicit KeyContext(Builder& builder) : builder_(builder) {}
    DictContext StartDict();
    ArrayContext StartArray();
    DictContext Value(Node::Value value);
private:
    Builder& builder_;
};

class DictContext {
public:
    explicit DictContext(Builder& builder) : builder_(builder) {}
    KeyContext Key(std::string key);
    Builder& EndDict();
private:
    Builder& builder_;
};

class ArrayContext {
public:
    explicit ArrayContext(Builder& builder) : builder_(builder) {}
    ArrayContext Value(Node::Value value);
    DictContext StartDict();
    ArrayContext StartArray();
    Builder& EndArray();
private:
    Builder& builder_;
};
}
