#include "runtime_stack.hpp"

#include <utility>

namespace backend {

RuntimeStack::RuntimeStack(std::size_t max_frames) : max_frames(max_frames) {}

void RuntimeStack::clear() {
    frames.clear();
    values.clear();
}

bool RuntimeStack::push_frame(std::size_t frame_slot_count,
                              std::size_t parameter_slot_count,
                              int static_link,
                              int dynamic_link,
                              int return_address,
                              std::string* error) {
    if (frames.size() >= max_frames) {
        set_error(error, "Runtime stack overflow: maximum frame count reached.");
        return false;
    }
    if (frame_slot_count < kFrameHeaderSlots) {
        set_error(error, "Invalid frame allocation: frame size is smaller than header slots.");
        return false;
    }
    if (parameter_slot_count > frame_slot_count - kFrameHeaderSlots) {
        set_error(error, "Invalid frame allocation: parameter slots exceed frame size.");
        return false;
    }

    const std::size_t operand_base = frames.empty()
        ? 0
        : frames.back().base + frames.back().slot_count;
    if (values.size() < operand_base + parameter_slot_count) {
        set_error(error, "Runtime stack underflow: not enough call argument value(s).");
        return false;
    }

    std::vector<RuntimeValue> parameters;
    if (parameter_slot_count > 0) {
        parameters.assign(values.end() - static_cast<std::ptrdiff_t>(parameter_slot_count),
                          values.end());
        values.resize(values.size() - parameter_slot_count);
    }

    StackFrame frame;
    frame.base = values.size();
    frame.slot_count = frame_slot_count;
    frame.static_link = static_link;
    frame.dynamic_link = dynamic_link;
    frame.return_address = return_address;

    values.push_back(RuntimeValue::integer(static_link));
    values.push_back(RuntimeValue::integer(dynamic_link));
    values.push_back(RuntimeValue::integer(return_address));
    for (std::size_t i = 0; i < parameter_slot_count; ++i) {
        values.push_back(parameters[i]);
    }
    for (std::size_t i = kFrameHeaderSlots + parameter_slot_count; i < frame_slot_count; ++i) {
        values.push_back(RuntimeValue::empty());
    }

    frames.push_back(frame);
    return true;
}

bool RuntimeStack::pop_frame(std::string* error) {
    if (frames.empty()) {
        set_error(error, "Runtime stack underflow: no frame to pop.");
        return false;
    }

    values.resize(frames.back().base);
    frames.pop_back();
    return true;
}

void RuntimeStack::push(RuntimeValue value) {
    values.push_back(std::move(value));
}

bool RuntimeStack::pop(RuntimeValue* out, std::string* error) {
    const std::size_t operand_base = frames.empty()
        ? 0
        : frames.back().base + frames.back().slot_count;
    if (values.size() <= operand_base) {
        set_error(error, "Runtime stack underflow: no operand value to pop.");
        return false;
    }

    if (out) {
        *out = values.back();
    }
    values.pop_back();
    return true;
}

bool RuntimeStack::peek(RuntimeValue* out, std::string* error) const {
    const std::size_t operand_base = frames.empty()
        ? 0
        : frames.back().base + frames.back().slot_count;
    if (values.size() <= operand_base) {
        set_error(error, "Runtime stack underflow: no operand value to duplicate.");
        return false;
    }

    if (out) {
        *out = values.back();
    }
    return true;
}

bool RuntimeStack::load(std::size_t lexical_level,
                        std::size_t address,
                        RuntimeValue* out,
                        std::string* error) const {
    std::size_t frame_index = 0;
    if (!resolve_frame_index(lexical_level, &frame_index, error)) {
        return false;
    }

    std::size_t resolved = 0;
    if (!resolve_frame_address(frame_index, address, &resolved, error)) {
        return false;
    }

    const RuntimeValue& value = values[resolved];
    if (value.kind == RuntimeValueKind::Empty) {
        set_error(error, "Invalid memory access: frame-relative address is uninitialized.");
        return false;
    }

    if (out) {
        *out = value;
    }
    return true;
}

bool RuntimeStack::store(std::size_t lexical_level,
                         std::size_t address,
                         RuntimeValue value,
                         std::string* error) {
    std::size_t frame_index = 0;
    if (!resolve_frame_index(lexical_level, &frame_index, error)) {
        return false;
    }

    std::size_t resolved = 0;
    if (!resolve_frame_address(frame_index, address, &resolved, error)) {
        return false;
    }

    values[resolved] = std::move(value);
    return true;
}

bool RuntimeStack::address(std::size_t lexical_level,
                           std::size_t address,
                           std::size_t* out,
                           std::string* error) const {
    std::size_t frame_index = 0;
    if (!resolve_frame_index(lexical_level, &frame_index, error)) {
        return false;
    }
    return resolve_frame_address(frame_index, address, out, error);
}

bool RuntimeStack::load_absolute(std::size_t absolute_address,
                                 RuntimeValue* out,
                                 std::string* error) const {
    if (!validate_absolute_address(absolute_address, error)) {
        return false;
    }
    const RuntimeValue& value = values[absolute_address];
    if (value.kind == RuntimeValueKind::Empty) {
        set_error(error, "Invalid memory access: absolute address is uninitialized.");
        return false;
    }
    if (out) {
        *out = value;
    }
    return true;
}

bool RuntimeStack::store_absolute(std::size_t absolute_address,
                                  RuntimeValue value,
                                  std::string* error) {
    if (!validate_absolute_address(absolute_address, error)) {
        return false;
    }
    values[absolute_address] = std::move(value);
    return true;
}

bool RuntimeStack::current_frame_index(int* out, std::string* error) const {
    if (frames.empty()) {
        set_error(error, "Invalid frame access: no active stack frame.");
        return false;
    }
    if (out) {
        *out = static_cast<int>(frames.size() - 1);
    }
    return true;
}

bool RuntimeStack::static_link_target(std::size_t lexical_level,
                                      int* out,
                                      std::string* error) const {
    std::size_t frame_index = 0;
    if (!resolve_frame_index(lexical_level, &frame_index, error)) {
        return false;
    }
    if (out) {
        *out = static_cast<int>(frame_index);
    }
    return true;
}

bool RuntimeStack::current_return_address(int* out, std::string* error) const {
    if (frames.empty()) {
        set_error(error, "Invalid frame access: no active stack frame.");
        return false;
    }
    if (out) {
        *out = frames.back().return_address;
    }
    return true;
}

std::size_t RuntimeStack::frame_count() const {
    return frames.size();
}

std::size_t RuntimeStack::value_count() const {
    return values.size();
}

bool RuntimeStack::resolve_frame_index(std::size_t lexical_level,
                                       std::size_t* frame_index,
                                       std::string* error) const {
    if (frames.empty()) {
        set_error(error, "Invalid memory access: no active stack frame.");
        return false;
    }

    std::size_t index = frames.size() - 1;
    for (std::size_t i = 0; i < lexical_level; ++i) {
        const int link = frames[index].static_link;
        if (link < 0 || static_cast<std::size_t>(link) >= frames.size()) {
            set_error(error, "Invalid static link: target frame is out of bounds.");
            return false;
        }
        index = static_cast<std::size_t>(link);
    }

    if (frame_index) {
        *frame_index = index;
    }
    return true;
}

bool RuntimeStack::resolve_frame_address(std::size_t frame_index,
                                         std::size_t address,
                                         std::size_t* resolved,
                                         std::string* error) const {
    if (frame_index >= frames.size()) {
        set_error(error, "Invalid memory access: frame index is out of bounds.");
        return false;
    }
    if (address < kFrameHeaderSlots) {
        set_error(error, "Invalid memory access: frame header slots are reserved.");
        return false;
    }

    const StackFrame& frame = frames[frame_index];
    if (address >= frame.slot_count) {
        set_error(error, "Invalid memory access: frame-relative address is out of bounds.");
        return false;
    }

    const std::size_t absolute_address = frame.base + address;
    if (absolute_address >= values.size()) {
        set_error(error, "Invalid memory access: resolved address is out of bounds.");
        return false;
    }

    if (resolved) {
        *resolved = absolute_address;
    }
    return true;
}

bool RuntimeStack::validate_absolute_address(std::size_t absolute_address,
                                             std::string* error) const {
    if (absolute_address >= values.size()) {
        set_error(error, "Invalid memory access: absolute address is out of bounds.");
        return false;
    }

    for (const auto& frame : frames) {
        if (absolute_address >= frame.base &&
            absolute_address < frame.base + frame.slot_count) {
            if (absolute_address - frame.base < kFrameHeaderSlots) {
                set_error(error, "Invalid memory access: absolute address targets frame header.");
                return false;
            }
            return true;
        }
    }

    set_error(error, "Invalid memory access: absolute address does not belong to a stack frame.");
    return false;
}

void RuntimeStack::set_error(std::string* target, const std::string& message) {
    if (target) {
        *target = message;
    }
}

}
