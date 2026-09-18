#include "SDLInput.hpp"

#include <algorithm>

namespace btd4 {

SDLInput::SDLInput(FrontendProfile profile)
    : m_profile(profile), m_pointer{240.0f, 136.0f, false, false} {
}

SDLInput::~SDLInput() {
    if (m_controller) {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
    }
}

void SDLInput::beginFrame() {
    m_previousActions = m_currentActions;
}

bool SDLInput::isActionDown(InputAction action) const {
    return (m_currentActions & static_cast<uint32_t>(action)) != 0;
}

bool SDLInput::isActionJustPressed(InputAction action) const {
    uint32_t mask = static_cast<uint32_t>(action);
    return (m_currentActions & mask) && !(m_previousActions & mask);
}

bool SDLInput::isActionJustReleased(InputAction action) const {
    uint32_t mask = static_cast<uint32_t>(action);
    return !(m_currentActions & mask) && (m_previousActions & mask);
}

PointerState SDLInput::pointerState() const {
    return m_pointer;
}

void SDLInput::setProfile(FrontendProfile profile) {
    m_profile = profile;
    m_keyboardActions = 0;
    m_pointerActions = 0;
    m_controllerActions = 0;
    m_keyDown.fill(false);
    m_keyActionCounts.fill(0);
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
                case SDLK_p:
                case SDLK_PAUSE: action = InputAction::Pause; break;
                case SDLK_r: action = InputAction::StartRound; break;
                case SDLK_u: action = InputAction::Upgrade; break;
                case SDLK_q: action = InputAction::UpgradePath1; break;
                case SDLK_e: action = InputAction::UpgradePath2; break;
                case SDLK_s: action = InputAction::Sell; break;
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
                case SDLK_p: action = InputAction::Pause; break;
                case SDLK_r: action = InputAction::StartRound; break;
                case SDLK_u: action = InputAction::Upgrade; break;
                case SDLK_s: action = InputAction::Sell; break;
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

void SDLInput::mapControllerButton(SDL_GameControllerButton button, bool isDown) {
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
                case SDL_CONTROLLER_BUTTON_A: action = InputAction::Confirm; break;      // Cross
                case SDL_CONTROLLER_BUTTON_B: action = InputAction::Cancel; break;       // Circle
                case SDL_CONTROLLER_BUTTON_X: action = InputAction::Sell; break;         // Square
                case SDL_CONTROLLER_BUTTON_Y: action = InputAction::Upgrade; break;      // Triangle
                case SDL_CONTROLLER_BUTTON_BACK: action = InputAction::Pause; break;     // Select
                case SDL_CONTROLLER_BUTTON_START: action = InputAction::StartRound; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP: action = InputAction::MoveUp; movePointer(0, -12); break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: action = InputAction::MoveDown; movePointer(0, 12); break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: action = InputAction::PrevTarget; movePointer(-12, 0); break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: action = InputAction::NextTarget; movePointer(12, 0); break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: action = InputAction::PrevTarget; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: action = InputAction::NextTarget; break;
                default: break;
            }
            break;

        case FrontendProfile::XboxConsole:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_A: action = InputAction::Confirm; break;
                case SDL_CONTROLLER_BUTTON_B: action = InputAction::Cancel; break;
                case SDL_CONTROLLER_BUTTON_X: action = InputAction::Sell; break;
                case SDL_CONTROLLER_BUTTON_Y: action = InputAction::Upgrade; break;
                case SDL_CONTROLLER_BUTTON_START: action = InputAction::Pause; break;
                case SDL_CONTROLLER_BUTTON_BACK: action = InputAction::Cancel; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP: action = InputAction::MoveUp; movePointer(0, -12); break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: action = InputAction::MoveDown; movePointer(0, 12); break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: action = InputAction::PrevTarget; movePointer(-12, 0); break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: action = InputAction::NextTarget; movePointer(12, 0); break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: action = InputAction::PrevTarget; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: action = InputAction::StartRound; break;
                default: break;
            }
            break;

        case FrontendProfile::FlashDesktop:
            break;
    }

    if (action == InputAction::None) {
        return;
    }

    const uint32_t mask = static_cast<uint32_t>(action);
    if (isDown) {
        m_controllerActions |= mask;
    } else {
        m_controllerActions &= ~mask;
    }
    updateCurrentActions();
}

void SDLInput::updateControllerDevice(const SDL_Event& event) {
    if (event.type == SDL_CONTROLLERDEVICEADDED && !m_controller) {
        m_controller = SDL_GameControllerOpen(event.cdevice.which);
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED && m_controller) {
        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(m_controller);
        if (joystick && SDL_JoystickInstanceID(joystick) == event.cdevice.which) {
            SDL_GameControllerClose(m_controller);
            m_controller = nullptr;
            m_controllerActions = 0;
            updateCurrentActions();
        }
    }
}

void SDLInput::updateCurrentActions() {
    m_currentActions = m_keyboardActions | m_pointerActions | m_controllerActions;
}

void SDLInput::processEvent(const SDL_Event& event, const Viewport& viewport) {
    updateControllerDevice(event);

    if (event.type == SDL_KEYDOWN) {
        if (!event.key.repeat) {
            mapKey(event.key.keysym.sym, event.key.keysym.scancode, true);
        }
    } else if (event.type == SDL_KEYUP) {
        mapKey(event.key.keysym.sym, event.key.keysym.scancode, false);
    } else if (event.type == SDL_MOUSEMOTION) {
        LogicalResolution::screenToLogical(event.motion.x, event.motion.y, viewport, m_pointer.logicalX, m_pointer.logicalY);
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        LogicalResolution::screenToLogical(event.button.x, event.button.y, viewport, m_pointer.logicalX, m_pointer.logicalY);
        if (event.button.button == SDL_BUTTON_LEFT) {
            m_pointer.primaryDown = true;
            m_pointerActions |= static_cast<uint32_t>(InputAction::Confirm);
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            m_pointer.secondaryDown = true;
            m_pointerActions |= static_cast<uint32_t>(InputAction::Cancel);
        }
        updateCurrentActions();
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        LogicalResolution::screenToLogical(event.button.x, event.button.y, viewport, m_pointer.logicalX, m_pointer.logicalY);
        if (event.button.button == SDL_BUTTON_LEFT) {
            m_pointer.primaryDown = false;
            m_pointerActions &= ~static_cast<uint32_t>(InputAction::Confirm);
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            m_pointer.secondaryDown = false;
            m_pointerActions &= ~static_cast<uint32_t>(InputAction::Cancel);
        }
        updateCurrentActions();
    } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        mapControllerButton(static_cast<SDL_GameControllerButton>(event.cbutton.button), true);
    } else if (event.type == SDL_CONTROLLERBUTTONUP) {
        mapControllerButton(static_cast<SDL_GameControllerButton>(event.cbutton.button), false);
    } else if (event.type == SDL_CONTROLLERAXISMOTION && m_profile != FrontendProfile::FlashDesktop) {
        constexpr float deadzone = 0.25f;
        if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            const float value = static_cast<float>(event.caxis.value) / 32767.0f;
            if (value > deadzone || value < -deadzone) {
                const float scaled = value * 7.0f;
                if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    m_pointer.logicalX = std::clamp(m_pointer.logicalX + scaled, 0.0f, 479.0f);
                } else {
                    m_pointer.logicalY = std::clamp(m_pointer.logicalY + scaled, 0.0f, 271.0f);
                }
            }
        }
    }
}

} // namespace btd4
