#pragma once

#include "json.h"

#include <deque>
#include <optional>
#include <string>

namespace json {

class Builder {
public:
    class KeyContext;
    class DictContext;
    class ArrayContext;

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

public:
    class BaseContext {
    public:
        explicit BaseContext(Builder& builder) : builder_(builder) {}
        KeyContext Key(std::string key);
        Builder& Value(Node::Value value);
        DictContext StartDict();
        ArrayContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
        Node Build() = delete;
    protected:
        Builder& builder_;
    };

    class KeyContext : public BaseContext {
    public:
        using BaseContext::BaseContext;
        KeyContext Key(std::string) = delete;
        DictContext Value(Node::Value value);
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
    };

    class DictContext : public BaseContext {
    public:
        using BaseContext::BaseContext;
        Builder& Value(Node::Value) = delete;
        ArrayContext StartArray() = delete;
        DictContext StartDict() = delete;
        Builder& EndArray() = delete;
    };

    class ArrayContext : public BaseContext {
    public:
        using BaseContext::BaseContext;
        KeyContext Key(std::string) = delete;
        ArrayContext Value(Node::Value value);
        Builder& EndDict() = delete;
    };
};

using KeyContext = Builder::KeyContext;
using DictContext = Builder::DictContext;
using ArrayContext = Builder::ArrayContext;

}  // namespace json
