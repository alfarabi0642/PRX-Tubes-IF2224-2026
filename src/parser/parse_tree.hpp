#pragma once

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

class ParseTreeNode {
public:
    std::string name;
    std::vector<std::shared_ptr<ParseTreeNode>> children;

    explicit ParseTreeNode(const std::string& name) : name(name) {}

    void add_child(const std::shared_ptr<ParseTreeNode>& child) {
        if (child) {
            children.push_back(child);
        } else {
            children.push_back(std::make_shared<ParseTreeNode>("<empty>"));
        }
    }

    bool is_terminal() const {
        return children.empty();
    }

    void print_children(std::ostream& os, const std::string& prefix = "") const {
        for (size_t i = 0; i < children.size(); ++i) {
            const bool is_last = i == children.size() - 1;
            os << prefix << (is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 " : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ") << children[i]->name << '\n';
            children[i]->print_children(os, prefix + (is_last ? "    " : "\xe2\x94\x82   "));
        }
    }

    void print(std::ostream& os) const {
        os << name << '\n';
        print_children(os);
    }
};

inline std::shared_ptr<ParseTreeNode> make_node(const std::string& name) {
    return std::make_shared<ParseTreeNode>(name);
}

