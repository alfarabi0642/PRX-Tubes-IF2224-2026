#pragma once

#include <cstddef>
#include <string>

#include "runtime_layout.hpp"
#include "runtime_value.hpp"

namespace backend {

struct StackFrame {
    std::size_t base = 0;
    std::size_t slot_count = 0;
    int static_link = 0;
    int dynamic_link = 0;
    int return_address = 0;
};

class RuntimeStack {
public:
    explicit RuntimeStack(std::size_t max_frames = 1000);

    void clear();
    bool push_frame(std::size_t frame_slot_count,
                    int static_link,
                    int dynamic_link,
                    int return_address,
                    std::string* error = nullptr);
    bool pop_frame(std::string* error = nullptr);

    void push(RuntimeValue value);
    bool pop(RuntimeValue* out, std::string* error = nullptr);
    bool load(std::size_t address, RuntimeValue* out, std::string* error = nullptr) const;
    bool store(std::size_t address, RuntimeValue value, std::string* error = nullptr);

    std::size_t frame_count() const;
    std::size_t value_count() const;

private:
    std::size_t max_frames;

    static void set_error(std::string* target, const std::string& message);
};

}
