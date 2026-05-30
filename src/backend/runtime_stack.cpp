#include "runtime_stack.hpp"

#include <utility>

namespace backend {

RuntimeStack::RuntimeStack(std::size_t max_frames) : max_frames(max_frames) {}

void RuntimeStack::clear() {
    frames.clear();
    values.clear();
}

bool RuntimeStack::push_frame(std::size_t frame_slot_count,
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

    StackFrame frame;
    frame.base = values.size();
    frame.slot_count = frame_slot_count;
    frame.static_link = static_link;
    frame.dynamic_link = dynamic_link;
    frame.return_address = return_address;

    values.push_back(RuntimeValue::integer(static_link));
    values.push_back(RuntimeValue::integer(dynamic_link));
    values.push_back(RuntimeValue::integer(return_address));
    for (std::size_t i = kFrameHeaderSlots; i < frame_slot_count; ++i) {
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

bool RuntimeStack::load(std::size_t address, RuntimeValue* out, std::string* error) const {
    std::size_t resolved = 0;
    if (!resolve_frame_address(address, &resolved, error)) {
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

bool RuntimeStack::store(std::size_t address, RuntimeValue value, std::string* error) {
    std::size_t resolved = 0;
    if (!resolve_frame_address(address, &resolved, error)) {
        return false;
    }

    values[resolved] = std::move(value);
    return true;
}

std::size_t RuntimeStack::frame_count() const {
    return frames.size();
}

std::size_t RuntimeStack::value_count() const {
    return values.size();
}

bool RuntimeStack::resolve_frame_address(std::size_t address,
                                         std::size_t* resolved,
                                         std::string* error) const {
    if (frames.empty()) {
        set_error(error, "Invalid memory access: no active stack frame.");
        return false;
    }
    if (address < kFrameHeaderSlots) {
        set_error(error, "Invalid memory access: frame header slots are reserved.");
        return false;
    }

    const StackFrame& frame = frames.back();
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

void RuntimeStack::set_error(std::string* target, const std::string& message) {
    if (target) {
        *target = message;
    }
}

}
