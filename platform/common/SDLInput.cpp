#include "SDLInput.hpp"

namespace btd4 {

SDLInput::SDLInput() = default;

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

void SDLInput::mapKey(SDL_Keycode key, SDL_Scancode scancode, bool isDown) {
    InputAction action = InputAction::None;

    switch (key) {
        case SDLK_RETURN:
        case SDLK_SPACE:
            action = InputAction::Confirm;
            break;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
            action = InputAction::Cancel;
            break;
        case SDLK_p:
        case SDLK_PAUSE:
            action = InputAction::Pause;
            break;
        case SDLK_u:
            action = InputAction::Upgrade;
            break;
        case SDLK_s:
            action = InputAction::Sell;
            break;
        case SDLK_UP:
        case SDLK_w:
            action = InputAction::MoveUp;
            break;
        case SDLK_DOWN:
            action = InputAction::MoveDown;
            break;
        case SDLK_LEFT:
        case SDLK_a:
            action = InputAction::MoveLeft;
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            action = InputAction::MoveRight;
            break;
        case SDLK_TAB:
            action = InputAction::NextTarget;
            break;
        default:
            break;
    }

    const size_t scancodeIndex = static_cast<size_t>(scancode);
    if (action == InputAction::None || scancodeIndex >= m_keyDown.size() || m_keyDown[scancodeIndex] == isDown) {
        return;
    }

    m_keyDown[scancodeIndex] = isDown;
    uint32_t actionMask = static_cast<uint32_t>(action);
    size_t actionIndex = 0;
    while (actionMask > 1) {
        actionMask >>= 1;
        ++actionIndex;
    }

    uint16_t& keyCount = m_keyActionCounts[actionIndex];
    if (isDown) {
        ++keyCount;
        m_keyboardActions |= static_cast<uint32_t>(action);
    } else if (keyCount > 0 && --keyCount == 0) {
        m_keyboardActions &= ~static_cast<uint32_t>(action);
    }
    updateCurrentActions();
}

void SDLInput::updateCurrentActions() {
    m_currentActions = m_keyboardActions | m_pointerActions;
}

void SDLInput::processEvent(const SDL_Event& event, const Viewport& viewport) {
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
    }
}

} // namespace btd4
