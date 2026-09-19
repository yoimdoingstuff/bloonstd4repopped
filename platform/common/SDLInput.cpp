#include "SDLInput.hpp"

#include <algorithm>

namespace btd4 {

SDLInput::SDLInput(FrontendProfile profile)
    : m_profile(profile), m_pointer{240.0f, 136.0f, false, false} {
}

SDLInput::~SDLInput() {
    if (m_controller) {
        SDL_CloseGamepad(m_controller);
        m_controller = nullptr;
    }
}

void SDLInput::beginFrame() {
    m_previousActions = m_currentActions;
    m_pressedActions = 0;
    m_releasedActions = 0;
}

bool SDLInput::isActionDown(InputAction action) const {
    return (m_currentActions & static_cast<uint32_t>(action)) != 0;
}

bool SDLInput::isActionJustPressed(InputAction action) const {
    return (m_pressedActions & static_cast<uint32_t>(action)) != 0;
}

bool SDLInput::isActionJustReleased(InputAction action) const {
    return (m_releasedActions & static_cast<uint32_t>(action)) != 0;
}

PointerState SDLInput::pointerState() const {
    return m_pointer;
}

void SDLInput::setProfile(FrontendProfile profile) {
    m_profile = profile;
    m_keyboardActions = 0;
    m_pointerActions = 0;
    m_controllerActions = 0;
    m_previousActions = 0;
    m_pressedActions = 0;
    m_releasedActions = 0;
    m_keyDown.fill(false);
    m_keyActionCounts.fill(0);
    m_controllerActionCounts.fill(0);
    m_currentActions = 0;
    m_pointer.primaryDown = false;
    m_pointer.secondaryDown = false;
    updateCurrentActions();
}

void SDLInput::mapKey(SDL_Keycode key, SDL_Scancode scancode, bool isDown) {
    InputAction action = InputAction::None;

    switch (m_profile) {
        case FrontendProfile::FlashDesktop:
            switch (key) {
                case SDLK_RETURN:
                case SDLK_SPACE: action = InputAction::Confirm; break;
                case SDLK_ESCAPE:
                case SDLK_BACKSPACE: action = InputAction::Cancel; break;
                case SDLK_P:
                case SDLK_PAUSE: action = InputAction::Pause; break;
                case SDLK_R: action = InputAction::StartRound; break;
                case SDLK_U: action = InputAction::Upgrade; break;
                case SDLK_Q: action = InputAction::UpgradePath1; break;
                case SDLK_E: action = InputAction::UpgradePath2; break;
                case SDLK_S: action = InputAction::Sell; break;
                case SDLK_UP: action = InputAction::MoveUp; break;
                case SDLK_DOWN: action = InputAction::MoveDown; break;
                case SDLK_LEFT: action = InputAction::MoveLeft; break;
                case SDLK_RIGHT: action = InputAction::MoveRight; break;
                case SDLK_TAB: action = InputAction::NextTarget; break;
                case SDLK_F2: action = InputAction::OpenTrackEditor; break;
                case SDLK_1: action = InputAction::SelectTower1; break;
                case SDLK_2: action = InputAction::SelectTower2; break;
                case SDLK_3: action = InputAction::SelectTower3; break;
                case SDLK_4: action = InputAction::SelectTower4; break;
                case SDLK_5: action = InputAction::SelectTower5; break;
                case SDLK_6: action = InputAction::SelectTower6; break;
                default: break;
            }
            break;

        case FrontendProfile::PspConsole:
        case FrontendProfile::XboxConsole:
            // Keyboard remains useful for desktop controller testing, but follows
            // the console action layout instead of duplicating the Flash shortcuts.
            switch (key) {
                case SDLK_RETURN: action = InputAction::Confirm; break;
                case SDLK_ESCAPE: action = InputAction::Cancel; break;
                case SDLK_P: action = InputAction::Pause; break;
                case SDLK_R: action = InputAction::StartRound; break;
                case SDLK_U: action = InputAction::Upgrade; break;
                case SDLK_S: action = InputAction::Sell; break;
                case SDLK_UP: action = InputAction::MoveUp; break;
                case SDLK_DOWN: action = InputAction::MoveDown; break;
                case SDLK_LEFT: action = InputAction::MoveLeft; break;
                case SDLK_RIGHT: action = InputAction::MoveRight; break;
                case SDLK_TAB: action = InputAction::NextTarget; break;
                default: break;
            }
            break;
    }

    const size_t scancodeIndex = static_cast<size_t>(scancode);
    if (action == InputAction::None || scancodeIndex >= m_keyDown.size() || m_keyDown[scancodeIndex] == isDown) {
        return;
    }

    m_keyDown[scancodeIndex] = isDown;
    const uint32_t actionMask = static_cast<uint32_t>(action);
    size_t actionIndex = 0;
    uint32_t mask = actionMask;
    while (mask > 1) {
        mask >>= 1;
        ++actionIndex;
    }

    uint16_t& keyCount = m_keyActionCounts[actionIndex];
    if (isDown) {
        ++keyCount;
        m_keyboardActions |= actionMask;
    } else if (keyCount > 0 && --keyCount == 0) {
        m_keyboardActions &= ~actionMask;
    }
    updateCurrentActions();
}

void SDLInput::mapControllerButton(SDL_GamepadButton button, bool isDown) {
    InputAction action = InputAction::None;

    const auto movePointer = [this](float dx, float dy) {
        if (m_profile == FrontendProfile::FlashDesktop) {
            return;
        }
        m_pointer.logicalX = std::clamp(m_pointer.logicalX + dx, 0.0f, 479.0f);
        m_pointer.logicalY = std::clamp(m_pointer.logicalY + dy, 0.0f, 271.0f);
    };

    switch (m_profile) {
        case FrontendProfile::PspConsole:
            switch (button) {
                case SDL_GAMEPAD_BUTTON_SOUTH: action = InputAction::Confirm; break;
                case SDL_GAMEPAD_BUTTON_EAST: action = InputAction::Cancel; break;
                case SDL_GAMEPAD_BUTTON_WEST: action = InputAction::Sell; break;
                case SDL_GAMEPAD_BUTTON_NORTH: action = InputAction::Upgrade; break;
                case SDL_GAMEPAD_BUTTON_EASTACK: action = InputAction::Pause; break;
                case SDL_GAMEPAD_BUTTON_START: action = InputAction::StartRound; break;
                case SDL_GAMEPAD_BUTTON_DPAD_UP: action = InputAction::MoveUp; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: action = InputAction::MoveDown; break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT: action = InputAction::PrevTarget; break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: action = InputAction::NextTarget; break;
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: action = InputAction::PrevTarget; break;
                case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: action = InputAction::NextTarget; break;
                default: break;
            }
            break;

        case FrontendProfile::XboxConsole:
            switch (button) {
                case SDL_GAMEPAD_BUTTON_SOUTH: action = InputAction::Confirm; break;
                case SDL_GAMEPAD_BUTTON_EAST: action = InputAction::Cancel; break;
                case SDL_GAMEPAD_BUTTON_WEST: action = InputAction::Sell; break;
                case SDL_GAMEPAD_BUTTON_NORTH: action = InputAction::Upgrade; break;
                case SDL_GAMEPAD_BUTTON_START: action = InputAction::Pause; break;
                case SDL_GAMEPAD_BUTTON_EASTACK: action = InputAction::Cancel; break;
                case SDL_GAMEPAD_BUTTON_DPAD_UP: action = InputAction::MoveUp; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: action = InputAction::MoveDown; break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT: action = InputAction::PrevTarget; break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: action = InputAction::NextTarget; break;
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: action = InputAction::PrevTarget; break;
                case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: action = InputAction::StartRound; break;
                default: break;
            }
            break;

        case FrontendProfile::FlashDesktop:
            break;
    }

    if (action == InputAction::None) {
        return;
    }

    // D-pad/shoulder pairs can intentionally map to the same logical action.
    // Reference-count them so releasing one physical control does not cancel
    // the other while it is still held.
    const uint32_t mask = static_cast<uint32_t>(action);
    size_t actionIndex = 0;
    uint32_t actionBits = mask;
    while (actionBits > 1) {
        actionBits >>= 1;
        ++actionIndex;
    }

    uint16_t& actionCount = m_controllerActionCounts[actionIndex];
    if (isDown) {
        if (actionCount < 0xffffu) {
            ++actionCount;
            if (actionCount == 1) {
                m_controllerActions |= mask;
            }
        }
        if (m_profile != FrontendProfile::FlashDesktop) {
            switch (button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP: movePointer(0.0f, -12.0f); break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: movePointer(0.0f, 12.0f); break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT: movePointer(-12.0f, 0.0f); break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: movePointer(12.0f, 0.0f); break;
                default: break;
            }
        }
    } else if (actionCount > 0) {
        --actionCount;
        if (actionCount == 0) {
            m_controllerActions &= ~mask;
        }
    }

    updateCurrentActions();
}


void SDLInput::updateControllerDevice(const SDL_Event& event) {
    if (event.type == SDL_EVENT_GAMEPAD_ADDED && !m_controller) {
        m_controller = SDL_OpenGamepad(event.gdevice.which);
    } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED && m_controller) {
        SDL_Joystick* joystick = SDL_GetGamepadJoystick(m_controller);
        if (joystick && SDL_GetJoystickID(joystick) == event.gdevice.which) {
            // A disconnected controller should simply disappear from the held
            // action set. Do not manufacture gameplay release events while the
            // hardware itself has vanished.
            SDL_CloseGamepad(m_controller);
            m_controller = nullptr;
            m_controllerActions = 0;
            m_controllerActionCounts.fill(0);
            m_currentActions = m_keyboardActions | m_pointerActions;
        }
    }
}

