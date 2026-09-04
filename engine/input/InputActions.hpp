#pragma once

#include <cstdint>

namespace btd4 {

enum class InputAction : uint32_t {
    None        = 0,
    Confirm     = 1 << 0,
    Cancel      = 1 << 1,
    Pause       = 1 << 2,
    Upgrade     = 1 << 3,
    Sell        = 1 << 4,
    MoveUp      = 1 << 5,
    MoveDown    = 1 << 6,
    MoveLeft    = 1 << 7,
    MoveRight   = 1 << 8,
    NextTarget  = 1 << 9,
    PrevTarget  = 1 << 10
};

inline InputAction operator|(InputAction a, InputAction b) {
    return static_cast<InputAction>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline InputAction operator&(InputAction a, InputAction b) {
    return static_cast<InputAction>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasAction(InputAction mask, InputAction action) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(action)) != 0;
}

} // namespace btd4
