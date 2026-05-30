#include "runtime_stack.hpp"

namespace backend {

RuntimeStack::RuntimeStack(std::size_t max_frames) : max_frames(max_frames) {}

void RuntimeStack::clear() {
    (void)max_frames;
}

bool RuntimeStack::push_frame(std::size_t frame_slot_count,
                              int static_link,
                              int dynamic_link,
                              int return_address,
                              std::string* error) {
    (void)frame_slot_count;
    (void)static_link;
    (void)dynamic_link;
    (void)return_address;
    set_error(error, "TODO: RuntimeStack::push_frame is not implemented yet.");
    return false;
}

bool RuntimeStack::pop_frame(std::string* error) {
    set_error(error, "TODO: RuntimeStack::pop_frame is not implemented yet.");
    return false;
}

void RuntimeStack::push(RuntimeValue value) {
    (void)value;
}

bool RuntimeStack::pop(RuntimeValue* out, std::string* error) {
    if (out) {
        *out = RuntimeValue::empty();
    }
    set_error(error, "TODO: RuntimeStack::pop is not implemented yet.");
    return false;
}

bool RuntimeStack::load(std::size_t address, RuntimeValue* out, std::string* error) const {
    (void)address;
    if (out) {
        *out = RuntimeValue::empty();
    }
    set_error(error, "TODO: RuntimeStack::load is not implemented yet.");
    return false;
}

bool RuntimeStack::store(std::size_t address, RuntimeValue value, std::string* error) {
    (void)address;
    (void)value;
    set_error(error, "TODO: RuntimeStack::store is not implemented yet.");
    return false;
}

std::size_t RuntimeStack::frame_count() const {
    return 0;
}

std::size_t RuntimeStack::value_count() const {
    return 0;
}

void RuntimeStack::set_error(std::string* target, const std::string& message) {
    if (target) {
        *target = message;
    }
}

}
