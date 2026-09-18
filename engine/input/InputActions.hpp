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
    PrevTarget  = 1 << 10,
    StartRound  = 1 << 11,
    SelectTower1 = 1 << 12,
    SelectTower2 = 1 << 13,
    SelectTower3 = 1 << 14,
    SelectTower4 = 1 << 15,
    SelectTower5 = 1 << 16,
    SelectTower6 = 1 << 17,
    UpgradePath1 = 1 << 18,
    UpgradePath2 = 1 << 19
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