void SDLInput::updateCurrentActions() {
    const uint32_t nextActions = m_keyboardActions | m_pointerActions | m_controllerActions;
    const uint32_t pressed = nextActions & ~m_currentActions;
    const uint32_t released = m_currentActions & ~nextActions;
    m_pressedActions |= pressed;
    m_releasedActions |= released;
    m_currentActions = nextActions;
}

void SDLInput::processEvent(const SDL_Event& event, const Viewport& viewport) {
    updateControllerDevice(event);

    if (event.type == SDL_EVENT_WINDOW_ && event.window.event == SDL_EVENT_WINDOW__FOCUS_LOST) {
        m_keyboardActions = 0;
        m_pointerActions = 0;
        m_controllerActions = 0;
        m_keyDown.fill(false);
        m_keyActionCounts.fill(0);
        m_controllerActionCounts.fill(0);
        m_pointer.primaryDown = false;
        m_pointer.secondaryDown = false;
        m_pressedActions = 0;
        m_releasedActions = 0;
        m_currentActions = 0;
        m_previousActions = 0;
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (!event.key.repeat) {
            mapKey(event.key.keysym.sym, event.key.keysym.scancode, true);
        }
    } else if (event.type == SDL_EVENT_KEY_UP) {
        mapKey(event.key.keysym.sym, event.key.keysym.scancode, false);
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        LogicalResolution::screenToLogical(event.motion.x, event.motion.y, viewport, m_pointer.logicalX, m_pointer.logicalY);
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        LogicalResolution::screenToLogical(event.button.x, event.button.y, viewport, m_pointer.logicalX, m_pointer.logicalY);
        if (event.button.button == SDL_BUTTON_LEFT) {
            m_pointer.primaryDown = true;
            m_pointerActions |= static_cast<uint32_t>(InputAction::Confirm);
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            m_pointer.secondaryDown = true;
            m_pointerActions |= static_cast<uint32_t>(InputAction::Cancel);
        }
        updateCurrentActions();
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        LogicalResolution::screenToLogical(event.button.x, event.button.y, viewport, m_pointer.logicalX, m_pointer.logicalY);
        if (event.button.button == SDL_BUTTON_LEFT) {
            m_pointer.primaryDown = false;
            m_pointerActions &= ~static_cast<uint32_t>(InputAction::Confirm);
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            m_pointer.secondaryDown = false;
            m_pointerActions &= ~static_cast<uint32_t>(InputAction::Cancel);
        }
        updateCurrentActions();
    } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
        if (!m_controller) return;
        SDL_Joystick* joystick = SDL_GetGamepadJoystick(m_controller);
        if (!joystick || SDL_GetJoystickID(joystick) != event.gbutton.which) return;
        mapControllerButton(
            static_cast<SDL_GamepadButton>(event.gbutton.button),
            event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
    } else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && m_profile != FrontendProfile::FlashDesktop) {
        if (!m_controller) return;
        SDL_Joystick* joystick = SDL_GetGamepadJoystick(m_controller);
        if (!joystick || SDL_GetJoystickID(joystick) != event.gaxis.which) return;
        constexpr float deadzone = 0.25f;
        if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX || event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
            const float value = static_cast<float>(event.gaxis.value) / 32767.0f;
            if (value > deadzone || value < -deadzone) {
                const float scaled = value * 7.0f;
                if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
                    m_pointer.logicalX = std::clamp(m_pointer.logicalX + scaled, 0.0f, 479.0f);
                } else {
                    m_pointer.logicalY = std::clamp(m_pointer.logicalY + scaled, 0.0f, 271.0f);
                }
            }
        }
    }
}

} // namespace btd4
