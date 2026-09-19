#pragma once

#include "../../engine/input/IInput.hpp"
#include "../../engine/input/FrontendProfile.hpp"
#include "../../engine/rendering/LogicalResolution.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace btd4 {

class SDLInput : public IInput {
public:
    explicit SDLInput(FrontendProfile profile = FrontendProfile::FlashDesktop);
    ~SDLInput() override;

    void beginFrame() override;

    bool isActionDown(InputAction action) const override;
    bool isActionJustPressed(InputAction action) const override;
    bool isActionJustReleased(InputAction action) const override;

    PointerState pointerState() const override;

    void setProfile(FrontendProfile profile);
    FrontendProfile profile() const { return m_profile; }

    // Call from SDL event loop.
    void processEvent(const SDL_Event& event, const Viewport& viewport);

private:
    FrontendProfile m_profile{FrontendProfile::FlashDesktop};
    uint32_t m_currentActions{0};
    uint32_t m_previousActions{0};
    uint32_t m_pressedActions{0};
    uint32_t m_releasedActions{0};
    uint32_t m_keyboardActions{0};
    uint32_t m_pointerActions{0};
    uint32_t m_controllerActions{0};
    std::array<bool, SDL_SCANCODE_COUNT> m_keyDown{};
    std::array<uint16_t, 32> m_keyActionCounts{};
    std::array<uint16_t, 32> m_controllerActionCounts{};

    PointerState m_pointer;
    SDL_Gamepad* m_controller{nullptr};

    void mapKey(SDL_Keycode key, SDL_Scancode scancode, bool isDown);
    void mapControllerButton(SDL_GamepadButton button, bool isDown);
    void updateControllerDevice(const SDL_Event& event);
    void updateCurrentActions();
};

} // namespace btd4
