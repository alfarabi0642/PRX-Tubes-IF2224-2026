#pragma once

#include <cstddef>
#include <string>
#include <vector>

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
                    std::size_t parameter_slot_count,
                    int static_link,
                    int dynamic_link,
                    int return_address,
                    std::string* error = nullptr);
    bool pop_frame(std::string* error = nullptr);

    void push(RuntimeValue value);
    bool pop(RuntimeValue* out, std::string* error = nullptr);
    bool peek(RuntimeValue* out, std::string* error = nullptr) const;
    bool load(std::size_t lexical_level, std::size_t address,
              RuntimeValue* out, std::string* error = nullptr) const;
    bool store(std::size_t lexical_level, std::size_t address,
               RuntimeValue value, std::string* error = nullptr);
    bool address(std::size_t lexical_level, std::size_t address,
                 std::size_t* out, std::string* error = nullptr) const;
    bool load_absolute(std::size_t absolute_address, RuntimeValue* out,
                       std::string* error = nullptr) const;
    bool store_absolute(std::size_t absolute_address, RuntimeValue value,
                        std::string* error = nullptr);
    bool current_frame_index(int* out, std::string* error = nullptr) const;
    bool static_link_target(std::size_t lexical_level, int* out,
                            std::string* error = nullptr) const;
    bool current_return_address(int* out, std::string* error = nullptr) const;

    std::size_t frame_count() const;
    std::size_t value_count() const;

private:
    std::vector<StackFrame> frames;
    std::vector<RuntimeValue> values;
    std::size_t max_frames;

    bool resolve_frame_index(std::size_t lexical_level,
                             std::size_t* frame_index,
                             std::string* error = nullptr) const;
    bool resolve_frame_address(std::size_t frame_index,
                               std::size_t address,
                               std::size_t* resolved,
                               std::string* error = nullptr) const;
    bool validate_absolute_address(std::size_t absolute_address,
                                   std::string* error = nullptr) const;
    static void set_error(std::string* target, const std::string& message);
};

}
