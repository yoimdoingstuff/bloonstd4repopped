#pragma once

#include "InputActions.hpp"

namespace btd4 {

struct PointerState {
    float logicalX{0.0f};
    float logicalY{0.0f};
    bool primaryDown{false};
    bool secondaryDown{false};
};

class IInput {
public:
    virtual ~IInput() = default;

    // Must be called before the platform pumps events for the next frame.
    // This establishes the baseline used for just-pressed/released queries.
    virtual void beginFrame() = 0;

    virtual bool isActionDown(InputAction action) const = 0;
    virtual bool isActionJustPressed(InputAction action) const = 0;
    virtual bool isActionJustReleased(InputAction action) const = 0;

    virtual PointerState pointerState() const = 0;
};

} // namespace btd4
