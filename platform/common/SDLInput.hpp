#pragma once

#include "../../engine/input/IInput.hpp"
#include "../../engine/rendering/LogicalResolution.hpp"
#include <SDL.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace btd4 {

class SDLInput : public IInput {
public:
    SDLInput();
    ~SDLInput() override = default;

    void beginFrame() override;

    bool isActionDown(InputAction action) const override;
    bool isActionJustPressed(InputAction action) const override;
    bool isActionJustReleased(InputAction action) const override;

    PointerState pointerState() const override;

    // Call from SDL event loop
    void processEvent(const SDL_Event& event, const Viewport& viewport);

private:
    uint32_t m_currentActions{0};
    uint32_t m_previousActions{0};
    uint32_t m_keyboardActions{0};
    uint32_t m_pointerActions{0};
    std::array<bool, SDL_NUM_SCANCODES> m_keyDown{};
    std::array<uint16_t, 32> m_keyActionCounts{};

    PointerState m_pointer;
    void mapKey(SDL_Keycode key, SDL_Scancode scancode, bool isDown);
    void updateCurrentActions();
};

} // namespace btd4
